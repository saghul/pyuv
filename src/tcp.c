
static PyObject* PyExc_TCPClientError;
static PyObject* PyExc_TCPClientConnectionError;
static PyObject* PyExc_TCPServerError;


/* TCPServer */

static void
on_tcp_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(server);

    TCPServer *self = (TCPServer *)(server->data);
    ASSERT(self);
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
    ASSERT(handle);
    /* Decrement reference count of the object this handle was keeping alive */
    PyObject *obj = (PyObject *)handle->data;
    Py_DECREF(obj);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static PyObject *
TCPServer_func_bind(TCPServer *self, PyObject *args)
{
    int r = 0;
    char *bind_ip;
    int bind_port;
    int address_type;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_tcp_t *uv_tcp_server = NULL;

    if (self->bound) {
        PyErr_SetString(PyExc_TCPServerError, "already bound!");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "(si):bind", &bind_ip, &bind_port)) {
        return NULL;
    }

    if (bind_port < 0 || bind_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return NULL;
    }

    if (inet_pton(AF_INET, bind_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (inet_pton(AF_INET6, bind_ip, &addr6) == 1) {
        address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    uv_tcp_server = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_tcp_server) {
        PyErr_NoMemory();
        goto error;
    }
    r = uv_tcp_init(SELF_LOOP, uv_tcp_server);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        goto error;
    }
    uv_tcp_server->data = (void *)self;
    self->uv_tcp_server = uv_tcp_server;

    if (address_type == AF_INET) {
        r = uv_tcp_bind(self->uv_tcp_server, uv_ip4_addr(bind_ip, bind_port));
    } else {
        r = uv_tcp_bind6(self->uv_tcp_server, uv_ip6_addr(bind_ip, bind_port));
    }

    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        return NULL;
    }

    self->bound = True;

    /* Increment reference count while libuv keeps this object around. It'll be decremented on handle close. */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    if (uv_tcp_server) {
        uv_tcp_server->data = NULL;
        PyMem_Free(uv_tcp_server);
    }
    self->uv_tcp_server = NULL;
    return NULL;
}


static PyObject *
TCPServer_func_listen(TCPServer *self, PyObject *args)
{
    int r;
    int backlog = 128;
    PyObject *callback;
    PyObject *tmp = NULL;

    if (!self->bound) {
        PyErr_SetString(PyExc_TCPServerError, "not bound");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O|i:listen", &callback, &backlog)) {
        return NULL;
    }

    if (backlog < 0) {
        PyErr_SetString(PyExc_ValueError, "backlog must be bigger than 0");
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    } else {
        tmp = self->on_new_connection_cb;
        Py_INCREF(callback);
        self->on_new_connection_cb = callback;
        Py_XDECREF(tmp);
    }

    r = uv_listen((uv_stream_t *)self->uv_tcp_server, backlog, on_tcp_connection);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    return NULL;
}


static PyObject *
TCPServer_func_close(TCPServer *self)
{
    if (!self->bound) {
        PyErr_SetString(PyExc_TCPServerError, "not bound");
        return NULL;
    }

    self->bound = False;
    uv_close((uv_handle_t *)self->uv_tcp_server, on_tcp_server_close);

    Py_RETURN_NONE;
}


static PyObject *
TCPServer_func_accept(TCPServer *self)
{
    int r = 0;
    uv_tcp_t *uv_stream = NULL;

    if (!self->bound) {
        PyErr_SetString(PyExc_TCPServerError, "not bound");
        return NULL;
    }

    TCPClientConnection *connection;
    connection = (TCPClientConnection *)PyObject_CallFunction((PyObject *)&TCPClientConnectionType, "O", self->loop);

    uv_stream = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        goto error;
    }

    r = uv_tcp_init(((IOStream *)connection)->loop->uv_loop, uv_stream);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        goto error;
    }

    uv_stream->data = (void *)connection;
    ((IOStream *)connection)->uv_stream = (uv_stream_t *)uv_stream;

    r = uv_accept((uv_stream_t *)self->uv_tcp_server, ((IOStream *)connection)->uv_stream);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        goto error;
    }
    ((IOStream *)connection)->connected = True;

    /* Increment reference count while libuv keeps this object around. It'll be decremented on handle close. */
    Py_INCREF(connection);

    return (PyObject *)connection;

