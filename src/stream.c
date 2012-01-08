
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
    PyGILState_Release(gstate);
    return buf;
}


static void
on_iostream_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);

    IOStream *self = (IOStream *)handle->data;
    ASSERT(self);

    PyObject *result;

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

    iostream_req_data_t* req_data = (iostream_req_data_t *)req->data;

    IOStream *self = (IOStream *)req_data->obj;
    PyObject *callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result, *py_errorno;

    if (callback != Py_None) {
        if (status < 0) {
            uv_err_t err = uv_last_error(UV_LOOP(self));
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
    }

    Py_DECREF(callback);
    PyMem_Free(req_data);
    req->data = NULL;
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_iostream_read(uv_stream_t* handle, int nread, uv_buf_t buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);

    IOStream *self = (IOStream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result, *data, *py_errorno;

    if (nread >= 0) {
        data = PyString_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        data = Py_None;
        Py_INCREF(Py_None);
        uv_err_t err = uv_last_error(UV_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);

    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_iostream_write(uv_write_t* req, int status)
{
    int i;

    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    iostream_req_data_t* req_data = (iostream_req_data_t *)req->data;
    iostream_write_data_t* write_data = (iostream_write_data_t *)req_data->data;

    IOStream *self = (IOStream *)req_data->obj;
    PyObject *callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result, *py_errorno;

    if (callback != Py_None) {
        if (status < 0) {
            uv_err_t err = uv_last_error(UV_LOOP(self));
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
    }

    for (i = 0; i < write_data->buf_count; i++) {
        PyMem_Free(write_data->bufs[i].base);
    }
    PyMem_Free(write_data->bufs);
    PyMem_Free(write_data);
    Py_DECREF(callback);
    PyMem_Free(req_data);
    req->data = NULL;
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
    int r = 0;
    PyObject *callback = Py_None;
    uv_shutdown_t *req = NULL;
    iostream_req_data_t *req_data = NULL;

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
        req_data->obj = NULL;
        req_data->callback = NULL;
        PyMem_Free(req_data);
    }
    if (req) {
        req->data = NULL;
        PyMem_Free(req);
    }
    return NULL;
}


static PyObject *
IOStream_func_start_read(IOStream *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;

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
IOStream_func_stop_read(IOStream *self, PyObject *args, PyObject *kwargs)
{
    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

    int r = uv_read_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_write(IOStream *self, PyObject *args)
{
    int i, n;
    int r = 0;
    int buf_count = 0;
    char *data_str;
    char *tmp;
    Py_ssize_t data_len;
    PyObject *item;
    PyObject *data;
    PyObject *callback = Py_None;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs = NULL;
    uv_write_t *wr = NULL;
    iostream_req_data_t *req_data = NULL;
    iostream_write_data_t *write_data = NULL;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "IOStream is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O|O:write", &data, &callback)) {
        return NULL;
    }

    if (!PySequence_Check(data) || PyUnicode_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "only strings and iterables are supported");
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

    if (PyString_Check(data)) {
        // We have a single string
        bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t));
        if (!bufs) {
            PyErr_NoMemory();
            goto error;
        }
        data_len = PyString_Size(data);
        data_str = PyString_AsString(data);
        tmp = (char *) PyMem_Malloc(data_len+1);
        if (!tmp) {
            PyMem_Free(bufs);
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, data_str, data_len+1);
        tmpbuf = uv_buf_init(tmp, data_len);
        bufs[0] = tmpbuf;
        buf_count = 1;
    } else {
        // We have a list
        buf_count = 0;
        n = PySequence_Length(data);
        bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t) * n);
        if (!bufs) {
            PyErr_NoMemory();
            goto error;
        }
        for (i = 0;i < n; i++) {
            item = PySequence_GetItem(data, i);
            if (!item || !PyString_Check(item))
                continue;
            data_len = PyString_Size(item);
            data_str = PyString_AsString(item);
            tmp = (char *) PyMem_Malloc(data_len + 1);
            if (!tmp)
                continue;
            memcpy(tmp, data_str, data_len+1);
            tmpbuf = uv_buf_init(tmp, data_len);
            bufs[i] = tmpbuf;
            buf_count++;
        }

    }

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
        req_data->obj = NULL;
        req_data->callback = NULL;
        req_data->data = NULL;
        PyMem_Free(req_data);
    }
    if (wr) {
        wr->data = NULL;
        PyMem_Free(wr);
    }
    return NULL;
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
    { "write", (PyCFunction)IOStream_func_write, METH_VARARGS, "Write data-" },
    { "start_read", (PyCFunction)IOStream_func_start_read, METH_VARARGS, "Start read data from the connected endpoint." },
    { "stop_read", (PyCFunction)IOStream_func_stop_read, METH_NOARGS, "Stop read data from the connected endpoint." },
    { NULL }
};


static PyMemberDef IOStream_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(IOStream, loop), READONLY, "Loop where this IOStream is running on."},
    {"data", T_OBJECT, offsetof(IOStream, data), 0, "Arbitrary data."},
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
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    IOStream_tp_new,                                               /*tp_new*/
};

