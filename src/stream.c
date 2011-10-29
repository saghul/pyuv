
static PyObject* PyExc_IOStreamError;


typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    void *data;
} iostream_write_req_t;

typedef struct {
    PyObject *obj;
    PyObject *callback;
} iostream_req_data_t;


static uv_buf_t
on_iostream_alloc(uv_stream_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf;
    buf.base = PyMem_Malloc(suggested_size);
    buf.len = suggested_size;
    PyGILState_Release(gstate);
    return buf;
}


static void
on_iostream_close(uv_handle_t *handle)
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

    IOStream *self = (IOStream *)(((uv_handle_t*)req->handle)->data);
    ASSERT(self);

    uv_close((uv_handle_t*)req->handle, on_iostream_close);

    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
on_iostream_read(uv_tcp_t* handle, int nread, uv_buf_t buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);

    IOStream *self = (IOStream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *data;
    PyObject *result;

    if (nread >= 0) {
        data = PyString_FromStringAndSize(buf.base, nread);
    } else if (nread < 0) {
        uv_err_t err = uv_last_error(SELF_LOOP);
        ASSERT(err.code == UV_EOF);
        UNUSED_ARG(err);
        data = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, NULL);
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
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    iostream_write_req_t* wr = (iostream_write_req_t*) req;
    iostream_req_data_t* req_data = (iostream_req_data_t *)wr->data;

    IOStream *self = (IOStream *)req_data->obj;
    PyObject *callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result;

    if (callback != Py_None) {
        result = PyObject_CallFunctionObjArgs(callback, self, PyInt_FromLong(status), NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(callback);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(callback);
    PyMem_Free(req_data);
    wr->data = NULL;
    PyMem_Free(wr);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
IOStream_func_close(IOStream *self)
{
    self->connected = False;
    if (self->uv_stream != NULL) {
        uv_close((uv_handle_t *)self->uv_stream, on_iostream_close);
    }
    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_disconnect(IOStream *self)
{
    if (!self->connected) {
        PyErr_SetString(PyExc_IOStreamError, "already disconnected");
        return NULL;
    }

    self->connected = False;
    uv_read_stop(self->uv_stream);
    uv_shutdown_t* req = (uv_shutdown_t*) PyMem_Malloc(sizeof *req);
    if (!req) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(self->on_read_cb);
    } else {
        uv_shutdown(req, self->uv_stream, on_iostream_shutdown);
    }

    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_start_reading(IOStream *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;

    if (!self->connected) {
        PyErr_SetString(PyExc_IOStreamError, "disconnected");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start_reading", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    } else {
        tmp = self->on_read_cb;
        Py_INCREF(callback);
        self->on_read_cb = callback;
        Py_XDECREF(tmp);
    }

    r = uv_read_start((uv_stream_t *)self->uv_stream, (uv_alloc_cb)on_iostream_alloc, (uv_read_cb)on_iostream_read);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        goto error;
    }
    Py_RETURN_NONE;
error:
    Py_DECREF(callback);
    return NULL;
}


static PyObject *
IOStream_func_stop_reading(IOStream *self, PyObject *args, PyObject *kwargs)
{
    if (!self->connected) {
        PyErr_SetString(PyExc_IOStreamError, "disconnected");
        return NULL;
    }

    int r = uv_read_stop(self->uv_stream);
    if (r) {
        raise_uv_exception(self->loop, PyExc_IOStreamError);
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
IOStream_func_write(IOStream *self, PyObject *args)
{
    char *data;
    int r = 0;
    iostream_write_req_t *wr = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *callback = Py_None;

    if (!PyArg_ParseTuple(args, "s|O:write", &data, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    wr = (iostream_write_req_t*) PyMem_Malloc(sizeof *wr);
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof *req_data);
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    wr->buf.base = data;
    wr->buf.len = strlen(data);

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;

    wr->data = (void *)req_data;

    r = uv_write(&wr->req, self->uv_stream, &wr->buf, 1, on_iostream_write);
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
    if (wr) {
        wr->data = NULL;
        PyMem_Free(wr);
    }
    return NULL;
}


static int
IOStream_tp_init(IOStream *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_IOStreamError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    self->initialized = True;
    self->connected = False;
    self->uv_stream = NULL;

    return 0;
}


static PyObject *
IOStream_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    IOStream *self = (IOStream *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static int
IOStream_tp_traverse(IOStream *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_disconnect_cb);
    Py_VISIT(self->on_read_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
IOStream_tp_clear(IOStream *self)
{
    Py_CLEAR(self->on_disconnect_cb);
    Py_CLEAR(self->on_read_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
IOStream_tp_dealloc(IOStream *self)
{
    IOStream_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
IOStream_tp_methods[] = {
    { "disconnect", (PyCFunction)IOStream_func_disconnect, METH_NOARGS, "Disconnect this IOStream." },
    { "close", (PyCFunction)IOStream_func_close, METH_NOARGS, "Abruptly stop this IOStream connection." },
    { "write", (PyCFunction)IOStream_func_write, METH_VARARGS, "Write data-" },
    { "start_reading", (PyCFunction)IOStream_func_start_reading, METH_VARARGS, "Start reading data from the connected endpoint." },
    { "stop_reading", (PyCFunction)IOStream_func_stop_reading, METH_NOARGS, "Stop reading data from the connected endpoint." },
    { NULL }
};


static PyMemberDef IOStream_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(IOStream, loop), READONLY, "Loop where this IOStream is running on."},
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
    (initproc)IOStream_tp_init,                                    /*tp_init*/
    0,                                                             /*tp_alloc*/
    IOStream_tp_new,                                               /*tp_new*/
};

