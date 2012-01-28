
#define UDP_MAX_BUF_SIZE 65536

static PyObject* PyExc_UDPError;


typedef struct {
    PyObject *obj;
    PyObject *callback;
    void *data;
} udp_req_data_t;

typedef struct {
    uv_buf_t *bufs;
    int buf_count;
} udp_send_data_t;


static void
on_udp_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    UDP *self;
    PyObject *result;

    ASSERT(handle);
    self = (UDP *)handle->data;
    ASSERT(self);

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_close_cb);
        }
        Py_XDECREF(result);
    }

    handle->data = NULL;
    PyMem_Free(handle);

    /* Refcount was increased in func_close */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_udp_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static uv_buf_t
on_udp_alloc(uv_udp_t* handle, size_t suggested_size)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_buf_t buf;
    ASSERT(suggested_size <= UDP_MAX_BUF_SIZE);
    buf = uv_buf_init(PyMem_Malloc(suggested_size), suggested_size);
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
        err = uv_last_error(UV_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    if (nread != 0) {
        result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, address_tuple, data, py_errorno, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_read_cb);
        }
        Py_XDECREF(result);
    }

    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_udp_send(uv_udp_send_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    udp_req_data_t* req_data;
    udp_send_data_t* send_data;
    UDP *self;
    PyObject *callback, *result, *py_errorno;

    ASSERT(req);

    req_data = (udp_req_data_t *)req->data;
    send_data = (udp_send_data_t *)req_data->data;

    self = (UDP *)req_data->obj;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (callback != Py_None) {
        if (status < 0) {
            uv_err_t err = uv_last_error(UV_LOOP(self));
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
    }

    for (i = 0; i < send_data->buf_count; i++) {
        PyMem_Free(send_data->bufs[i].base);
    }
    PyMem_Free(send_data);
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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "(si):bind", &bind_ip, &bind_port)) {
        return NULL;
    }

    if (bind_port < 0 || bind_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
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
        r = uv_udp_bind(self->uv_handle, uv_ip4_addr(bind_ip, bind_port), 0);
    } else {
        r = uv_udp_bind6(self->uv_handle, uv_ip6_addr(bind_ip, bind_port), UV_UDP_IPV6ONLY);
    }

    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start_recv", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_udp_recv_start(self->uv_handle, (uv_alloc_cb)on_udp_alloc, (uv_udp_recv_cb)on_udp_read);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    r = uv_udp_recv_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
