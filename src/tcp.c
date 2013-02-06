
static void
on_tcp_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    TCP *self;
    PyObject *result, *py_errorno;

    ASSERT(server);
    self = (TCP *)server->data;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    } else {
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, py_errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(py_errorno);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_tcp_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    TCP *self;
    PyObject *callback, *result, *py_errorno;

    ASSERT(req);
    self = (TCP *)req->handle->data;
    callback = (PyObject *)req->data;

    ASSERT(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong(err.code);
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

    Py_DECREF(callback);
    PyMem_Free(req);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
TCP_func_bind(TCP *self, PyObject *args)
{
    int r, bind_port, address_type;
    char *bind_ip;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "(si):bind", &bind_ip, &bind_port)) {
        return NULL;
    }

    if (bind_port < 0 || bind_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }

    if (pyuv_guess_ip_family(bind_ip, &address_type)) {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    if (address_type == AF_INET) {
        r = uv_tcp_bind((uv_tcp_t *)UV_HANDLE(self), uv_ip4_addr(bind_ip, bind_port));
    } else {
        r = uv_tcp_bind6((uv_tcp_t *)UV_HANDLE(self), uv_ip6_addr(bind_ip, bind_port));
    }

    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_listen(TCP *self, PyObject *args)
{
    int r, backlog;
    PyObject *callback, *tmp;

    backlog = 128;
    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

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

    r = uv_listen((uv_stream_t *)UV_HANDLE(self), backlog, on_tcp_connection);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
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
    int r;
    PyObject *client;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:accept", &client)) {
        return NULL;
    }

    if (!PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&StreamType)) {
        PyErr_SetString(PyExc_TypeError, "Only stream objects are supported for accept");
        return NULL;
    }

    r = uv_accept((uv_stream_t *)UV_HANDLE(self), (uv_stream_t *)UV_HANDLE(client));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_connect(TCP *self, PyObject *args)
{
    int r, connect_port, address_type;
    char *connect_ip;
    uv_connect_t *connect_req = NULL;
    PyObject *callback;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "(si)O:connect", &connect_ip, &connect_port, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (connect_port < 0 || connect_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }

    if (pyuv_guess_ip_family(connect_ip, &address_type)) {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    Py_INCREF(callback);

    connect_req = PyMem_Malloc(sizeof *connect_req);
    if (!connect_req) {
        PyErr_NoMemory();
        goto error;
    }

    connect_req->data = (void *)callback;

    if (address_type == AF_INET) {
        r = uv_tcp_connect(connect_req, (uv_tcp_t *)UV_HANDLE(self), uv_ip4_addr(connect_ip, connect_port), on_tcp_client_connection);
    } else {
        r = uv_tcp_connect6(connect_req, (uv_tcp_t *)UV_HANDLE(self), uv_ip6_addr(connect_ip, connect_port), on_tcp_client_connection);
    }

    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    PyMem_Free(connect_req);
    return NULL;
}


static PyObject *
TCP_func_getsockname(TCP *self)
{
    int r, namelen;
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    namelen = sizeof(sockname);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_tcp_getsockname((uv_tcp_t *)UV_HANDLE(self), &sockname, &namelen);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    if (sockname.sa_family == AF_INET) {
        addr4 = (struct sockaddr_in*)&sockname;
        uv_ip4_name(addr4, ip, INET_ADDRSTRLEN);
        return Py_BuildValue("si", ip, ntohs(addr4->sin_port));
    } else if (sockname.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&sockname;
        uv_ip6_name(addr6, ip, INET6_ADDRSTRLEN);
        return Py_BuildValue("si", ip, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_TCPError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCP_func_getpeername(TCP *self)
{
    int r, namelen;
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr peername;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    namelen = sizeof(peername);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_tcp_getpeername((uv_tcp_t *)UV_HANDLE(self), &peername, &namelen);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    if (peername.sa_family == AF_INET) {
        addr4 = (struct sockaddr_in*)&peername;
        r = uv_ip4_name(addr4, ip, INET_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip, ntohs(addr4->sin_port));
    } else if (peername.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&peername;
        r = uv_ip6_name(addr6, ip, INET6_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_TCPError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
TCP_func_nodelay(TCP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:nodelay", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_tcp_nodelay((uv_tcp_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_keepalive(TCP *self, PyObject *args)
{
    int r;
    unsigned int delay;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!I:keepalive", &PyBool_Type, &enable, &delay)) {
        return NULL;
    }

    r = uv_tcp_keepalive((uv_tcp_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0, delay);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_simultaneous_accepts(TCP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:simultaneous_accepts", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_tcp_simultaneous_accepts((uv_tcp_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_open(TCP *self, PyObject *args)
{
    long fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "l:open", &fd)) {
        return NULL;
    }

    uv_tcp_open((uv_tcp_t *)UV_HANDLE(self), (uv_os_sock_t)fd);

    Py_RETURN_NONE;
}


static int
TCP_tp_init(TCP *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    r = uv_tcp_init(loop->uv_loop, (uv_tcp_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_TCPError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
TCP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCP *self;
    uv_tcp_t *uv_tcp;

    uv_tcp = PyMem_Malloc(sizeof *uv_tcp);
    if (!uv_tcp) {
        PyErr_NoMemory();
        return NULL;
    }

    self = (TCP *)StreamType.tp_new(type, args, kwargs);
    if (!self) {
        PyMem_Free(uv_tcp);
        return NULL;
    }

    uv_tcp->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_tcp;

    return (PyObject *)self;
}


static int
TCP_tp_traverse(TCP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    StreamType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
TCP_tp_clear(TCP *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    StreamType.tp_clear((PyObject *)self);
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
    { "open", (PyCFunction)TCP_func_open, METH_VARARGS, "Open the specified file descriptor and manage it as a TCP handle." },
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
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

