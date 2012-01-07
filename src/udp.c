
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
    ASSERT(handle);
    UDP *self = (UDP *)handle->data;
    ASSERT(self);

    PyObject *result;

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
    ASSERT(suggested_size <= UDP_MAX_BUF_SIZE);
    uv_buf_t buf = uv_buf_init(PyMem_Malloc(suggested_size), suggested_size);
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

    PyObject *result, *address_tuple, *data, *py_errorno;

    ASSERT(handle);
    ASSERT(flags == 0);

    UDP *self = (UDP *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread > 0) {
        ASSERT(addr);
        if (addr->sa_family == AF_INET) {
            addr4 = *(struct sockaddr_in*)addr;
            r = uv_ip4_name(&addr4, ip4, INET_ADDRSTRLEN);
            ASSERT(r == 0);
            address_tuple = Py_BuildValue("(si)", ip4, ntohs(addr4.sin_port));
        } else {
            addr6 = *(struct sockaddr_in6*)addr;
            r = uv_ip6_name(&addr6, ip6, INET6_ADDRSTRLEN);
            ASSERT(r == 0);
            address_tuple = Py_BuildValue("(si)", ip6, ntohs(addr6.sin6_port));
        }
        data = PyString_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        address_tuple = Py_None;
        Py_INCREF(Py_None);
        data = Py_None;
        Py_INCREF(Py_None);
        uv_err_t err = uv_last_error(UV_LOOP(self));
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
    int i;

    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    udp_req_data_t* req_data = (udp_req_data_t *)req->data;
    udp_send_data_t* send_data = (udp_send_data_t *)req_data->data;

    UDP *self = (UDP *)req_data->obj;
    PyObject *callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result, *py_errorno;

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
    req->data = NULL;
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
UDP_func_bind(UDP *self, PyObject *args)
{
    int r = 0;
    char *bind_ip;
    int bind_port;
    int address_type;
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

    if (inet_pton(AF_INET, bind_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (inet_pton(AF_INET6, bind_ip, &addr6) == 1) {
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
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;

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
    int r = 0;

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
    int i, n;
    int r = 0;
    int buf_count = 0;
    char *data_str;
    char *tmp;
    Py_ssize_t data_len;
    char *dest_ip;
    int dest_port;
    int address_type;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs;
    uv_udp_send_t *wr = NULL;
    udp_req_data_t *req_data = NULL;
    udp_send_data_t *send_data = NULL;
    PyObject *item;
    PyObject *data;
    PyObject *address_tuple;
    PyObject *callback = Py_None;

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

    if (inet_pton(AF_INET, dest_ip, &addr4) == 1) {
        address_type = AF_INET;
    } else if (inet_pton(AF_INET6, dest_ip, &addr6) == 1) {
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
        // We have a single string
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
        // We have a list
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
        req_data->obj = NULL;
        req_data->callback = NULL;
        req_data->data = NULL;
        PyMem_Free(req_data);
    }
    if (wr) {
        wr->data = NULL;
        PyMem_Free(wr);
    }
    return NULL;
}


static PyObject *
UDP_func_set_membership(UDP *self, PyObject *args)
{
    int r = 0;
    char *multicast_address;
    char *interface_address = NULL;
    int membership;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_UDPError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "si|:set_membership", &multicast_address, &membership, &interface_address)) {
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
    int r = 0;
    struct sockaddr sockname;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    int namelen = sizeof(sockname);

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
        r = uv_ip4_name(addr4, ip4, INET_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip4, ntohs(addr4->sin_port));
    } else if (sockname.sa_family == AF_INET6) {
        addr6 = (struct sockaddr_in6*)&sockname;
        r = uv_ip6_name(addr6, ip6, INET6_ADDRSTRLEN);
        ASSERT(r == 0);
        return Py_BuildValue("si", ip6, ntohs(addr6->sin6_port));
    } else {
        PyErr_SetString(PyExc_UDPError, "unknown address type detected");
        return NULL;
    }
}


static int
UDP_tp_init(UDP *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_udp_t *uv_udp_handle = NULL;

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


