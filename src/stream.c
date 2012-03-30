
static PyObject* PyExc_IOStreamError;


typedef struct {
    PyObject *obj;
    PyObject *callback;
    void *data;
} iostream_req_data_t;

typedef struct {
    uv_buf_t *bufs;
    int buf_count;
} iostream_write_data_t;


static uv_buf_t
on_iostream_alloc(uv_stream_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf = uv_buf_init(PyMem_Malloc(suggested_size), suggested_size);
    UNUSED_ARG(handle);
    PyGILState_Release(gstate);
    return buf;
}


static void
on_iostream_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    IOStream *self;
    PyObject *result;
    ASSERT(handle);

    self = (IOStream *)handle->data;
    ASSERT(self);

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_close_cb);
        }
        Py_XDECREF(result);
    }

    handle->data = NULL;
    PyMem_Free(handle);

    /* Refcount was increased in func_close */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_iostream_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


/*
 * Called when all pending write requests have been sent, write direction has been closed.
 */
static void
on_iostream_shutdown(uv_shutdown_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    iostream_req_data_t* req_data;
    uv_err_t err;
    IOStream *self;
    PyObject *callback, *result, *py_errorno;

    req_data = (iostream_req_data_t *)req->data;
    self = (IOStream *)req_data->obj;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (callback != Py_None) {
        if (status < 0) {
            err = uv_last_error(UV_LOOP(self));
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
on_iostream_read(uv_stream_t* handle, int nread, uv_buf_t buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    IOStream *self;
    PyObject *result, *data, *py_errorno;
    ASSERT(handle);

    self = (IOStream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread >= 0) {
        data = PyString_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_iostream_write(uv_write_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    iostream_req_data_t* req_data;
    iostream_write_data_t* write_data;
    IOStream *self;
    PyObject *callback, *result, *py_errorno;
    uv_err_t err;

    ASSERT(req);

    req_data = (iostream_req_data_t *)req->data;
    write_data = (iostream_write_data_t *)req_data->data;
    self = (IOStream *)req_data->obj;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (callback != Py_None) {
        if (status < 0) {
            err = uv_last_error(UV_LOOP(self));
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

    for (i = 0; i < write_data->buf_count; i++) {
        PyMem_Free(write_data->bufs[i].base);
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
IOStream_func_close(IOStream *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:close", &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    Py_INCREF(callback);
    self->on_close_cb = callback;

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    uv_close((uv_handle_t *)self->uv_handle, on_iostream_close);
    self->uv_handle = NULL;

    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_shutdown(IOStream *self, PyObject *args)
{
    int r;
    uv_shutdown_t *req = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:shutdown", &callback)) {
        return NULL;
    }

    req = (uv_shutdown_t*) PyMem_Malloc(sizeof(uv_shutdown_t));
    if (!req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;
    req->data = (void *)req_data;

    r = uv_shutdown(req, self->uv_handle, on_iostream_shutdown);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
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
IOStream_func_start_read(IOStream *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start_read", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_read_start((uv_stream_t *)self->uv_handle, (uv_alloc_cb)on_iostream_alloc, (uv_read_cb)on_iostream_read);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        return NULL;
    }

    tmp = self->on_read_cb;
    Py_INCREF(callback);
    self->on_read_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_stop_read(IOStream *self)
{
    int r;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

    r = uv_read_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_write(IOStream *self, PyObject *args)
{
    int i, r, buf_count;
    char *data_str, *tmp;
    Py_buffer pbuf;
    Py_ssize_t data_len;
    PyObject *callback;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs = NULL;
    uv_write_t *wr = NULL;
    iostream_req_data_t *req_data = NULL;
    iostream_write_data_t *write_data = NULL;

    buf_count = 0;
    callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

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

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (iostream_write_data_t *) PyMem_Malloc(sizeof(iostream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;
    wr->data = (void *)req_data;

    bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t));
    if (!bufs) {
        PyErr_NoMemory();
        goto error;
    }

    data_str = pbuf.buf;
    data_len = pbuf.len;
    tmp = (char *) PyMem_Malloc(data_len);
    if (!tmp) {
        PyErr_NoMemory();
        goto error;
    }
    memcpy(tmp, data_str, data_len);
    tmpbuf = uv_buf_init(tmp, data_len);
    bufs[0] = tmpbuf;
    buf_count = 1;

    write_data->bufs = bufs;
    write_data->buf_count = buf_count;
    req_data->data = (void *)write_data;

    r = uv_write(wr, self->uv_handle, bufs, buf_count, on_iostream_write);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        goto error;
    }

    PyBuffer_Release(&pbuf);

    Py_RETURN_NONE;

error:
    PyBuffer_Release(&pbuf);
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
IOStream_func_writelines(IOStream *self, PyObject *args)
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
    iostream_req_data_t *req_data = NULL;
    iostream_write_data_t *write_data = NULL;

    buf_count = 0;
    bufs = new_bufs = NULL;
    callback = Py_None;
    default_encoding = PyUnicode_GetDefaultEncoding();

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

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

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (iostream_write_data_t *) PyMem_Malloc(sizeof(iostream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)self;
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
            data_str = PyString_AS_STRING(encoded);
            data_len = PyString_GET_SIZE(encoded);
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
    req_data->data = (void *)write_data;

    r = uv_write(wr, self->uv_handle, bufs, buf_count, on_iostream_write);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
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
IOStream_readable_get(IOStream *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!self->uv_handle) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)uv_is_readable(self->uv_handle));
    }
}


static PyObject *
IOStream_writable_get(IOStream *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!self->uv_handle) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)uv_is_writable(self->uv_handle));
    }
}


static PyObject *
IOStream_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    IOStream *self = (IOStream *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
IOStream_tp_traverse(IOStream *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->on_read_cb);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
IOStream_tp_clear(IOStream *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->on_read_cb);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
IOStream_tp_dealloc(IOStream *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_iostream_dealloc_close);
        self->uv_handle = NULL;
    }
    Py_TYPE(self)->tp_clear((PyObject *)self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
IOStream_tp_methods[] = {
    { "shutdown", (PyCFunction)IOStream_func_shutdown, METH_VARARGS, "Shutdown the write side of this IOStream." },
    { "close", (PyCFunction)IOStream_func_close, METH_VARARGS, "Close this IOStream connection." },
    { "write", (PyCFunction)IOStream_func_write, METH_VARARGS, "Write data on the stream." },
    { "writelines", (PyCFunction)IOStream_func_writelines, METH_VARARGS, "Write a sequence of data on the stream." },
    { "start_read", (PyCFunction)IOStream_func_start_read, METH_VARARGS, "Start read data from the connected endpoint." },
    { "stop_read", (PyCFunction)IOStream_func_stop_read, METH_NOARGS, "Stop read data from the connected endpoint." },
    { NULL }
};


static PyMemberDef IOStream_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(IOStream, loop), READONLY, "Loop where this IOStream is running on."},
    {"data", T_OBJECT, offsetof(IOStream, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef IOStream_tp_getsets[] = {
    {"readable", (getter)IOStream_readable_get, 0, "Indicates if stream is readable.", NULL},
    {"writable", (getter)IOStream_writable_get, 0, "Indicates if stream is writable.", NULL},
    {NULL}
};


static PyTypeObject IOStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.IOStream",                                               /*tp_name*/
    sizeof(IOStream),                                              /*tp_basicsize*/
    0,                                                             /*tp_itemsize*/
    (destructor)IOStream_tp_dealloc,                               /*tp_dealloc*/
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
    (traverseproc)IOStream_tp_traverse,                            /*tp_traverse*/
    (inquiry)IOStream_tp_clear,                                    /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    IOStream_tp_methods,                                           /*tp_methods*/
    IOStream_tp_members,                                           /*tp_members*/
    IOStream_tp_getsets,                                           /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    IOStream_tp_new,                                               /*tp_new*/
};

