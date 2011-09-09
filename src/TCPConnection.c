
static PyObject* PyExc_TCPConnectionError;

#define TCPCONNECTION_LOOP self->server->loop->uv_loop

#define TCPCONNECTION_ERROR()                                           \
    do {                                                                \
        uv_err_t err = uv_last_error(TCPCONNECTION_LOOP);               \
        PyErr_SetString(PyExc_TCPConnectionError, uv_strerror(err));    \
        return NULL;                                                    \
    } while (0)                                                         \

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    void *data;
} tcp_write_req_t;


static uv_buf_t
on_tcp_alloc(uv_tcp_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf;
    buf.base = PyMem_Malloc(suggested_size);
    buf.len = suggested_size;
    PyGILState_Release(gstate);
    return buf;
}


static void
on_tcp_connection_closed(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(handle);
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_tcp_shutdown(uv_shutdown_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    TCPConnection *self = (TCPConnection *)((uv_handle_t*)req->handle->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_close_cb);
    }
    Py_XDECREF(result);

    self->uv_stream->data = NULL;
    uv_close((uv_handle_t*)req->handle, on_tcp_connection_closed);
    self->uv_stream = NULL;
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_tcp_read(uv_tcp_t* handle, int nread, uv_buf_t buf)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(handle);

    TCPConnection *self = (TCPConnection *)(handle->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);
   
    if (nread > 0) {
        PyObject *data = PyString_FromStringAndSize(buf.base, nread);
        PyObject *result;
        result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_read_cb);
        }
        Py_XDECREF(result);
    } else if (nread < 0) { 
        uv_err_t err = uv_last_error(TCPCONNECTION_LOOP);
        assert(err.code == UV_EOF);
        UNUSED_ARG(err);
        uv_shutdown_t* req = (uv_shutdown_t*) PyMem_Malloc(sizeof *req);
        if (!req) {
            PyErr_NoMemory();
            PyErr_WriteUnraisable(self->on_read_cb);
        } else {
            uv_shutdown(req, (uv_stream_t *)handle, on_tcp_shutdown);
        }
    }
    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_write(uv_write_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(req);
    assert(status == 0);

    tcp_write_req_t* wr = (tcp_write_req_t*) req;
    TCPConnection *self = (TCPConnection *)(wr->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);
  
    PyObject *result;
    result = PyObject_CallFunctionObjArgs(self->on_write_cb, self, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);

    wr->data = NULL;
    PyMem_Free(wr);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


/* Called by the TCPServer after accepting the connection */
void
TCPConnection_start_read(TCPConnection *self)
{
    uv_read_start(self->uv_stream, (uv_alloc_cb)on_tcp_alloc, (uv_read_cb)on_tcp_read);
}


static PyObject *
TCPConnection_func_close(TCPConnection *self)
{
    if (self->uv_stream) {
        self->uv_stream->data = NULL;
        uv_read_stop(self->uv_stream);
        uv_close((uv_handle_t *)self->uv_stream, on_tcp_connection_closed);
        self->uv_stream = NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
TCPConnection_func_write(TCPConnection *self, PyObject *args)
{
    tcp_write_req_t *wr;
    char *write_data;

    if (!PyArg_ParseTuple(args, "s:write", &write_data)) {
        return NULL;
    }

    wr = (tcp_write_req_t*) PyMem_Malloc(sizeof *wr);
    if (!wr) {
        return PyErr_NoMemory();
    }
    wr->buf.base = write_data;
    wr->buf.len = strlen(write_data);
    wr->data = (void *)self;

    int r = uv_write(&wr->req, self->uv_stream, &wr->buf, 1, on_write);
    if (r) {
        wr->data = NULL;
        TCPCONNECTION_ERROR();
    }

    Py_RETURN_NONE;
}


static int
TCPConnection_tp_init(TCPConnection *self, PyObject *args, PyObject *kwargs)
{
    PyObject *read_callback;
    PyObject *write_callback;
    PyObject *close_callback;
    PyObject *tmp = NULL;
    TCPServer *server;
    uv_stream_t *uv_stream;

    static char *kwlist[] = {"server", "read_cb", "write_cb", "close_cb", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!OOO:__init__", kwlist, &TCPServerType, &server, &read_callback, &write_callback, &close_callback)) {
        return -1;
    }

    tmp = (PyObject *)self->server;
    Py_INCREF(server);
    self->server = server;
    Py_XDECREF(tmp);

    if (!PyCallable_Check(read_callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    } else {
        tmp = self->on_read_cb;
        Py_INCREF(read_callback);
        self->on_read_cb = read_callback;
        Py_XDECREF(tmp);
    }

    if (!PyCallable_Check(write_callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    } else {
        tmp = self->on_write_cb;
        Py_INCREF(write_callback);
        self->on_write_cb = write_callback;
        Py_XDECREF(tmp);
    }

    if (!PyCallable_Check(close_callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    } else {
        tmp = self->on_close_cb;
        Py_INCREF(close_callback);
        self->on_close_cb = close_callback;
        Py_XDECREF(tmp);
    }

    uv_stream = PyMem_Malloc(sizeof(uv_stream_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        return -1;
    }
    int r = uv_tcp_init(TCPCONNECTION_LOOP, (uv_tcp_t *)uv_stream);
    if (r) {
        uv_err_t err = uv_last_error(TCPCONNECTION_LOOP);
        PyErr_SetString(PyExc_TCPConnectionError, uv_strerror(err));
        return -1;
    }
    uv_stream->data = (void *)self;
    self->uv_stream = uv_stream;
    return 0;
}


static PyObject *
TCPConnection_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPConnection *self = (TCPConnection *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
TCPConnection_tp_traverse(TCPConnection *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_read_cb);
    Py_VISIT(self->on_write_cb);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->server);
    return 0;
}


static int
TCPConnection_tp_clear(TCPConnection *self)
{
    Py_CLEAR(self->on_read_cb);
    Py_CLEAR(self->on_write_cb);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->server);
    return 0;
}


static void
TCPConnection_tp_dealloc(TCPConnection *self)
{
    if (self->uv_stream) {
        self->uv_stream->data = NULL;
        uv_read_stop(self->uv_stream);
        uv_close((uv_handle_t *)self->uv_stream, on_tcp_connection_closed);
        self->uv_stream = NULL;
    }
    TCPConnection_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
TCPConnection_tp_methods[] = {
    { "write", (PyCFunction)TCPConnection_func_write, METH_VARARGS, "Write data on this TCP connection." },
    { "close", (PyCFunction)TCPConnection_func_close, METH_NOARGS, "Close this TCP connection." },
    { NULL }
};


static PyMemberDef TCPConnection_tp_members[] = {
    {"server", T_OBJECT_EX, offsetof(TCPConnection, server), READONLY, "Reference to the TCPServer instance where this connection is made."},
    {NULL}
};


static PyTypeObject TCPConnectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCPConnection",                                           /*tp_name*/
    sizeof(TCPConnection),                                          /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)TCPConnection_tp_dealloc,                           /*tp_dealloc*/
    0,                                                              /*tp_print*/
    0,                                                              /*tp_getattr*/
    0,                                                              /*tp_setattr*/
    0,                                                              /*tp_compare*/
    0,                                                              /*tp_repr*/
    0,                                                              /*tp_as_number*/
    0,                                                              /*tp_as_sequence*/
    0,                                                              /*tp_as_mapping*/
    0,                                                              /*tp_hash */
    0,                                                              /*tp_call*/
    0,                                                              /*tp_str*/
    0,                                                              /*tp_getattro*/
    0,                                                              /*tp_setattro*/
    0,                                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,                          /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)TCPConnection_tp_traverse,                        /*tp_traverse*/
    (inquiry)TCPConnection_tp_clear,                                /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    TCPConnection_tp_methods,                                       /*tp_methods*/
    TCPConnection_tp_members,                                       /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)TCPConnection_tp_init,                                /*tp_init*/
    0,                                                              /*tp_alloc*/
    TCPConnection_tp_new,                                           /*tp_new*/
};