error:
    if (uv_stream) {
        uv_stream->data = NULL;
        PyMem_Free(uv_stream);
    }
    return NULL;
}


static PyObject *
TCPServer_func_getsockname(TCPServer *self)
{
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(sockname);

    if (!self->bound) {
        PyErr_SetString(PyExc_TCPServerError, "not bound");
        return NULL;
    }

    int r = uv_tcp_getsockname(self->uv_tcp_server, &sockname, &namelen);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TCPServerError);
        return NULL;
    }

    if (sockname.sa_family == AF_INET) {
        addr4 = (struct sockaddr_in*)&sockname;
        r = uv_ip4_name(addr4, ip4, INET_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip4, ntohs(addr4->sin_port));
    } else if (sockname.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&sockname;
        r = uv_ip6_name(addr6, ip6, INET6_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip6, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_TCPServerError, "unknown address type detected");
        return NULL;
    }
}


static int
TCPServer_tp_init(TCPServer *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_TCPServerError, "Object already initialized");
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
    self->bound = False;
    self->uv_tcp_server = NULL;

    return 0;
}


static PyObject *
TCPServer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPServer *self = (TCPServer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static int
TCPServer_tp_traverse(TCPServer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
TCPServer_tp_clear(TCPServer *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
TCPServer_tp_dealloc(TCPServer *self)
{
    TCPServer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
TCPServer_tp_methods[] = {
    { "bind", (PyCFunction)TCPServer_func_bind, METH_VARARGS, "Bind to the specified IP and port." },
    { "listen", (PyCFunction)TCPServer_func_listen, METH_VARARGS, "Start listening for TCP connections." },
    { "close", (PyCFunction)TCPServer_func_close, METH_NOARGS, "Stop listening for TCP connections." },
    { "accept", (PyCFunction)TCPServer_func_accept, METH_NOARGS, "Accept incoming TCP connection." },
    { "getsockname", (PyCFunction)TCPServer_func_getsockname, METH_NOARGS, "Get local socket information." },
    { NULL }
};


static PyMemberDef TCPServer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(TCPServer, loop), READONLY, "Loop where this TCPServer is running on."},
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


/* TCPClientConnection */

static PyObject *
TCPClientConnection_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPClientConnection *self = (TCPClientConnection *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static PyTypeObject TCPClientConnectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCPClientConnection",                                    /*tp_name*/
    sizeof(TCPClientConnection),                                   /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /*tp_flags*/
    0,                                                             /*tp_doc*/
    0,                                                             /*tp_traverse*/
    0,                                                             /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    0,                                                             /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    TCPClientConnection_tp_new,                                    /*tp_new*/
};


/* TCPClient */

typedef struct {
    uv_connect_t req;
    void *data;
} tcp_connect_req_t;


static void
on_tcp_client_connect(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    tcp_connect_req_t* connect_req = (tcp_connect_req_t*) req;
    iostream_req_data_t* req_data = (iostream_req_data_t *)connect_req->data;

    IOStream *self = (IOStream *)req_data->obj;
    PyObject *callback = req_data->callback;

    PyObject *error;
    PyObject *exc_data;
    PyObject *result;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        self->connected = False;
        error = PyBool_FromLong(1);
        uv_err_t err = uv_last_error(SELF_LOOP);
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_TCPClientError, exc_data);
            Py_DECREF(exc_data);
        }
        // TODO: pass exception to callback, how?
        PyErr_WriteUnraisable(callback);
    } else {
        self->connected = True;
        error = PyBool_FromLong(0);
    }

    result = PyObject_CallFunctionObjArgs(callback, self, error, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);

    Py_DECREF(callback);
    PyMem_Free(req_data);
    connect_req->data = NULL;
    PyMem_Free(connect_req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
TCPClient_func_connect(TCPClient *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *connect_ip;
    int connect_port;
    int address_type;
    struct in_addr addr4;
    struct in6_addr addr6;
    tcp_connect_req_t *connect_req = NULL;
    iostream_req_data_t *req_data = NULL;
    uv_tcp_t *uv_stream = NULL;
    PyObject *connect_address;
    PyObject *callback;

    IOStream *parent = (IOStream *)self;

    if (parent->connected) {
        PyErr_SetString(PyExc_TCPClientError, "already connected");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "OO:connect", &connect_address, &callback)) {
        return NULL;
    }

    if (!PyArg_ParseTuple(connect_address, "si", &connect_ip, &connect_port)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (connect_port < 0 || connect_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return NULL;
    }

    if (inet_pton(AF_INET, connect_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (inet_pton(AF_INET6, connect_ip, &addr6) == 1) {
        address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    connect_req = (tcp_connect_req_t*) PyMem_Malloc(sizeof(tcp_connect_req_t));
    if (!connect_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    uv_stream = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        goto error;
    }

    r = uv_tcp_init(PARENT_LOOP, (uv_tcp_t *)uv_stream);
    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_TCPClientError);
        goto error;
    }
    uv_stream->data = (void *)self;
    parent->uv_stream = (uv_stream_t *)uv_stream;

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;

    connect_req->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_tcp_connect(&connect_req->req, (uv_tcp_t *)parent->uv_stream, uv_ip4_addr(connect_ip, connect_port), on_tcp_client_connect);
    } else {
        r = uv_tcp_connect6(&connect_req->req, (uv_tcp_t *)parent->uv_stream, uv_ip6_addr(connect_ip, connect_port), on_tcp_client_connect);
    }

    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_TCPClientError);
        goto error;
    }

    /* Increment reference count while libuv keeps this object around. It'll be decremented on handle close. */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    if (connect_req) {
        connect_req->data = NULL;
        PyMem_Free(connect_req);
    }
    if (req_data) {
        Py_DECREF(callback);
        req_data->obj = NULL;
        req_data->callback = NULL;
        PyMem_Free(req_data);
    }
    if (uv_stream) {
        uv_stream->data = NULL;
        PyMem_Free(uv_stream);
    }
    parent->uv_stream = NULL;
    return NULL;
}


