
static PyObject* PyExc_StreamError;


typedef struct {
    uv_buf_t *bufs;
    int buf_count;
    Py_buffer view;
} stream_write_data_t;

typedef struct {
    PyObject *callback;
    stream_write_data_t *data;
} stream_req_data_t;


static uv_buf_t
on_stream_alloc(uv_stream_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf = uv_buf_init(PyMem_Malloc(suggested_size), suggested_size);
    UNUSED_ARG(handle);
    PyGILState_Release(gstate);
    return buf;
}


static void
on_stream_shutdown(uv_shutdown_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    stream_req_data_t* req_data;
    uv_err_t err;
    Stream *self;
    PyObject *callback, *result, *py_errorno;

    req_data = (stream_req_data_t *)req->data;
    self = (Stream *)req->handle->data;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

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
            PyErr_WriteUnraisable(callback);
        }
        Py_XDECREF(result);
        Py_DECREF(py_errorno);
    }

    Py_DECREF(callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

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
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

    /* In case of error libuv may not call alloc_cb */
    if (buf.base != NULL) {
        PyMem_Free(buf.base);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_stream_write(uv_write_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    stream_req_data_t* req_data;
    stream_write_data_t* write_data;
    Stream *self;
    PyObject *callback, *result, *py_errorno;
    uv_err_t err;

    ASSERT(req);

    req_data = (stream_req_data_t *)req->data;
    write_data = req_data->data;
    self = (Stream *)req->handle->data;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

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
            PyErr_WriteUnraisable(callback);
        }
        Py_XDECREF(result);
        Py_DECREF(py_errorno);
    }

    if (write_data->buf_count == 1) {
        PyBuffer_Release(&write_data->view);
    } else {
        for (i = 0; i < write_data->buf_count; i++) {
            PyMem_Free(write_data->bufs[i].base);
        }
    }
    PyMem_Free(write_data->bufs);
    PyMem_Free(write_data);
    Py_DECREF(callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Stream_func_shutdown(Stream *self, PyObject *args)
{
    int r;
    uv_shutdown_t *req = NULL;
    stream_req_data_t *req_data = NULL;
    PyObject *callback = Py_None;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "|O:shutdown", &callback)) {
        return NULL;
    }

    req = (uv_shutdown_t*) PyMem_Malloc(sizeof(uv_shutdown_t));
    if (!req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (stream_req_data_t*) PyMem_Malloc(sizeof(stream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    req_data->callback = callback;
    req->data = (void *)req_data;

    r = uv_shutdown(req, (uv_stream_t *)UV_HANDLE(self), on_stream_shutdown);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (req_data) {
        Py_DECREF(callback);
        PyMem_Free(req_data);
    }
    if (req) {
        PyMem_Free(req);
    }
    return NULL;
}


static PyObject *
Stream_func_start_read(Stream *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

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


static PyObject *
Stream_func_write(Stream *self, PyObject *args)
{
    int r, buf_count;
    Py_buffer pbuf;
    PyObject *callback;
    uv_buf_t *bufs = NULL;
    uv_write_t *wr = NULL;
    stream_req_data_t *req_data = NULL;
    stream_write_data_t *write_data = NULL;

    buf_count = 0;
    callback = Py_None;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "s*|O:write", &pbuf, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    wr = (uv_write_t *)PyMem_Malloc(sizeof(uv_write_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (stream_req_data_t*) PyMem_Malloc(sizeof(stream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (stream_write_data_t *) PyMem_Malloc(sizeof(stream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    req_data->callback = callback;
    wr->data = (void *)req_data;

    bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t));
    if (!bufs) {
        PyErr_NoMemory();
        goto error;
    }

    bufs[0] = uv_buf_init(pbuf.buf, pbuf.len);
    buf_count = 1;

    write_data->bufs = bufs;
    write_data->buf_count = buf_count;
    write_data->view = pbuf;
    req_data->data = write_data;

    r = uv_write(wr, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, on_stream_write);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    PyBuffer_Release(&pbuf);
    if (bufs) {
        PyMem_Free(bufs);
    }
    if (write_data) {
        PyMem_Free(write_data);
    }
    if (req_data) {
        Py_DECREF(callback);
        PyMem_Free(req_data);
    }
    if (wr) {
        PyMem_Free(wr);
    }
    return NULL;
}


static Py_ssize_t
iter_guess_size(PyObject *o, Py_ssize_t defaultvalue)
{
    PyObject *ro;
    Py_ssize_t rv;

    /* try o.__length_hint__() */
    ro = PyObject_CallMethod(o, "__length_hint__", NULL);
    if (ro == NULL) {
        /* whatever the error is, clear it and return the default */
        PyErr_Clear();
        return defaultvalue;
    }
    rv = PyLong_Check(ro) ? PyLong_AsSsize_t(ro) : defaultvalue;
    Py_DECREF(ro);
    return rv;
}

static PyObject *
Stream_func_writelines(Stream *self, PyObject *args)
{
    int i, r, buf_count;
    char *data_str, *tmp;
    const char *default_encoding;
    Py_buffer pbuf;
    Py_ssize_t data_len, n;
    PyObject *callback, *seq, *iter, *item, *encoded;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs, *new_bufs;
    uv_write_t *wr = NULL;
    stream_req_data_t *req_data = NULL;
    stream_write_data_t *write_data = NULL;

    buf_count = 0;
    bufs = new_bufs = NULL;
    callback = Py_None;
    default_encoding = PyUnicode_GetDefaultEncoding();

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O|O:writelines", &seq, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    wr = (uv_write_t *)PyMem_Malloc(sizeof(uv_write_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (stream_req_data_t*) PyMem_Malloc(sizeof(stream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (stream_write_data_t *) PyMem_Malloc(sizeof(stream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    req_data->callback = callback;
    wr->data = (void *)req_data;

    iter = PyObject_GetIter(seq);
    if (iter == NULL) {
        goto error;
    }

    n = iter_guess_size(iter, 8);   /* if we can't get the size hint, preallocate 8 slots */
    bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t) * n);
    if (!bufs) {
        Py_DECREF(iter);
        PyErr_NoMemory();
        goto error;
    }

    i = 0;
    while (1) {
        item = PyIter_Next(iter);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(iter);
                goto error;
            } else {
                /* StopIteration */
                break;
            }
        }

        if (PyUnicode_Check(item)) {
            encoded = PyUnicode_AsEncodedString(item, default_encoding, "strict");
            if (encoded == NULL) {
                Py_DECREF(item);
                Py_DECREF(iter);
                goto error;
            }
            data_str = PyBytes_AS_STRING(encoded);
            data_len = PyBytes_GET_SIZE(encoded);
            tmp = (char *) PyMem_Malloc(data_len);
            if (!tmp) {
                Py_DECREF(item);
                Py_DECREF(iter);
                PyErr_NoMemory();
                goto error;
            }
            memcpy(tmp, data_str, data_len);
            tmpbuf = uv_buf_init(tmp, data_len);
        } else {
            if (PyObject_GetBuffer(item, &pbuf, PyBUF_CONTIG_RO) < 0) {
                Py_DECREF(item);
                Py_DECREF(iter);
                goto error;
            }
            data_str = pbuf.buf;
            data_len = pbuf.len;
            tmp = (char *) PyMem_Malloc(data_len);
            if (!tmp) {
                Py_DECREF(item);
                Py_DECREF(iter);
                PyBuffer_Release(&pbuf);
                PyErr_NoMemory();
                goto error;
            }
            memcpy(tmp, data_str, data_len);
            tmpbuf = uv_buf_init(tmp, data_len);
            PyBuffer_Release(&pbuf);
        }

        /* Check if we allocated enough space */
        if (buf_count+1 < n) {
            /* we have enough size */
        } else {
            /* preallocate 8 more slots */
            n += 8;
            new_bufs = (uv_buf_t *) PyMem_Realloc(bufs, sizeof(uv_buf_t) * n);
            if (!new_bufs) {
                Py_DECREF(item);
                Py_DECREF(iter);
                PyErr_NoMemory();
                goto error;
            }
            bufs = new_bufs;
        }
        bufs[i] = tmpbuf;
        i++;
        buf_count++;
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    /* we may have over allocated space, shrink it to the minimum required */
    new_bufs = (uv_buf_t *) PyMem_Realloc(bufs, sizeof(uv_buf_t) * buf_count);
    if (!new_bufs) {
        PyErr_NoMemory();
        goto error;
    }
    bufs = new_bufs;

    write_data->bufs = bufs;
    write_data->buf_count = buf_count;
    req_data->data = write_data;

    r = uv_write(wr, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, on_stream_write);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_StreamError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (bufs) {
        for (i = 0; i < buf_count; i++) {
            PyMem_Free(bufs[i].base);
        }
        PyMem_Free(bufs);
    }
    if (write_data) {
        PyMem_Free(write_data);
    }
    if (req_data) {
        Py_DECREF(callback);
        PyMem_Free(req_data);
    }
    if (wr) {
        PyMem_Free(wr);
    }
    return NULL;
}


static PyObject *
Stream_readable_get(Stream *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!UV_HANDLE(self)) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)uv_is_readable((uv_stream_t *)UV_HANDLE(self)));
    }
}


static PyObject *
Stream_writable_get(Stream *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!UV_HANDLE(self)) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)uv_is_writable((uv_stream_t *)UV_HANDLE(self)));
    }
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

