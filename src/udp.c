
typedef struct {
    PyObject *callback;
    uv_buf_t *bufs;
    int buf_count;
    Py_buffer view;
} udp_send_data_t;


static uv_buf_t
on_udp_alloc(uv_udp_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf = uv_buf_init(PyMem_Malloc(suggested_size), suggested_size);
    UNUSED_ARG(handle);
    PyGILState_Release(gstate);
    return buf;
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
        data = PyString_FromStringAndSize(buf.base, nread);
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
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);
    Py_DECREF(address_tuple);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

done:
    /* In case of error libuv may not call alloc_cb */
    if (buf.base != NULL) {
        PyMem_Free(buf.base);
    }

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
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

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
            PyErr_WriteUnraisable(callback);
        }
        Py_XDECREF(result);
        Py_DECREF(py_errorno);
    }

    if (req_data->buf_count == 1) {
        PyBuffer_Release(&req_data->view);
    } else {
        for (i = 0; i < req_data->buf_count; i++) {
            PyMem_Free(req_data->bufs[i].base);
        }
        PyMem_Free(req_data->bufs);
    }
    Py_DECREF(callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
UDP_func_bind(UDP *self, PyObject *args)
{
    int r, bind_port, address_type;
    char *bind_ip;
    struct in_addr addr4;
    struct in6_addr addr6;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "(si):bind", &bind_ip, &bind_port)) {
        return NULL;
    }

    if (bind_port < 0 || bind_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }

    if (uv_inet_pton(AF_INET, bind_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (uv_inet_pton(AF_INET6, bind_ip, &addr6) == 1) {
        address_type = AF_INET6;
    } else {
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
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_buf_t buf;
    Py_buffer pbuf;
    PyObject *callback;
    uv_udp_send_t *wr = NULL;
    udp_send_data_t *req_data = NULL;

    callback = Py_None;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "(si)s*|O:send", &dest_ip, &dest_port, &pbuf, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    if (dest_port < 0 || dest_port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }

    if (uv_inet_pton(AF_INET, dest_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (uv_inet_pton(AF_INET6, dest_ip, &addr6) == 1) {
        address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    Py_INCREF(callback);

    wr = (uv_udp_send_t *)PyMem_Malloc(sizeof(uv_udp_send_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (udp_send_data_t*) PyMem_Malloc(sizeof(udp_send_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    buf = uv_buf_init(pbuf.buf, pbuf.len);
    req_data->callback = callback;
    req_data->bufs = &buf;
    req_data->buf_count = 1;
    req_data->view = pbuf;

    wr->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_udp_send(wr, (uv_udp_t *)UV_HANDLE(self), &buf, 1, uv_ip4_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(wr, (uv_udp_t *)UV_HANDLE(self), &buf, 1, uv_ip6_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    }
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    PyBuffer_Release(&pbuf);
    Py_DECREF(callback);
    if (req_data) {
        PyMem_Free(req_data);
    }
    if (wr) {
        PyMem_Free(wr);
    }
    return NULL;
}


static PyObject *
UDP_func_sendlines(UDP *self, PyObject *args)
{
    int i, r, buf_count, dest_port, address_type;
    char *dest_ip;
    struct in_addr addr4;
    struct in6_addr addr6;
    PyObject *callback, *seq;
    uv_buf_t *bufs;
    uv_udp_send_t *wr = NULL;
    udp_send_data_t *req_data = NULL;

    callback = Py_None;

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

    if (uv_inet_pton(AF_INET, dest_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (uv_inet_pton(AF_INET6, dest_ip, &addr6) == 1) {
        address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    Py_INCREF(callback);

    r = pyseq2uvbuf(seq, &bufs, &buf_count);
    if (r != 0) {
        /* error is already set */
        goto error;
    }

    wr = (uv_udp_send_t *)PyMem_Malloc(sizeof(uv_udp_send_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (udp_send_data_t*) PyMem_Malloc(sizeof(udp_send_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->callback = callback;
    req_data->bufs = bufs;
    req_data->buf_count = buf_count;
    wr->data = (void *)req_data;

    if (address_type == AF_INET) {
        r = uv_udp_send(wr, (uv_udp_t *)UV_HANDLE(self), bufs, buf_count, uv_ip4_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(wr, (uv_udp_t *)UV_HANDLE(self), bufs, buf_count, uv_ip6_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    }
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    Py_DECREF(callback);
    if (bufs) {
        for (i = 0; i < buf_count; i++) {
            PyMem_Free(bufs[i].base);
        }
        PyMem_Free(bufs);
    }
    if (req_data) {
        PyMem_Free(req_data);
    }
    if (wr) {
        PyMem_Free(wr);
    }
    return NULL;
}


static PyObject *
UDP_func_set_membership(UDP *self, PyObject *args)
{
    int r, membership;
    char *multicast_address, *interface_address;

    interface_address = NULL;

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


static int
UDP_tp_init(UDP *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_udp_t *uv_udp_handle = NULL;
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_UDPError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    uv_udp_handle = PyMem_Malloc(sizeof(uv_udp_t));
    if (!uv_udp_handle) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }
    r = uv_udp_init(UV_HANDLE_LOOP(self), uv_udp_handle);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        Py_DECREF(loop);
        return -1;
    }
    uv_udp_handle->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_udp_handle;

    return 0;
}


static PyObject *
UDP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    UDP *self = (UDP *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
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


