
typedef struct {
    uv_write_t req;
    Stream *obj;
    PyObject *callback;
    PyObject *send_handle;
    Py_buffer *views;
    Py_buffer viewsml[4];
    int view_count;
} stream_write_ctx;


typedef struct {
    uv_shutdown_t req;
    Stream *obj;
    PyObject *callback;
} stream_shutdown_ctx;


static void
pyuv__stream_shutdown_cb(uv_shutdown_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    stream_shutdown_ctx *ctx;
    Stream *self;
    PyObject *callback, *result, *py_errorno;

    ctx = PYUV_CONTAINER_OF(req, stream_shutdown_ctx, req);
    self = ctx->obj;
    callback = ctx->callback;

    if (callback != Py_None) {
        if (status < 0) {
            py_errorno = PyInt_FromLong((long)status);
        } else {
            py_errorno = Py_None;
            Py_INCREF(Py_None);
        }
        result = PyObject_CallFunctionObjArgs(callback, self, py_errorno, NULL);
        if (result == NULL) {
            handle_uncaught_exception(HANDLE(self)->loop);
        }
        Py_XDECREF(result);
        Py_DECREF(py_errorno);
    }

    Py_DECREF(callback);
    PyMem_Free(req);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
pyuv__stream_read_cb(uv_stream_t* handle, int nread, const uv_buf_t* buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Loop *loop;
    Stream *self;
    PyObject *result, *data, *py_errorno;
    ASSERT(handle);

    /* Can't use container_of here */
    self = (Stream *)handle->data;

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread >= 0) {
        data = PyBytes_FromStringAndSize(buf->base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else {
        data = Py_None;
        Py_INCREF(Py_None);
        py_errorno = PyInt_FromLong((long)nread);
        /* Stop reading, otherwise an assert blows up on unix */
        uv_read_stop(handle);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

    /* data has been read, unlock the buffer */
    loop = handle->loop->data;
    ASSERT(loop);
    loop->buffer.in_use = False;

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
pyuv__stream_write_cb(uv_write_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    stream_write_ctx *ctx;
    Stream *self;
    PyObject *callback, *send_handle, *result, *py_errorno;

    ASSERT(req);

    ctx = PYUV_CONTAINER_OF(req, stream_write_ctx, req);
    self = ctx->obj;
    callback = ctx->callback;
    send_handle = ctx->send_handle;

    if (callback != Py_None) {
        if (status < 0) {
            py_errorno = PyInt_FromLong((long)status);
        } else {
            py_errorno = Py_None;
            Py_INCREF(Py_None);
        }
        result = PyObject_CallFunctionObjArgs(callback, self, py_errorno, NULL);
        if (result == NULL) {
            handle_uncaught_exception(HANDLE(self)->loop);
        }
        Py_XDECREF(result);
        Py_DECREF(py_errorno);
    }

    Py_DECREF(callback);
    Py_XDECREF(send_handle);

    for (i = 0; i < ctx->view_count; i++)
        PyBuffer_Release(&ctx->views[i]);
    if (ctx->views != ctx->viewsml)
        PyMem_Free(ctx->views);
    PyMem_Free(ctx);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
Stream_func_shutdown(Stream *self, PyObject *args)
{
    int err;
    stream_shutdown_ctx *ctx;
    PyObject *callback;

    ctx = NULL;
    callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "|O:shutdown", &callback)) {
        return NULL;
    }

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(callback);

    ctx->obj = self;
    ctx->callback = callback;

    err = uv_shutdown(&ctx->req, (uv_stream_t *)UV_HANDLE(self), pyuv__stream_shutdown_cb);
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    PyMem_Free(ctx);
    return NULL;
}


static PyObject *
Stream_func_start_read(Stream *self, PyObject *args)
{
    int err;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start_read", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    err = uv_read_start((uv_stream_t *)UV_HANDLE(self), (uv_alloc_cb)pyuv__alloc_cb, (uv_read_cb)pyuv__stream_read_cb);
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        return NULL;
    }

    tmp = self->on_read_cb;
    Py_INCREF(callback);
    self->on_read_cb = callback;
    Py_XDECREF(tmp);

    PYUV_HANDLE_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
Stream_func_stop_read(Stream *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_read_stop((uv_stream_t *)UV_HANDLE(self));
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        return NULL;
    }

    Py_XDECREF(self->on_read_cb);
    self->on_read_cb = NULL;

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static PyObject *
Stream_func_try_write(Stream *self, PyObject *args)
{
    int err;
    uv_buf_t buf;
    Py_buffer view;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, PYUV_BYTES"*:try_write", &view)) {
        return NULL;
    }

    buf = uv_buf_init(view.buf, view.len);
    err = uv_try_write((uv_stream_t *)UV_HANDLE(self), &buf, 1);
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        PyBuffer_Release(&view);
        return NULL;
    }

    PyBuffer_Release(&view);
    return PyInt_FromLong((long)err);
}


