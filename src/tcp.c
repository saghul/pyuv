
static PyObject* PyExc_TCPError;


static void
on_tcp_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(server);

    TCP *self = (TCP *)server->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    IOStream *base = (IOStream *)self;

    PyObject *result, *py_errorno;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
        py_errorno = PyInt_FromLong((long)err.code);
    } else {
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_new_connection_cb);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_tcp_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    iostream_req_data_t* req_data = (iostream_req_data_t *)req->data;

    TCP *self = (TCP *)req_data->obj;
    PyObject *callback = req_data->callback;

    PyObject *result, *py_errorno;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    IOStream *base = (IOStream *)self;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
        py_errorno = PyInt_FromLong(err.code);
    } else {
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(callback, self, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);

    Py_DECREF(callback);
    PyMem_Free(req_data);
    req->data = NULL;
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
TCP_func_bind(TCP *self, PyObject *args)
{
    int r = 0;
    char *bind_ip;
    int bind_port;
    int address_type;
    struct in_addr addr4;
    struct in6_addr addr6;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
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

    if (address_type == AF_INET) {
        r = uv_tcp_bind((uv_tcp_t *)base->uv_handle, uv_ip4_addr(bind_ip, bind_port));
    } else {
        r = uv_tcp_bind6((uv_tcp_t *)base->uv_handle, uv_ip6_addr(bind_ip, bind_port));
    }

    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_listen(TCP *self, PyObject *args)
{
    int r;
    int backlog = 128;
    PyObject *callback;
    PyObject *tmp = NULL;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
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
    }

    r = uv_listen(base->uv_handle, backlog, on_tcp_connection);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    tmp = self->on_new_connection_cb;
    Py_INCREF(callback);
    self->on_new_connection_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_accept(TCP *self, PyObject *args)
{
    int r = 0;
    PyObject *client;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:accept", &client)) {
        return NULL;
    }

    if (!PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&IOStreamType)) {
        PyErr_SetString(PyExc_TypeError, "Only stream objects are supported for accept");
        return NULL;
    }

    r = uv_accept(base->uv_handle, ((IOStream *)client)->uv_handle);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_connect(TCP *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *connect_ip;
    int connect_port;
    int address_type;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_connect_t *connect_req = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *connect_address;
    PyObject *callback;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "closed");
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

    connect_req = (uv_connect_t *)PyMem_Malloc(sizeof(uv_connect_t));
    if (!connect_req) {
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
    connect_req->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_tcp_connect(connect_req, (uv_tcp_t *)base->uv_handle, uv_ip4_addr(connect_ip, connect_port), on_tcp_client_connection);
    } else {
        r = uv_tcp_connect6(connect_req, (uv_tcp_t *)base->uv_handle, uv_ip6_addr(connect_ip, connect_port), on_tcp_client_connection);
    }

    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        goto error;
    }

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
    return NULL;
}


static PyObject *
TCP_func_getsockname(TCP *self)
{
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(sockname);

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "closed");
        return NULL;
    }

    int r = uv_tcp_getsockname((uv_tcp_t *)base->uv_handle, &sockname, &namelen);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
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
        PyErr_SetString(PyExc_TCPError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCP_func_getpeername(TCP *self)
{
    struct sockaddr peername;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(peername);

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "closed");
        return NULL;
    }

    int r = uv_tcp_getpeername((uv_tcp_t *)base->uv_handle, &peername, &namelen);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
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
        PyErr_SetString(PyExc_TCPError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCP_func_nodelay(TCP *self, PyObject *args)
{
    int r = 0;
    PyObject *enable;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!:nodelay", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_tcp_nodelay((uv_tcp_t *)base->uv_handle, (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_keepalive(TCP *self, PyObject *args)
{
    int r = 0;
    unsigned int delay;
    PyObject *enable;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!I:keepalive", &PyBool_Type, &enable, &delay)) {
        return NULL;
    }

    r = uv_tcp_keepalive((uv_tcp_t *)base->uv_handle, (enable == Py_True) ? 1 : 0, delay);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_simultaneous_accepts(TCP *self, PyObject *args)
{
    /*
    * This setting applies to Windows only.
    * Enable/disable simultaneous asynchronous accept requests that are
    * queued by the operating system when listening for new tcp connections.
    * This setting is used to tune a tcp server for the desired performance.
    * Having simultaneous accepts can significantly improve the rate of
    * accepting connections (which is why it is enabled by default).
    */

    int r = 0;
    PyObject *enable;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!:simultaneous_accepts", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_tcp_simultaneous_accepts((uv_tcp_t *)base->uv_handle, (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
TCP_tp_init(TCP *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_tcp_t *uv_stream;

    IOStream *base = (IOStream *)self;

    if (base->uv_handle) {
        PyErr_SetString(PyExc_TCPError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)base->loop;
    Py_INCREF(loop);
    base->loop = loop;
    Py_XDECREF(tmp);

    uv_stream = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_tcp_init(UV_LOOP(base), (uv_tcp_t *)uv_stream);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TCPError);
        Py_DECREF(loop);
        return -1;
    }
    uv_stream->data = (void *)self;
    base->uv_handle = (uv_stream_t *)uv_stream;

    return 0;
}


static PyObject *
TCP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCP *self = (TCP *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
TCP_tp_traverse(TCP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    IOStreamType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
TCP_tp_clear(TCP *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    IOStreamType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
TCP_tp_methods[] = {
    { "bind", (PyCFunction)TCP_func_bind, METH_VARARGS, "Bind to the specified IP and port." },
    { "listen", (PyCFunction)TCP_func_listen, METH_VARARGS, "Start listening for TCP connections." },
    { "accept", (PyCFunction)TCP_func_accept, METH_VARARGS, "Accept incoming connection." },
    { "connect", (PyCFunction)TCP_func_connect, METH_VARARGS, "Start connecion to remote endpoint." },
    { "getsockname", (PyCFunction)TCP_func_getsockname, METH_NOARGS, "Get local socket information." },
    { "getpeername", (PyCFunction)TCP_func_getpeername, METH_NOARGS, "Get remote socket information." },
    { "nodelay", (PyCFunction)TCP_func_nodelay, METH_VARARGS, "Enable/disable Nagle's algorithm." },
    { "keepalive", (PyCFunction)TCP_func_keepalive, METH_VARARGS, "Enable/disable TCP keep-alive." },
    { "simultaneous_accepts", (PyCFunction)TCP_func_simultaneous_accepts, METH_VARARGS, "Enable/disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new tcp connections." },
    { NULL }
};


static PyTypeObject TCPType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCP",                                                    /*tp_name*/
    sizeof(TCP),                                                   /*tp_basicsize*/
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
    (traverseproc)TCP_tp_traverse,                                 /*tp_traverse*/
    (inquiry)TCP_tp_clear,                                         /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    TCP_tp_methods,                                                /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)TCP_tp_init,                                         /*tp_init*/
    0,                                                             /*tp_alloc*/
    TCP_tp_new,                                                    /*tp_new*/
};

