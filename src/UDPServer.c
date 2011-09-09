
static PyObject* PyExc_UDPServerError;


typedef struct {
    uv_udp_send_t req;
    uv_buf_t buf;
    void *data;
} udp_write_req_t;


static void
on_udp_server_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(handle);
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static uv_buf_t
on_udp_alloc(uv_udp_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(suggested_size <= UDP_MAX_BUF_SIZE);
    uv_buf_t buf;
    buf.base = PyMem_Malloc(suggested_size);
    buf.len = suggested_size;
    PyGILState_Release(gstate);
    return buf;
}


static void
on_udp_read(uv_udp_t* handle, int nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int r = 0;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    PyObject *address_tuple;
    PyObject *data;
    PyObject *result;

    UNUSED_ARG(r);

    assert(handle);
    assert(flags == 0);

    UDPServer *self = (UDPServer *)(handle->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread > 0) {
        assert(addr);
        if (addr->sa_family == AF_INET) {
            addr4 = *(struct sockaddr_in*)addr;
            r = uv_ip4_name(&addr4, ip4, INET_ADDRSTRLEN);
            assert(r == 0);
            address_tuple = Py_BuildValue("(si)", ip4, ntohs(addr4.sin_port));
        } else {
            addr6 = *(struct sockaddr_in6*)addr;
            r = uv_ip6_name(&addr6, ip6, INET6_ADDRSTRLEN);
            assert(r == 0);
            address_tuple = Py_BuildValue("(si)", ip6, ntohs(addr6.sin6_port));
        }
        data = PyString_FromStringAndSize(buf.base, nread);
        result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, address_tuple, data, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_read_cb);
        }
        Py_XDECREF(result);
    } else if (nread < 0) { 
        PyErr_SetString(PyExc_UDPServerError, "unexpected recv error");
        PyErr_WriteUnraisable(self->on_read_cb);
    } else {
        assert(addr == NULL);
    }
    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_udp_write(uv_udp_send_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(req);
    assert(status == 0);

    udp_write_req_t *wr = (udp_write_req_t *)req;
    UDPServer *self = (UDPServer *)(wr->data);
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


static PyObject *
UDPServer_func_listen(UDPServer *self, PyObject *args, PyObject *kwargs)
{
    PyObject *read_callback;
    PyObject *write_callback;
    PyObject *tmp = NULL;
    int r;

    static char *kwlist[] = {"read_cb", "write_cb", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:listen", kwlist, &read_callback, &write_callback)) {
        return NULL;
    }

    if (!PyCallable_Check(read_callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    } else {
        tmp = self->on_read_cb;
        Py_INCREF(read_callback);
        self->on_read_cb = read_callback;
        Py_XDECREF(tmp);
    }

    if (!PyCallable_Check(write_callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    } else {
        tmp = self->on_write_cb;
        Py_INCREF(write_callback);
        self->on_write_cb = write_callback;
        Py_XDECREF(tmp);
    }

    if (self->address_type == AF_INET) {
        r = uv_udp_bind(self->uv_udp_server, uv_ip4_addr(self->listen_ip, self->listen_port), 0);
    } else {
        r = uv_udp_bind6(self->uv_udp_server, uv_ip6_addr(self->listen_ip, self->listen_port), UV_UDP_IPV6ONLY);
    }
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_UDPServerError, NULL);
    }

    r = uv_udp_recv_start(self->uv_udp_server, (uv_alloc_cb)on_udp_alloc, (uv_udp_recv_cb)on_udp_read);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_UDPServerError, NULL);
    }

    Py_RETURN_NONE;
}