static PyObject *
pyuv__stream_write_bytes(Stream *self, PyObject *data, PyObject *callback, PyObject *send_handle)
{
    int err;
    uv_buf_t buf;
    stream_write_ctx *ctx;
    Py_buffer *view;

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    ctx->views = ctx->viewsml;
    view = &ctx->views[0];

    if (PyObject_GetBuffer(data, view, PyBUF_SIMPLE) != 0) {
        PyMem_Free(ctx);
        return NULL;
    }

    ctx->view_count = 1;
    ctx->obj = self;
    ctx->callback = callback;
    ctx->send_handle = send_handle;

    Py_INCREF(callback);
    Py_XINCREF(send_handle);

    buf = uv_buf_init(view->buf, view->len);
    if (send_handle != NULL) {
        ASSERT(UV_HANDLE(self)->type == UV_NAMED_PIPE);
        err = uv_write2(&ctx->req, (uv_stream_t *)UV_HANDLE(self), &buf, 1, (uv_stream_t *)UV_HANDLE(send_handle), pyuv__stream_write_cb);
    } else {
        err = uv_write(&ctx->req, (uv_stream_t *)UV_HANDLE(self), &buf, 1, pyuv__stream_write_cb);
    }

    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        Py_DECREF(callback);
        Py_XDECREF(send_handle);
        PyBuffer_Release(view);
        PyMem_Free(ctx);
        return NULL;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
pyuv__stream_write_sequence(Stream *self, PyObject *data, PyObject *callback, PyObject *send_handle)
{
    int err;
    stream_write_ctx *ctx;
    PyObject *data_fast, *item;
    Py_ssize_t i, j, buf_count;

    data_fast = PySequence_Fast(data, "data must be an iterable");
    if (data_fast == NULL)
        return NULL;

    buf_count = PySequence_Fast_GET_SIZE(data_fast);
    if (buf_count > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "iterable is too long");
        Py_DECREF(data_fast);
        return NULL;
    }

    if (buf_count == 0) {
        PyErr_SetString(PyExc_ValueError, "iterable is empty");
        Py_DECREF(data_fast);
        return NULL;
    }

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        Py_DECREF(data_fast);
        return NULL;
    }

    ctx->views = ctx->viewsml;
    if (buf_count > ARRAY_SIZE(ctx->viewsml))
        ctx->views = PyMem_Malloc(sizeof(Py_buffer) * buf_count);
    if (!ctx->views) {
        PyErr_NoMemory();
        PyMem_Free(ctx);
        Py_DECREF(data_fast);
        return NULL;
    }
    ctx->view_count = buf_count;

    {
        STACK_ARRAY(uv_buf_t, bufs, buf_count);

        for (i = 0; i < buf_count; i++) {
            item = PySequence_Fast_GET_ITEM(data_fast, i);
            if (PyObject_GetBuffer(item, &ctx->views[i], PyBUF_SIMPLE) != 0)
                goto error;
            bufs[i].base = ctx->views[i].buf;
            bufs[i].len = ctx->views[i].len;
        }

        ctx->obj = self;
        ctx->callback = callback;
        ctx->send_handle = send_handle;

        Py_INCREF(callback);
        Py_XINCREF(send_handle);

        if (send_handle != NULL) {
            ASSERT(UV_HANDLE(self)->type == UV_NAMED_PIPE);
            err = uv_write2(&ctx->req, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, (uv_stream_t *)UV_HANDLE(send_handle), pyuv__stream_write_cb);
        } else {
            err = uv_write(&ctx->req, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, pyuv__stream_write_cb);
        }
    }

    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        Py_DECREF(callback);
        Py_XDECREF(send_handle);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    for (j = 0; j < i; j++)
        PyBuffer_Release(&ctx->views[j]);
    if (ctx->views != ctx->viewsml)
        PyMem_Free(ctx->views);
    PyMem_Free(ctx);
    Py_XDECREF(data_fast);
    return NULL;
}


