
static PyObject* PyExc_TCPServerError;
static PyObject* PyExc_TCPConnectionError;


typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    void *data;
} tcp_write_req_t;


/* TCPConnection */


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
        uv_err_t err = uv_last_error(SERVER_LOOP);
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
on_tcp_write(uv_write_t* req, int status)
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


static void
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

    int r = uv_write(&wr->req, self->uv_stream, &wr->buf, 1, on_tcp_write);
    if (r) {
        wr->data = NULL;
        RAISE_ERROR(SERVER_LOOP, PyExc_TCPConnectionError, NULL);
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
    int r = uv_tcp_init(SERVER_LOOP, (uv_tcp_t *)uv_stream);
    if (r) {
        RAISE_ERROR(SERVER_LOOP, PyExc_TCPConnectionError, -1);
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


/* TCPServer */


static void
on_tcp_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(server);

    TCPServer *self = (TCPServer *)(server->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(SELF_LOOP);
        PyErr_SetString(PyExc_TCPServerError, uv_strerror(err));
        PyErr_WriteUnraisable(PyExc_TCPServerError);
    } else {
        PyObject *result;
        result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_new_connection_cb);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_tcp_server_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(handle);
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static PyObject *
TCPServer_func_listen(TCPServer *self, PyObject *args)
{
    int r;
    int backlog = 1;

    if (!PyArg_ParseTuple(args, "i", &backlog)) {
        return NULL;
    }

    if (backlog < 0) {
        PyErr_SetString(PyExc_ValueError, "backlog must be bigger than 0");
        return NULL;
    }

    if (self->address_type == AF_INET) {
        r = uv_tcp_bind(self->uv_tcp_server, uv_ip4_addr(self->listen_ip, self->listen_port));
    } else {
        r = uv_tcp_bind6(self->uv_tcp_server, uv_ip6_addr(self->listen_ip, self->listen_port));
    }
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }

    r = uv_listen((uv_stream_t *)self->uv_tcp_server, backlog, on_tcp_connection);
    if (r) { 
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }

    Py_RETURN_NONE;
}


static PyObject *
TCPServer_func_stop_listening(TCPServer *self)
{
    if (self->uv_tcp_server) {
        self->uv_tcp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_tcp_server, on_tcp_server_close);
        self->uv_tcp_server = NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
TCPServer_func_accept(TCPServer *self, PyObject *args, PyObject *kwargs)
{
    PyObject *read_callback;
    PyObject *write_callback;
    PyObject *close_callback;

    static char *kwlist[] = {"read_cb", "write_cb", "close_cb", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO:accept", kwlist, &read_callback, &write_callback, &close_callback)) {
        return NULL;
    }

    TCPConnection *tcp_connection;
    tcp_connection = (TCPConnection *)PyObject_CallFunction((PyObject *)&TCPConnectionType, "OOOO", self, read_callback, write_callback, close_callback);
    int r = uv_accept((uv_stream_t *)self->uv_tcp_server, tcp_connection->uv_stream);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }
    TCPConnection_start_read(tcp_connection);

    return (PyObject *)tcp_connection;
}


static int
TCPServer_tp_init(TCPServer *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;
    PyObject *callback;
    PyObject *listen_address = NULL;
    char *listen_ip;
    int listen_port;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_tcp_t *uv_tcp_server;

    if (!PyArg_ParseTuple(args, "O!OO:__init__", &LoopType, &loop, &listen_address, &callback)) {
        return -1;
    }

    if (!PyArg_ParseTuple(listen_address, "si", &listen_ip, &listen_port)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    } else {
        tmp = self->on_new_connection_cb;
        Py_INCREF(callback);
        self->on_new_connection_cb = callback;
        Py_XDECREF(tmp);
    }

    if (listen_port < 0 || listen_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return -1;
    }

    if (inet_pton(AF_INET, listen_ip, &addr4) == 1) {
        self->address_type = AF_INET;
    } else if (inet_pton(AF_INET6, listen_ip, &addr6) == 1) {
        self->address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return -1;
    }

    tmp = self->listen_address;
    Py_INCREF(listen_address);
    self->listen_address = listen_address;
    Py_XDECREF(tmp);
    self->listen_ip = listen_ip;
    self->listen_port = listen_port;

    uv_tcp_server = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_tcp_server) {
        PyErr_NoMemory();
        return -1;
    }
    int r = uv_tcp_init(SELF_LOOP, uv_tcp_server);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, -1);
    }
    uv_tcp_server->data = (void *)self;
    self->uv_tcp_server = uv_tcp_server;

    return 0;
}


static PyObject *
TCPServer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPServer *self = (TCPServer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
TCPServer_tp_traverse(TCPServer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->listen_address);
    Py_VISIT(self->on_new_connection_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
TCPServer_tp_clear(TCPServer *self)
{
    Py_CLEAR(self->listen_address);
    Py_CLEAR(self->on_new_connection_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
TCPServer_tp_dealloc(TCPServer *self)
{
    if (self->uv_tcp_server) {
        self->uv_tcp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_tcp_server, on_tcp_server_close);
        self->uv_tcp_server = NULL;
    }
    TCPServer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
TCPServer_tp_methods[] = {
    { "listen", (PyCFunction)TCPServer_func_listen, METH_VARARGS, "Start listening for TCP connections." },
    { "stop_listening", (PyCFunction)TCPServer_func_stop_listening, METH_NOARGS, "Stop listening for TCP connections." },
    { "accept", (PyCFunction)TCPServer_func_accept, METH_VARARGS|METH_KEYWORDS, "Accept incoming TCP connection." },
    { NULL }
};


static PyMemberDef TCPServer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(TCPServer, loop), READONLY, "Loop where this TCPServer is running on."},
    {"listen_address", T_OBJECT_EX, offsetof(TCPServer, listen_address), 0, "Address tuple where this server listens."},
    {NULL}
};


static PyTypeObject TCPServerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCPServer",                                               /*tp_name*/
    sizeof(TCPServer),                                              /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)TCPServer_tp_dealloc,                               /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,      /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)TCPServer_tp_traverse,                            /*tp_traverse*/
    (inquiry)TCPServer_tp_clear,                                    /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    TCPServer_tp_methods,                                           /*tp_methods*/
    TCPServer_tp_members,                                           /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)TCPServer_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    TCPServer_tp_new,                                               /*tp_new*/
};


