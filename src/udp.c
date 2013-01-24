
typedef struct {
    PyObject *callback;
    Py_buffer *views;
    int view_count;
} udp_send_data_t;


static uv_buf_t
on_udp_alloc(uv_udp_t* handle, size_t suggested_size)
{
    static char slab[PYUV_SLAB_SIZE];
    UNUSED_ARG(handle);
    return uv_buf_init(slab, sizeof(slab));
}


static void
on_udp_read(uv_udp_t* handle, int nread, uv_buf_t buf, struct sockaddr* addr, unsigned flags)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    uv_err_t err;
    UDP *self;
    PyObject *result, *address_tuple, *data, *py_errorno;

    ASSERT(handle);
    ASSERT(flags == 0);

    self = (UDP *)handle->data;
    ASSERT(self);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread == 0) {
        goto done;
    }

    if (nread > 0) {
        ASSERT(addr);
        if (addr->sa_family == AF_INET) {
            addr4 = *(struct sockaddr_in*)addr;
            uv_ip4_name(&addr4, ip, INET_ADDRSTRLEN);
            address_tuple = Py_BuildValue("(si)", ip, ntohs(addr4.sin_port));
        } else {
            addr6 = *(struct sockaddr_in6*)addr;
            uv_ip6_name(&addr6, ip, INET6_ADDRSTRLEN);
            address_tuple = Py_BuildValue("(si)", ip, ntohs(addr6.sin6_port));
        }
        data = PyBytes_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        address_tuple = Py_None;
        Py_INCREF(Py_None);
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, address_tuple, data, py_errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(address_tuple);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

done:
    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_udp_send(uv_udp_send_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    udp_send_data_t* req_data;
    UDP *self;
    PyObject *callback, *result, *py_errorno;

    ASSERT(req);

    req_data = (udp_send_data_t *)req->data;

    self = (UDP *)req->handle->data;
    callback = req_data->callback;

    ASSERT(self);

    if (callback != Py_None) {
        if (status < 0) {
            uv_err_t err = uv_last_error(UV_HANDLE_LOOP(self));
            py_errorno = PyInt_FromLong((long)err.code);
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
    }

    Py_DECREF(callback);
    for (i = 0; i < req_data->view_count; i++) {
        PyBuffer_Release(&req_data->views[i]);
    }
    PyMem_Free(req_data->views);
    PyMem_Free(req_data);
    PyMem_Free(req);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
UDP_func_bind(UDP *self, PyObject *args)
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
        r = uv_udp_bind((uv_udp_t *)UV_HANDLE(self), uv_ip4_addr(bind_ip, bind_port), 0);
    } else {
        r = uv_udp_bind6((uv_udp_t *)UV_HANDLE(self), uv_ip6_addr(bind_ip, bind_port), UV_UDP_IPV6ONLY);
    }

    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_start_recv(UDP *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start_recv", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_udp_recv_start((uv_udp_t *)UV_HANDLE(self), (uv_alloc_cb)on_udp_alloc, (uv_udp_recv_cb)on_udp_read);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    tmp = self->on_read_cb;
    Py_INCREF(callback);
    self->on_read_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_stop_recv(UDP *self)
{
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_udp_recv_stop((uv_udp_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_XDECREF(self->on_read_cb);
    self->on_read_cb = NULL;

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_send(UDP *self, PyObject *args)
{
    int r, dest_port, address_type;
    char *dest_ip;
    uv_buf_t buf;
    Py_buffer *view;
    PyObject *callback = Py_None;
    uv_udp_send_t *wr = NULL;
    udp_send_data_t *req_data = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    view = PyMem_Malloc(sizeof *view);
    if (!view) {
        PyErr_NoMemory();
        return NULL;
    }

#ifdef PYUV_PYTHON3
    if (!PyArg_ParseTuple(args, "(si)y*|O:send", &dest_ip, &dest_port, view, &callback)) {
#else
    if (!PyArg_ParseTuple(args, "(si)s*|O:send", &dest_ip, &dest_port, view, &callback)) {
#endif
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        goto error1;
    }

    if (dest_port < 0 || dest_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        goto error1;
    }

    if (pyuv_guess_ip_family(dest_ip, &address_type)) {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        goto error1;
    }

    Py_INCREF(callback);

    wr = PyMem_Malloc(sizeof *wr);
    if (!wr) {
        PyErr_NoMemory();
        goto error2;
    }

    req_data = PyMem_Malloc(sizeof *req_data);
    if (!req_data) {
        PyErr_NoMemory();
        goto error2;
    }

    buf = uv_buf_init(view->buf, view->len);
    req_data->callback = callback;
    req_data->view_count = 1;
    req_data->views = view;

    wr->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_udp_send(wr, (uv_udp_t *)UV_HANDLE(self), &buf, 1, uv_ip4_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(wr, (uv_udp_t *)UV_HANDLE(self), &buf, 1, uv_ip6_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    }
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        goto error2;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error2:
    Py_DECREF(callback);
    PyMem_Free(req_data);
    PyMem_Free(wr);
error1:
    PyBuffer_Release(view);
    PyMem_Free(view);
    return NULL;
}


static PyObject *
UDP_func_sendlines(UDP *self, PyObject *args)
{
    int i, r, buf_count, dest_port, address_type;
    char *dest_ip;
    PyObject *callback, *seq;
    Py_buffer *views;
    uv_buf_t *bufs;
    uv_udp_send_t *wr = NULL;
    udp_send_data_t *req_data = NULL;

    callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "(si)O|O:sendlines", &dest_ip, &dest_port, &seq, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    if (dest_port < 0 || dest_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }

    if (pyuv_guess_ip_family(dest_ip, &address_type)) {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    r = pyseq2uvbuf(seq, &views, &bufs, &buf_count);
    if (r != 0) {
        /* error is already set */
        return NULL;
    }

    Py_INCREF(callback);

    wr = PyMem_Malloc(sizeof *wr);
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof *req_data);
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->callback = callback;
    req_data->view_count = buf_count;
    req_data->views = views;
    wr->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_udp_send(wr, (uv_udp_t *)UV_HANDLE(self), bufs, buf_count, uv_ip4_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(wr, (uv_udp_t *)UV_HANDLE(self), bufs, buf_count, uv_ip6_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    }

    /* uv_write copies the uv_buf_t structures, so we can free them now */
    PyMem_Free(bufs);
    bufs = NULL;

    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    for (i = 0; i < buf_count; i++) {
        PyBuffer_Release(&views[i]);
    }
    PyMem_Free(views);
    PyMem_Free(bufs);
    PyMem_Free(req_data);
    PyMem_Free(wr);
    return NULL;
}


static PyObject *
UDP_func_set_membership(UDP *self, PyObject *args)
{
    int r, membership;
    char *multicast_address, *interface_address;

    interface_address = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "si|s:set_membership", &multicast_address, &membership, &interface_address)) {
        return NULL;
    }

    r = uv_udp_set_membership((uv_udp_t *)UV_HANDLE(self), multicast_address, interface_address, membership);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_getsockname(UDP *self)
{
    int r, namelen;
    char ip[INET6_ADDRSTRLEN];
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    namelen = sizeof(sockname);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_udp_getsockname((uv_udp_t *)UV_HANDLE(self), &sockname, &namelen);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
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
        PyErr_SetString(PyExc_UDPError, "unknown address type detected");
        return NULL;
    }
}


static PyObject *
UDP_func_set_multicast_ttl(UDP *self, PyObject *args)
{
    int r, ttl;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    r = uv_udp_set_multicast_ttl((uv_udp_t *)UV_HANDLE(self), ttl);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_broadcast(UDP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:set_broadcast", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_udp_set_broadcast((uv_udp_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_multicast_loop(UDP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:set_multicast_loop", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_udp_set_multicast_loop((uv_udp_t *)UV_HANDLE(self), (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_ttl(UDP *self, PyObject *args)
{
    int r, ttl;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    r = uv_udp_set_ttl((uv_udp_t *)UV_HANDLE(self), ttl);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_open(UDP *self, PyObject *args)
{
    long fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "l:open", &fd)) {
        return NULL;
    }

    uv_udp_open((uv_udp_t *)UV_HANDLE(self), (uv_os_sock_t)fd);

    Py_RETURN_NONE;
}


static int
UDP_tp_init(UDP *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    r = uv_udp_init(loop->uv_loop, (uv_udp_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_UDPError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
UDP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    uv_udp_t *uv_udp;

    uv_udp = PyMem_Malloc(sizeof *uv_udp);
    if (!uv_udp) {
        PyErr_NoMemory();
        return NULL;
    }

    UDP *self = (UDP *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        PyMem_Free(uv_udp);
        return NULL;
    }

    uv_udp->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_udp;

    return (PyObject *)self;
}


static int
UDP_tp_traverse(UDP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_read_cb);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
UDP_tp_clear(UDP *self)
{
    Py_CLEAR(self->on_read_cb);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
UDP_tp_methods[] = {
    { "bind", (PyCFunction)UDP_func_bind, METH_VARARGS, "Bind to the specified IP and port." },
    { "start_recv", (PyCFunction)UDP_func_start_recv, METH_VARARGS, "Start accepting data." },
    { "stop_recv", (PyCFunction)UDP_func_stop_recv, METH_NOARGS, "Stop receiving data." },
    { "send", (PyCFunction)UDP_func_send, METH_VARARGS, "Send data over UDP." },
    { "sendlines", (PyCFunction)UDP_func_sendlines, METH_VARARGS, "Send a sequence of data over UDP." },
    { "getsockname", (PyCFunction)UDP_func_getsockname, METH_NOARGS, "Get local socket information." },
    { "open", (PyCFunction)UDP_func_open, METH_VARARGS, "Open the specified file descriptor and manage it as a UDP handle." },
    { "set_membership", (PyCFunction)UDP_func_set_membership, METH_VARARGS, "Set membership for multicast address." },
    { "set_multicast_ttl", (PyCFunction)UDP_func_set_multicast_ttl, METH_VARARGS, "Set the multicast TTL." },
    { "set_multicast_loop", (PyCFunction)UDP_func_set_multicast_loop, METH_VARARGS, "Set IP multicast loop flag. Makes multicast packets loop back to local sockets." },
    { "set_broadcast", (PyCFunction)UDP_func_set_broadcast, METH_VARARGS, "Set broadcast on or off." },
    { "set_ttl", (PyCFunction)UDP_func_set_ttl, METH_VARARGS, "Set the Time To Live." },
    { NULL }
};


static PyTypeObject UDPType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.UDP",                                                     /*tp_name*/
    sizeof(UDP),                                                    /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    0,                                                              /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)UDP_tp_traverse,                                  /*tp_traverse*/
    (inquiry)UDP_tp_clear,                                          /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    UDP_tp_methods,                                                 /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)UDP_tp_init,                                          /*tp_init*/
    0,                                                              /*tp_alloc*/
    UDP_tp_new,                                                     /*tp_new*/
};