static PyObject *
UDPServer_func_stop_listening(UDPServer *self)
{
    if (self->uv_udp_server) {
        self->uv_udp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_udp_server, on_udp_server_close);
        self->uv_udp_server = NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
UDPServer_func_write(UDPServer *self, PyObject *args)
{
    PyObject *address_tuple;
    udp_write_req_t *wr;
    char *write_data;
    char *listen_ip;
    int listen_port;
    int r;

    if (!PyArg_ParseTuple(args, "sO:write", &write_data, &address_tuple)) {
        return NULL;
    }

    if (!PyArg_ParseTuple(address_tuple, "si", &listen_ip, &listen_port)) {
        return NULL;
    }

    wr = (udp_write_req_t*) PyMem_Malloc(sizeof *wr);
    if (!wr) {
        return PyErr_NoMemory();
    }
    wr->buf.base = write_data;
    wr->buf.len = strlen(write_data);
    wr->data = (void *)self;

    if (self->address_type == AF_INET) {
        r = uv_udp_send(&wr->req, self->uv_udp_server, &wr->buf, 1, uv_ip4_addr(listen_ip, listen_port), (uv_udp_send_cb)on_udp_write);
    } else {
        r = uv_udp_send6(&wr->req, self->uv_udp_server, &wr->buf, 1, uv_ip6_addr(listen_ip, listen_port), (uv_udp_send_cb)on_udp_write);
    }
    if (r) {
        wr->data = NULL;
        RAISE_ERROR(SELF_LOOP, PyExc_UDPServerError, NULL);
    }

    Py_RETURN_NONE;
}


static int
UDPServer_tp_init(UDPServer *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *listen_address;
    PyObject *tmp = NULL;
    char *listen_ip;
    int listen_port;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_udp_t *uv_udp_server;

    if (!PyArg_ParseTuple(args, "O!O:__init__", &LoopType, &loop, &listen_address)) {
        return -1;
    }

    if (!PyArg_ParseTuple(listen_address, "si", &listen_ip, &listen_port)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

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

    uv_udp_server = PyMem_Malloc(sizeof(uv_udp_t));
    if (!uv_udp_server) {
        PyErr_NoMemory();
        return -1;
    }
    int r = uv_udp_init(SELF_LOOP, uv_udp_server);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_UDPServerError, -1);
    }
    uv_udp_server->data = (void *)self;
    self->uv_udp_server = uv_udp_server;

    return 0;
}


static PyObject *
UDPServer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    UDPServer *self = (UDPServer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
UDPServer_tp_traverse(UDPServer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->listen_address);
    Py_VISIT(self->on_read_cb);
    Py_VISIT(self->on_write_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
UDPServer_tp_clear(UDPServer *self)
{
    Py_CLEAR(self->listen_address);
    Py_CLEAR(self->on_read_cb);
    Py_CLEAR(self->on_write_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
UDPServer_tp_dealloc(UDPServer *self)
{
    if (self->uv_udp_server) {
        self->uv_udp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_udp_server, on_udp_server_close);
        self->uv_udp_server = NULL;
    }
    UDPServer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
UDPServer_tp_methods[] = {
    { "listen", (PyCFunction)UDPServer_func_listen, METH_VARARGS|METH_KEYWORDS, "Start listening for UDP connections." },
    { "stop_listening", (PyCFunction)UDPServer_func_stop_listening, METH_NOARGS, "Stop listening for UDP connections." },
    { "write", (PyCFunction)UDPServer_func_write, METH_VARARGS, "Write data over UDP." },
    { NULL }
};


static PyMemberDef UDPServer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(UDPServer, loop), READONLY, "Loop where this UDPServer is running on."},
    {"listen_address", T_OBJECT_EX, offsetof(UDPServer, listen_address), 0, "Address tuple where this server listens."},
    {NULL}
};


static PyTypeObject UDPServerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.UDPServer",                                               /*tp_name*/
    sizeof(UDPServer),                                              /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)UDPServer_tp_dealloc,                               /*tp_dealloc*/
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
    (traverseproc)UDPServer_tp_traverse,                            /*tp_traverse*/
    (inquiry)UDPServer_tp_clear,                                    /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    UDPServer_tp_methods,                                           /*tp_methods*/
    UDPServer_tp_members,                                           /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)UDPServer_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    UDPServer_tp_new,                                               /*tp_new*/
};


