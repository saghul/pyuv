
static void
on_tcp_connection(uv_stream_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    TCP *self;
    PyObject *result, *py_errorno;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, TCP, tcp_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        py_errorno = PyInt_FromLong((long)status);
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
    self = PYUV_CONTAINER_OF(req->handle, TCP, tcp_h);
    callback = (PyObject *)req->data;

    if (status != 0) {
        py_errorno = PyInt_FromLong(status);
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
    int err;
    struct sockaddr sa;
    PyObject *addr;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:bind", &addr)) {
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &sa) < 0) {
        /* Error is set by the function itself */
        return NULL;
    }

    if (sa.sa_family == AF_INET) {
        err = uv_tcp_bind(&self->tcp_h, *(struct sockaddr_in *)&sa);
    } else {
        err = uv_tcp_bind6(&self->tcp_h, *(struct sockaddr_in6 *)&sa);
    }

    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_listen(TCP *self, PyObject *args)
{
    int err, backlog;
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

    err = uv_listen((uv_stream_t *)&self->tcp_h, backlog, on_tcp_connection);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
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
    int err;
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

    err = uv_accept((uv_stream_t *)&self->tcp_h, (uv_stream_t *)UV_HANDLE(client));
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_connect(TCP *self, PyObject *args)
{
    int err;
    struct sockaddr sa;
    uv_connect_t *connect_req = NULL;
    PyObject *addr, *callback;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "OO:connect", &addr, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &sa) < 0) {
        /* Error is set by the function itself */
        return NULL;
    }

    Py_INCREF(callback);

    connect_req = PyMem_Malloc(sizeof *connect_req);
    if (!connect_req) {
        PyErr_NoMemory();
        goto error;
    }

    connect_req->data = callback;

    if (sa.sa_family == AF_INET) {
        err = uv_tcp_connect(connect_req, &self->tcp_h, *(struct sockaddr_in *)&sa, on_tcp_client_connection);
    } else {
        err = uv_tcp_connect6(connect_req, &self->tcp_h, *(struct sockaddr_in6 *)&sa, on_tcp_client_connection);
    }

    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
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
    int err, namelen;
    struct sockaddr sockname;

    namelen = sizeof(sockname);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_tcp_getsockname(&self->tcp_h, &sockname, &namelen);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    return makesockaddr(&sockname, namelen);
}


static PyObject *
TCP_func_getpeername(TCP *self)
{
    int err, namelen;
    struct sockaddr peername;

    namelen = sizeof(peername);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_tcp_getpeername(&self->tcp_h, &peername, &namelen);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    return makesockaddr(&peername, namelen);
}


static PyObject *
TCP_func_nodelay(TCP *self, PyObject *args)
{
    int err;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:nodelay", &PyBool_Type, &enable)) {
        return NULL;
    }

    err = uv_tcp_nodelay(&self->tcp_h, (enable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_keepalive(TCP *self, PyObject *args)
{
    int err;
    unsigned int delay;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!I:keepalive", &PyBool_Type, &enable, &delay)) {
        return NULL;
    }

    err = uv_tcp_keepalive(&self->tcp_h, (enable == Py_True) ? 1 : 0, delay);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_simultaneous_accepts(TCP *self, PyObject *args)
{
    int err;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:simultaneous_accepts", &PyBool_Type, &enable)) {
        return NULL;
    }

    err = uv_tcp_simultaneous_accepts(&self->tcp_h, (enable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TCP_func_open(TCP *self, PyObject *args)
{
    int err;
    long fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "l:open", &fd)) {
        return NULL;
    }

    err = uv_tcp_open(&self->tcp_h, (uv_os_sock_t)fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
TCP_tp_init(TCP *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_tcp_init(loop->uv_loop, &self->tcp_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TCPError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
TCP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCP *self;

    self = (TCP *)StreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->tcp_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->tcp_h;

    return (PyObject *)self;
}


static int
TCP_tp_traverse(TCP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    return StreamType.tp_traverse((PyObject *)self, visit, arg);
}


static int
TCP_tp_clear(TCP *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    return StreamType.tp_clear((PyObject *)self);
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