static PyObject *
Stream_func_write(Stream *self, PyObject *args)
{
    PyObject *data;
    PyObject *callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O|O:write", &data, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "'callback' must be a callable or None");
        return NULL;
    }

    if (PyObject_CheckBuffer(data)) {
        return pyuv__stream_write_bytes(self, data, callback, NULL);
    } else if (!PyUnicode_Check(data) && PySequence_Check(data)) {
        return pyuv__stream_write_sequence(self, data, callback, NULL);
    } else {
        PyErr_SetString(PyExc_TypeError, "only bytes and sequences are supported");
        return NULL;
    }
}


static PyObject *
Stream_func_fileno(Stream *self)
{
    int err;
    uv_os_fd_t fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_fileno(UV_HANDLE(self), &fd);
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        return NULL;
    }

    /* us_os_fd_t is a HANDLE on Windows which is a 64-bit data type but which
     * is guaranteed to contain only values < 2^24.
     * For more information, see: http://www.viva64.com/en/k/0005/ */
    return PyInt_FromLong((long) fd);
}


static PyObject *
Stream_func_set_blocking(Stream *self, PyObject *args)
{
    int err;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:set_blocking", &PyBool_Type, &enable)) {
        return NULL;
    }

    err = uv_stream_set_blocking((uv_stream_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_STREAM_EXCEPTION(err, UV_HANDLE(self));
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Stream_readable_get(Stream *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyBool_FromLong((long)uv_is_readable((uv_stream_t *)UV_HANDLE(self)));
}


static PyObject *
Stream_writable_get(Stream *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyBool_FromLong((long)uv_is_writable((uv_stream_t *)UV_HANDLE(self)));
}


static PyObject *
Stream_write_queue_size_get(Stream *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyLong_FromSize_t(((uv_stream_t *)UV_HANDLE(self))->write_queue_size);
}


static PyObject *
Stream_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Stream *self = (Stream *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Stream_tp_traverse(Stream *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_read_cb);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Stream_tp_clear(Stream *self)
{
    Py_CLEAR(self->on_read_cb);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
Stream_tp_methods[] = {
    { "shutdown", (PyCFunction)Stream_func_shutdown, METH_VARARGS, "Shutdown the write side of this Stream." },
    { "try_write", (PyCFunction)Stream_func_try_write, METH_VARARGS, "Try to write data on the stream." },
    { "write", (PyCFunction)Stream_func_write, METH_VARARGS, "Write data on the stream." },
    { "start_read", (PyCFunction)Stream_func_start_read, METH_VARARGS, "Start read data from the connected endpoint." },
    { "stop_read", (PyCFunction)Stream_func_stop_read, METH_NOARGS, "Stop read data from the connected endpoint." },
    { "fileno", (PyCFunction)Stream_func_fileno, METH_NOARGS, "Returns the libuv OS handle." },
    { "set_blocking", (PyCFunction)Stream_func_set_blocking, METH_VARARGS, "Set the stream to be blocking." },
    { NULL }
};


static PyGetSetDef Stream_tp_getsets[] = {
    {"readable", (getter)Stream_readable_get, 0, "Indicates if stream is readable.", NULL},
    {"writable", (getter)Stream_writable_get, 0, "Indicates if stream is writable.", NULL},
    {"write_queue_size", (getter)Stream_write_queue_size_get, 0, "Returns the size of the write queue.", NULL},
    {NULL}
};


static PyTypeObject StreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Stream",                                          /*tp_name*/
    sizeof(Stream),                                                /*tp_basicsize*/
    0,                                                             /*tp_itemsize*/
    0,                                                             /*tp_dealloc*/
    0,                                                             /*tp_print*/
    0,                                                             /*tp_getattr*/
    0,                                                             /*tp_setattr*/
    0,                                                             /*tp_compare*/
    0,                                                             /*tp_repr*/
    0,                                                             /*tp_as_number*/
    0,                                                             /*tp_as_sequence*/
    0,                                                             /*tp_as_mapping*/
    0,                                                             /*tp_hash */
    0,                                                             /*tp_call*/
    0,                                                             /*tp_str*/
    0,                                                             /*tp_getattro*/
    0,                                                             /*tp_setattro*/
    0,                                                             /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                       /*tp_flags*/
    0,                                                             /*tp_doc*/
    (traverseproc)Stream_tp_traverse,                              /*tp_traverse*/
    (inquiry)Stream_tp_clear,                                      /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    Stream_tp_methods,                                             /*tp_methods*/
    0,                                                             /*tp_members*/
    Stream_tp_getsets,                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    Stream_tp_new,                                                 /*tp_new*/
};

