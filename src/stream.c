
typedef struct {
    PyObject *callback;
    PyObject *send_handle;
    Py_buffer *views;
    int view_count;
} stream_write_data_t;


static uv_buf_t
on_stream_alloc(uv_stream_t* handle, size_t suggested_size)
{
    static char slab[PYUV_SLAB_SIZE];
    UNUSED_ARG(handle);
    return uv_buf_init(slab, sizeof(slab));
}


static void
on_stream_shutdown(uv_shutdown_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    Stream *self;
    PyObject *callback, *result, *py_errorno;

    self = (Stream *)req->handle->data;
    callback = (PyObject *)req->data;

    ASSERT(self);

    if (callback != Py_None) {
        if (status < 0) {
            err = uv_last_error(UV_HANDLE_LOOP(self));
            py_errorno = PyInt_FromLong((long)err.code);
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
on_stream_read(uv_stream_t* handle, int nread, uv_buf_t buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    Stream *self;
    PyObject *result, *data, *py_errorno;
    ASSERT(handle);

    self = (Stream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread >= 0) {
        data = PyBytes_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
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

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_stream_write(uv_write_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    stream_write_data_t* req_data;
    Stream *self;
    PyObject *callback, *send_handle, *result, *py_errorno;
    uv_err_t err;

    ASSERT(req);

    req_data = (stream_write_data_t *)req->data;
    self = (Stream *)req->handle->data;
    callback = req_data->callback;
    send_handle = req_data->send_handle;

    ASSERT(self);

    if (callback != Py_None) {
        if (status < 0) {
            err = uv_last_error(UV_HANDLE_LOOP(self));
            py_errorno = PyInt_FromLong((long)err.code);
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
    for (i = 0; i < req_data->view_count; i++) {
        PyBuffer_Release(&req_data->views[i]);
    }
    PyMem_Free(req_data->views);
    PyMem_Free(req_data);
    PyMem_Free(req);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
Stream_func_shutdown(Stream *self, PyObject *args)
{
    int r;
    uv_shutdown_t *req = NULL;
    PyObject *callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "|O:shutdown", &callback)) {
        return NULL;
    }

    Py_INCREF(callback);

    req = PyMem_Malloc(sizeof *req);
    if (!req) {
        PyErr_NoMemory();
        goto error;
    }

    req->data = (void *)callback;

    r = uv_shutdown(req, (uv_stream_t *)UV_HANDLE(self), on_stream_shutdown);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    PyMem_Free(req);
    return NULL;
}


static PyObject *
Stream_func_start_read(Stream *self, PyObject *args)
{
    int r;
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

    r = uv_read_start((uv_stream_t *)UV_HANDLE(self), (uv_alloc_cb)on_stream_alloc, (uv_read_cb)on_stream_read);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        return NULL;
    }

    tmp = self->on_read_cb;
    Py_INCREF(callback);
    self->on_read_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Stream_func_stop_read(Stream *self)
{
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_read_stop((uv_stream_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        return NULL;
    }

    Py_XDECREF(self->on_read_cb);
    self->on_read_cb = NULL;

    Py_RETURN_NONE;
}


static INLINE PyObject *
pyuv_stream_write(Stream *self, Py_buffer *views, uv_buf_t *bufs, int buf_count, PyObject *callback, PyObject *send_handle)
{
    int i, r;
    uv_write_t *wr = NULL;
    stream_write_data_t *req_data = NULL;

    Py_INCREF(callback);
    Py_XINCREF(send_handle);

    wr = PyMem_Malloc(sizeof *wr);
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof *req_data);
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->callback = callback;
    req_data->send_handle = send_handle;
    req_data->views = views;
    req_data->view_count = buf_count;

    wr->data = (void *)req_data;

    if (send_handle) {
        r = uv_write2(wr, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, (uv_stream_t *)UV_HANDLE(send_handle), on_stream_write);
    } else {
        r = uv_write(wr, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, on_stream_write);
    }

    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    Py_XDECREF(send_handle);
    for (i = 0; i < buf_count; i++) {
        PyBuffer_Release(&views[i]);
    }
    PyMem_Free(views);
    PyMem_Free(req_data);
    PyMem_Free(wr);
    return NULL;
}


static PyObject *
Stream_func_write(Stream *self, PyObject *args)
{
    uv_buf_t buf;
    Py_buffer *view;
    PyObject *callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    view = PyMem_Malloc(sizeof *view);
    if (!view) {
        PyErr_NoMemory();
        return NULL;
    }

#ifdef PYUV_PYTHON3
    if (!PyArg_ParseTuple(args, "y*|O:write", view, &callback)) {
#else
    if (!PyArg_ParseTuple(args, "s*|O:write", view, &callback)) {
#endif
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyBuffer_Release(view);
        PyMem_Free(view);
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    buf = uv_buf_init(view->buf, view->len);

    return pyuv_stream_write(self, view, &buf, 1, callback, NULL);
}


static PyObject *
Stream_func_writelines(Stream *self, PyObject *args)
{
    int r, buf_count;
    Py_buffer *views;
    PyObject *callback, *seq, *ret;
    uv_buf_t *bufs;

    callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O|O:writelines", &seq, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    r = pyseq2uvbuf(seq, &views, &bufs, &buf_count);
    if (r != 0) {
        /* error is already set */
        return NULL;
    }

    ret = pyuv_stream_write(self, views, bufs, buf_count, callback, NULL);

    /* uv_write copies the uv_buf_t structures, so we can free them now */
    PyMem_Free(bufs);

    return ret;
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
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Stream_tp_clear(Stream *self)
{
    Py_CLEAR(self->on_read_cb);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Stream_tp_methods[] = {
    { "shutdown", (PyCFunction)Stream_func_shutdown, METH_VARARGS, "Shutdown the write side of this Stream." },
    { "write", (PyCFunction)Stream_func_write, METH_VARARGS, "Write data on the stream." },
    { "writelines", (PyCFunction)Stream_func_writelines, METH_VARARGS, "Write a sequence of data on the stream." },
    { "start_read", (PyCFunction)Stream_func_start_read, METH_VARARGS, "Start read data from the connected endpoint." },
    { "stop_read", (PyCFunction)Stream_func_stop_read, METH_NOARGS, "Stop read data from the connected endpoint." },
    { NULL }
};


static PyGetSetDef Stream_tp_getsets[] = {
    {"readable", (getter)Stream_readable_get, 0, "Indicates if stream is readable.", NULL},
    {"writable", (getter)Stream_writable_get, 0, "Indicates if stream is writable.", NULL},
    {NULL}
};


static PyTypeObject StreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Stream",                                                 /*tp_name*/
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

