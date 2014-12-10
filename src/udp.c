
typedef struct {
    uv_udp_send_t req;
    PyObject *callback;
    Py_buffer *views;
    Py_buffer viewsml[4];
    int view_count;
} udp_send_ctx;


static void
pyuv__udp_recv_cd(uv_udp_t* handle, int nread, const uv_buf_t* buf, struct sockaddr* addr, unsigned flags)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Loop *loop;
    UDP *self;
    PyObject *result, *address_tuple, *data, *py_errorno;

    ASSERT(handle);
    ASSERT(flags == 0);

    self = PYUV_CONTAINER_OF(handle, UDP, udp_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread == 0 && addr == NULL) {
        /* libuv got EAGAIN and is returning the buffer to us */
        goto done;
    }

    if (nread >= 0) {
        ASSERT(addr);
        address_tuple = makesockaddr(addr);
        if (nread == 0) {
            data = PyBytes_FromString("");
        } else {
            data = PyBytes_FromStringAndSize(buf->base, nread);
        }
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else {
        address_tuple = Py_None;
        Py_INCREF(Py_None);
        data = Py_None;
        Py_INCREF(Py_None);
        py_errorno = PyInt_FromLong((long)nread);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, address_tuple, PyInt_FromLong((long)flags), data, py_errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(address_tuple);
    Py_DECREF(data);
    Py_DECREF(py_errorno);

done:
    /* data has been read, unlock the buffer */
    loop = handle->loop->data;
    ASSERT(loop);
    loop->buffer.in_use = False;

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
pyuv__udp_send_cb(uv_udp_send_t* req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int i;
    udp_send_ctx *ctx;
    UDP *self;
    PyObject *callback, *result, *py_errorno;

    ASSERT(req);

    ctx = PYUV_CONTAINER_OF(req, udp_send_ctx, req);
    self = PYUV_CONTAINER_OF(req->handle, UDP, udp_h);
    callback = ctx->callback;

    ASSERT(self);

    if (callback != Py_None) {
        if (status < 0) {
            py_errorno = PyInt_FromLong((long)status);
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

    for (i = 0; i < ctx->view_count; i++)
        PyBuffer_Release(&ctx->views[i]);
    if (ctx->views != ctx->viewsml)
        PyMem_Free(ctx->views);
    PyMem_Free(ctx);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
UDP_func_bind(UDP *self, PyObject *args)
{
    int err, flags;
    struct sockaddr_storage ss;
    PyObject *addr;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    flags = 0;

    if (!PyArg_ParseTuple(args, "O|i:bind", &addr, &flags)) {
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &ss) < 0) {
        /* Error is set by the function itself */
        return NULL;
    }

    err = uv_udp_bind(&self->udp_h, (struct sockaddr *)&ss, flags);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_start_recv(UDP *self, PyObject *args)
{
    int err;
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

    err = uv_udp_recv_start(&self->udp_h, (uv_alloc_cb)pyuv__alloc_cb, (uv_udp_recv_cb)pyuv__udp_recv_cd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    tmp = self->on_read_cb;
    Py_INCREF(callback);
    self->on_read_cb = callback;
    Py_XDECREF(tmp);

    PYUV_HANDLE_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_stop_recv(UDP *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_udp_recv_stop(&self->udp_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_XDECREF(self->on_read_cb);
    self->on_read_cb = NULL;

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_try_send(UDP *self, PyObject *args)
{
    int err;
    uv_buf_t buf;
    Py_buffer view;
    PyObject *addr;
    struct sockaddr_storage ss;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O"PYUV_BYTES"*:try_send", &addr, &view)) {
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &ss) < 0) {
        /* Error is set by the function itself */
        PyBuffer_Release(&view);
        return NULL;
    }

    buf = uv_buf_init(view.buf, view.len);
    err = uv_udp_try_send(&self->udp_h, &buf, 1, (struct sockaddr*) &ss);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        PyBuffer_Release(&view);
        return NULL;
    }

    PyBuffer_Release(&view);
    return PyInt_FromLong((long)err);
}


static PyObject *
pyuv__udp_send_bytes(UDP *self, struct sockaddr *addr, PyObject *data, PyObject *callback)
{
    int err;
    uv_buf_t buf;
    udp_send_ctx *ctx;
    Py_buffer *view;

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    ctx->views = ctx->viewsml;
    view = &ctx->views[0];

    if (PyObject_GetBuffer(data, view, PyBUF_SIMPLE) != 0) {
        PyMem_Free(ctx);
        return NULL;
    }

    ctx->view_count = 1;
    ctx->callback = callback;

    Py_INCREF(callback);

    buf = uv_buf_init(view->buf, view->len);

    err = uv_udp_send(&ctx->req, &self->udp_h, &buf, 1, addr, (uv_udp_send_cb)pyuv__udp_send_cb);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        Py_DECREF(callback);
        PyBuffer_Release(view);
        PyMem_Free(ctx);
        return NULL;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
pyuv__udp_send_sequence(UDP *self, struct sockaddr *addr, PyObject *data, PyObject *callback)
{
    int err;
    udp_send_ctx *ctx;
    PyObject *data_fast, *item;
    Py_ssize_t i, j, buf_count;

    data_fast = PySequence_Fast(data, "data must be an iterable");
    if (data_fast == NULL)
        return NULL;

    buf_count = PySequence_Fast_GET_SIZE(data_fast);
    if (buf_count > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "iterable is too long");
        Py_DECREF(data_fast);
        return NULL;
    }

    if (buf_count == 0) {
        PyErr_SetString(PyExc_ValueError, "iterable is empty");
        Py_DECREF(data_fast);
        return NULL;
    }

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        Py_DECREF(data_fast);
        return NULL;
    }

    ctx->views = ctx->viewsml;
    if (buf_count > ARRAY_SIZE(ctx->viewsml))
        ctx->views = PyMem_Malloc(sizeof(Py_buffer) * buf_count);
    if (!ctx->views) {
        PyErr_NoMemory();
        PyMem_Free(ctx);
        Py_DECREF(data_fast);
        return NULL;
    }
    ctx->view_count = buf_count;

    {
        STACK_ARRAY(uv_buf_t, bufs, buf_count);

        for (i = 0; i < buf_count; i++) {
            item = PySequence_Fast_GET_ITEM(data_fast, i);
            if (PyObject_GetBuffer(item, &ctx->views[i], PyBUF_SIMPLE) != 0)
                goto error;
            bufs[i].base = ctx->views[i].buf;
            bufs[i].len = ctx->views[i].len;
        }

        ctx->callback = callback;
        Py_INCREF(callback);

        err = uv_udp_send(&ctx->req, &self->udp_h, bufs, buf_count, addr, (uv_udp_send_cb)pyuv__udp_send_cb);
    }

    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        Py_DECREF(callback);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    for (j = 0; j < i; j++)
        PyBuffer_Release(&ctx->views[j]);
    if (ctx->views != ctx->viewsml)
        PyMem_Free(ctx->views);
    PyMem_Free(ctx);
    Py_XDECREF(data_fast);
    return NULL;
}


static PyObject *
UDP_func_send(UDP *self, PyObject *args)
{
    PyObject *addr, *callback, *data;
    struct sockaddr_storage ss;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    callback = Py_None;

    if (!PyArg_ParseTuple(args, "OO|O:send", &addr, &data, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "'callback' must be a callable or None");
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &ss) < 0) {
        /* Error is set by the function itself */
        return NULL;
    }

    if (PyObject_CheckBuffer(data)) {
        return pyuv__udp_send_bytes(self, (struct sockaddr*) &ss, data, callback);
    } else if (!PyUnicode_Check(data) && PySequence_Check(data)) {
        return pyuv__udp_send_sequence(self, (struct sockaddr*) &ss, data, callback);
    } else {
        PyErr_SetString(PyExc_TypeError, "only bytes and sequences are supported");
        return NULL;
    }
}


static PyObject *
UDP_func_set_membership(UDP *self, PyObject *args)
{
    int err, membership;
    char *multicast_address, *interface_address;

    interface_address = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "si|s:set_membership", &multicast_address, &membership, &interface_address)) {
        return NULL;
    }

    err = uv_udp_set_membership(&self->udp_h, multicast_address, interface_address, membership);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_multicast_interface(UDP *self, PyObject *args)
{
    int err;
    char *interface_address;

    interface_address = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "|s:set_multicast_interface", &interface_address)) {
        return NULL;
    }

    err = uv_udp_set_multicast_interface(&self->udp_h, interface_address);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_getsockname(UDP *self)
{
    int err, namelen;
    struct sockaddr_storage sockname;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    namelen = sizeof(sockname);

    err = uv_udp_getsockname(&self->udp_h, (struct sockaddr *)&sockname, &namelen);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    return makesockaddr((struct sockaddr *)&sockname);
}


static PyObject *
UDP_func_set_multicast_ttl(UDP *self, PyObject *args)
{
    int err, ttl;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    err = uv_udp_set_multicast_ttl(&self->udp_h, ttl);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_broadcast(UDP *self, PyObject *args)
{
    int err;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:set_broadcast", &PyBool_Type, &enable)) {
        return NULL;
    }

    err = uv_udp_set_broadcast(&self->udp_h, (enable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_multicast_loop(UDP *self, PyObject *args)
{
    int err;
    PyObject *enable;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O!:set_multicast_loop", &PyBool_Type, &enable)) {
        return NULL;
    }

    err = uv_udp_set_multicast_loop(&self->udp_h, (enable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_set_ttl(UDP *self, PyObject *args)
{
    int err, ttl;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:set_multicast_ttl", &ttl)) {
        return NULL;
    }

    if (ttl < 0 || ttl > 255) {
        PyErr_SetString(PyExc_ValueError, "ttl must be between 0 and 255");
        return NULL;
    }

    err = uv_udp_set_ttl(&self->udp_h, ttl);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
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

    uv_udp_open(&self->udp_h, (uv_os_sock_t)fd);

    Py_RETURN_NONE;
}


static PyObject *
UDP_func_fileno(UDP *self)
{
    int err;
    uv_os_fd_t fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_fileno(UV_HANDLE(self), &fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }

    /* This is safe. See note in Stream_func_fileno() */
    return PyInt_FromLong((long) fd);
}


static PyObject *
UDP_sndbuf_get(UDP *self, void *closure)
{
    int err;
    int sndbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    sndbuf_value = 0;
    err = uv_send_buffer_size(UV_HANDLE(self), &sndbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }
    return PyInt_FromLong((long) sndbuf_value);
}


static int
UDP_sndbuf_set(UDP *self, PyObject *value, void *closure)
{
    int err;
    int sndbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, -1);

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }

    sndbuf_value = (int) PyInt_AsLong(value);
    if (sndbuf_value == -1 && PyErr_Occurred()) {
        return -1;
    }

    err = uv_send_buffer_size(UV_HANDLE(self), &sndbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return -1;
    }
    return 0;
}


static PyObject *
UDP_rcvbuf_get(UDP *self, void *closure)
{
    int err;
    int rcvbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    rcvbuf_value = 0;
    err = uv_recv_buffer_size(UV_HANDLE(self), &rcvbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return NULL;
    }
    return PyInt_FromLong((long) rcvbuf_value);
}


static int
UDP_rcvbuf_set(UDP *self, PyObject *value, void *closure)
{
    int err;
    int rcvbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, -1);

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }

    rcvbuf_value = (int) PyInt_AsLong(value);
    if (rcvbuf_value == -1 && PyErr_Occurred()) {
        return -1;
    }

    err = uv_recv_buffer_size(UV_HANDLE(self), &rcvbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return -1;
    }
    return 0;
}


static int
UDP_tp_init(UDP *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_udp_init(loop->uv_loop, &self->udp_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UDPError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
UDP_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    UDP *self;

    self = (UDP *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->udp_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->udp_h;

    return (PyObject *)self;
}


static int
UDP_tp_traverse(UDP *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_read_cb);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
UDP_tp_clear(UDP *self)
{
    Py_CLEAR(self->on_read_cb);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
UDP_tp_methods[] = {
    { "bind", (PyCFunction)UDP_func_bind, METH_VARARGS, "Bind to the specified IP and port." },
    { "start_recv", (PyCFunction)UDP_func_start_recv, METH_VARARGS, "Start accepting data." },
    { "stop_recv", (PyCFunction)UDP_func_stop_recv, METH_NOARGS, "Stop receiving data." },
    { "try_send", (PyCFunction)UDP_func_try_send, METH_VARARGS, "Try to send data over UDP." },
    { "send", (PyCFunction)UDP_func_send, METH_VARARGS, "Send data over UDP." },
    { "getsockname", (PyCFunction)UDP_func_getsockname, METH_NOARGS, "Get local socket information." },
    { "open", (PyCFunction)UDP_func_open, METH_VARARGS, "Open the specified file descriptor and manage it as a UDP handle." },
    { "set_membership", (PyCFunction)UDP_func_set_membership, METH_VARARGS, "Set membership for multicast address." },
    { "set_multicast_interface", (PyCFunction)UDP_func_set_multicast_interface, METH_VARARGS, "Set the multicast interface." },
    { "set_multicast_ttl", (PyCFunction)UDP_func_set_multicast_ttl, METH_VARARGS, "Set the multicast TTL." },
    { "set_multicast_loop", (PyCFunction)UDP_func_set_multicast_loop, METH_VARARGS, "Set IP multicast loop flag. Makes multicast packets loop back to local sockets." },
    { "set_broadcast", (PyCFunction)UDP_func_set_broadcast, METH_VARARGS, "Set broadcast on or off." },
    { "set_ttl", (PyCFunction)UDP_func_set_ttl, METH_VARARGS, "Set the Time To Live." },
    { "fileno", (PyCFunction)UDP_func_fileno, METH_NOARGS, "Returns the libuv OS handle." },
    { NULL }
};


static PyGetSetDef UDP_tp_getsets[] = {
    {"send_buffer_size", (getter)UDP_sndbuf_get, (setter)UDP_sndbuf_set, "Send buffer size.", NULL},
    {"receive_buffer_size", (getter)UDP_rcvbuf_get, (setter)UDP_rcvbuf_set, "Receive buffer size.", NULL},
    {NULL}
};


static PyTypeObject UDPType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.UDP",                                              /*tp_name*/
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
    UDP_tp_getsets,                                                 /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)UDP_tp_init,                                          /*tp_init*/
    0,                                                              /*tp_alloc*/
    UDP_tp_new,                                                     /*tp_new*/
};