static PyObject *
TCPClient_func_getsockname(TCPClient *self)
{
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(sockname);

    IOStream *parent = (IOStream *)self;

    if (!parent->connected) {
        PyErr_SetString(PyExc_TCPClientError, "disconnected");
        return NULL;
    }

    int r = uv_tcp_getsockname((uv_tcp_t *)parent->uv_stream, &sockname, &namelen);
    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_TCPClientError);
        return NULL;
    }

    if (sockname.sa_family == AF_INET) {
        addr4 = (struct sockaddr_in*)&sockname;
        r = uv_ip4_name(addr4, ip4, INET_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip4, ntohs(addr4->sin_port));
    } else if (sockname.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&sockname;
        r = uv_ip6_name(addr6, ip6, INET6_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip6, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_TCPClientError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCPClient_func_getpeername(TCPClient *self)
{
    struct sockaddr peername;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(peername);

    IOStream *parent = (IOStream *)self;

    if (!parent->connected) {
        PyErr_SetString(PyExc_TCPClientError, "disconnected");
        return NULL;
    }

    int r = uv_tcp_getpeername((uv_tcp_t *)parent->uv_stream, &peername, &namelen);
    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_TCPClientError);
        return NULL;
    }

    if (peername.sa_family == AF_INET) {
        addr4 = (struct sockaddr_in*)&peername;
        r = uv_ip4_name(addr4, ip4, INET_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip4, ntohs(addr4->sin_port));
    } else if (peername.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&peername;
        r = uv_ip6_name(addr6, ip6, INET6_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip6, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_TCPClientError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCPClient_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPClient *self = (TCPClient *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static PyMethodDef
TCPClient_tp_methods[] = {
    { "connect", (PyCFunction)TCPClient_func_connect, METH_VARARGS, "Start connecion to the remote endpoint." },
    { "getsockname", (PyCFunction)TCPClient_func_getsockname, METH_NOARGS, "Get local socket information." },
    { "getpeername", (PyCFunction)TCPClient_func_getpeername, METH_NOARGS, "Get remote socket information." },
    { NULL }
};


static PyTypeObject TCPClientType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCPClient",                                              /*tp_name*/
    sizeof(TCPClient),                                             /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /*tp_flags*/
    0,                                                             /*tp_doc*/
    0,                                                             /*tp_traverse*/
    0,                                                             /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    TCPClient_tp_methods,                                          /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    TCPClient_tp_new,                                              /*tp_new*/
};