UDP_func_send(UDP *self, PyObject *args)
{
    int i, n, r, buf_count, dest_port, address_type;
    char *data_str, *tmp, *dest_ip;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs;
    uv_udp_send_t *wr = NULL;
    udp_req_data_t *req_data = NULL;
    udp_send_data_t *send_data = NULL;
    Py_ssize_t data_len;
    PyObject *item, *data, *address_tuple, *callback;

    buf_count = 0;
    callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "OO|O:send", &data, &address_tuple, &callback)) {
        return NULL;
    }

    if (!PySequence_Check(data) || PyUnicode_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "only strings and iterables are supported");
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    if (!PyArg_ParseTuple(address_tuple, "si", &dest_ip, &dest_port)) {
        return NULL;
    }

    if (dest_port < 0 || dest_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
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

    wr = (uv_udp_send_t *) PyMem_Malloc(sizeof(uv_udp_send_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (udp_req_data_t *) PyMem_Malloc(sizeof(udp_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    send_data = (udp_send_data_t *) PyMem_Malloc(sizeof(udp_send_data_t));
    if (!send_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;
    wr->data = (void *)req_data;

    if (PyString_Check(data)) {
        /* We have a single string */
        bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t));
        if (!bufs) {
            PyErr_NoMemory();
            goto error;
        }
        data_len = PyString_Size(data);
        data_str = PyString_AsString(data);
        tmp = (char *) PyMem_Malloc(data_len + 1);
        if (!tmp) {
            PyMem_Free(bufs);
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, data_str, data_len + 1);
        tmpbuf = uv_buf_init(tmp, data_len);
        bufs[0] = tmpbuf;
        buf_count = 1;
    } else {
        /* We have a list */
        buf_count = 0;
        n = PySequence_Length(data);
        bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t) * n);
        if (!bufs) {
            PyErr_NoMemory();
            goto error;
        }
        for (i = 0;i < n; i++) {
            item = PySequence_GetItem(data, i);
            if (!item || !PyString_Check(item))
                continue;
            data_len = PyString_Size(item);
            data_str = PyString_AsString(item);
            tmp = (char *) PyMem_Malloc(data_len + 1);
            if (!tmp)
                continue;
            memcpy(tmp, data_str, data_len + 1);
            tmpbuf = uv_buf_init(tmp, data_len);
            bufs[i] = tmpbuf;
            buf_count++;
        }

    }

    send_data->bufs = bufs;
    send_data->buf_count = buf_count;
    req_data->data = (void *)send_data;

    if (address_type == AF_INET) {
        r = uv_udp_send(wr, self->uv_handle, bufs, buf_count, uv_ip4_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(wr, self->uv_handle, bufs, buf_count, uv_ip6_addr(dest_ip, dest_port), (uv_udp_send_cb)on_udp_send);
    }
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        goto error;
    }
    Py_RETURN_NONE;

error:
    if (send_data) {
        PyMem_Free(send_data);
    }
    if (req_data) {
        Py_DECREF(callback);
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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "si|s:set_membership", &multicast_address, &membership, &interface_address)) {
        return NULL;
    }

    r = uv_udp_set_membership(self->uv_handle, multicast_address, interface_address, membership);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_close(UDP *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "UDP is already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:close", &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    Py_INCREF(callback);
    self->on_close_cb = callback;

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    uv_close((uv_handle_t *)self->uv_handle, on_udp_close);
    self->uv_handle = NULL;

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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    r = uv_udp_getsockname(self->uv_handle, &sockname, &namelen);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
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

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    r = uv_udp_set_multicast_ttl(self->uv_handle, ttl);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_broadcast(UDP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!:set_broadcast", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_udp_set_broadcast(self->uv_handle, (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_multicast_loop(UDP *self, PyObject *args)
{
    int r;
    PyObject *enable;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O!:set_multicast_loop", &PyBool_Type, &enable)) {
        return NULL;
    }

    r = uv_udp_set_multicast_loop(self->uv_handle, (enable == Py_True) ? 1 : 0);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_ttl(UDP *self, PyObject *args)
{
    int r, ttl;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    r = uv_udp_set_ttl(self->uv_handle, ttl);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
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

    if (self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_udp_handle = PyMem_Malloc(sizeof(uv_udp_t));
    if (!uv_udp_handle) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }
    r = uv_udp_init(UV_LOOP(self), uv_udp_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_UDPError);
        Py_DECREF(loop);
        return -1;
    }
    uv_udp_handle->data = (void *)self;
    self->uv_handle = uv_udp_handle;

    return 0;
}


static PyObject *
UDP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    UDP *self = (UDP *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
UDP_tp_traverse(UDP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->on_read_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
UDP_tp_clear(UDP *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->on_read_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
UDP_tp_dealloc(UDP *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_udp_dealloc_close);
        self->uv_handle = NULL;
    }
    UDP_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
UDP_tp_methods[] = {
    { "bind", (PyCFunction)UDP_func_bind, METH_VARARGS, "Bind to the specified IP and port." },
    { "start_recv", (PyCFunction)UDP_func_start_recv, METH_VARARGS, "Start accepting data." },
    { "stop_recv", (PyCFunction)UDP_func_stop_recv, METH_NOARGS, "Stop receiving data." },
    { "send", (PyCFunction)UDP_func_send, METH_VARARGS, "Send data over UDP." },
    { "close", (PyCFunction)UDP_func_close, METH_VARARGS, "Close UDP connection." },
    { "getsockname", (PyCFunction)UDP_func_getsockname, METH_NOARGS, "Get local socket information." },
    { "set_membership", (PyCFunction)UDP_func_set_membership, METH_VARARGS, "Set membership for multicast address." },
    { "set_multicast_ttl", (PyCFunction)UDP_func_set_multicast_ttl, METH_VARARGS, "Set the multicast TTL." },
    { "set_multicast_loop", (PyCFunction)UDP_func_set_multicast_loop, METH_VARARGS, "Set IP multicast loop flag. Makes multicast packets loop back to local sockets." },
    { "set_broadcast", (PyCFunction)UDP_func_set_broadcast, METH_VARARGS, "Set broadcast on or off." },
    { "set_ttl", (PyCFunction)UDP_func_set_ttl, METH_VARARGS, "Set the Time To Live." },
    { NULL }
};


static PyMemberDef UDP_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(UDP, loop), READONLY, "Loop where this UDP is running on."},
    {"data", T_OBJECT, offsetof(UDP, data), 0, "Arbitrary data."},
    {NULL}
};


static PyTypeObject UDPType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.UDP",                                                     /*tp_name*/
    sizeof(UDP),                                                    /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)UDP_tp_dealloc,                                     /*tp_dealloc*/
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
    UDP_tp_members,                                                 /*tp_members*/
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


