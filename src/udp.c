
typedef struct {
    uv_udp_send_t req;
    PyObject *callback;
    Py_buffer *views;
    Py_buffer view[1];
    int view_count;
} udp_send_ctx;


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
    int addrlen;
    uv_err_t err;
    UDP *self;
    PyObject *result, *address_tuple, *data, *py_errorno;

    ASSERT(handle);
    ASSERT(flags == 0);

    self = PYUV_CONTAINER_OF(handle, UDP, udp_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (nread == 0) {
        goto done;
    }

    if (nread > 0) {
        ASSERT(addr);
        addrlen = sizeof(*addr);
        address_tuple = makesockaddr(addr, addrlen);
        data = PyBytes_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else {
        address_tuple = Py_None;
        Py_INCREF(Py_None);
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
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
    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_udp_send(uv_udp_send_t* req, int status)
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
    for (i = 0; i < ctx->view_count; i++) {
        PyBuffer_Release(&ctx->views[i]);
    }
    if (ctx->views != ctx->view) {
        PyMem_Free(ctx->views);
    }
    PyMem_Free(ctx);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
UDP_func_bind(UDP *self, PyObject *args)
{
    int r, flags;
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

    if (ss.ss_family == AF_INET) {
        r = uv_udp_bind(&self->udp_h, *(struct sockaddr_in *)&ss, flags);
    } else {
        r = uv_udp_bind6(&self->udp_h, *(struct sockaddr_in6 *)&ss, flags);
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

    r = uv_udp_recv_start(&self->udp_h, (uv_alloc_cb)on_udp_alloc, (uv_udp_recv_cb)on_udp_read);
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

    r = uv_udp_recv_stop(&self->udp_h);
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
    int r;
    struct sockaddr_storage ss;
    uv_buf_t buf;
    Py_buffer *view;
    PyObject *addr, *callback;
    udp_send_ctx *ctx;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    callback = Py_None;

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    view = &ctx->view[0];

    if (!PyArg_ParseTuple(args, "O"PYUV_BYTES"*|O:send", &addr, view, &callback)) {
        PyMem_Free(ctx);
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        goto error;
    }

    if (pyuv_parse_addr_tuple(addr, &ss) < 0) {
        /* Error is set by the function itself */
        goto error;
    }

    Py_INCREF(callback);

    buf = uv_buf_init(view->buf, view->len);
    ctx->callback = callback;
    ctx->view_count = 1;
    ctx->views = view;

    if (ss.ss_family == AF_INET) {
        r = uv_udp_send(&ctx->req, &self->udp_h, &buf, 1, *(struct sockaddr_in *)&ss, (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(&ctx->req, &self->udp_h, &buf, 1, *(struct sockaddr_in6 *)&ss, (uv_udp_send_cb)on_udp_send);
    }
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        Py_DECREF(callback);
        goto error;
    }

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;

error:
    PyBuffer_Release(view);
    PyMem_Free(ctx);
    return NULL;
}


static PyObject *
UDP_func_sendlines(UDP *self, PyObject *args)
{
    int i, r, buf_count;
    struct sockaddr_storage ss;
    PyObject *addr, *callback, *seq;
    Py_buffer *views;
    uv_buf_t *bufs;
    udp_send_ctx *ctx;

    callback = Py_None;
    ctx = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "OO|O:sendlines", &addr, &seq, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    if (pyuv_parse_addr_tuple(addr, &ss) < 0) {
        /* Error is set by the function itself */
        return NULL;
    }

    r = pyseq2uvbuf(seq, &views, &bufs, &buf_count);
    if (r != 0) {
        /* error is already set */
        return NULL;
    }

    Py_INCREF(callback);

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        goto error;
    }

    ctx->callback = callback;
    ctx->view_count = buf_count;
    ctx->views = views;

    if (ss.ss_family == AF_INET) {
        r = uv_udp_send(&ctx->req, &self->udp_h, bufs, buf_count, *(struct sockaddr_in *)&ss, (uv_udp_send_cb)on_udp_send);
    } else {
        r = uv_udp_send6(&ctx->req, &self->udp_h, bufs, buf_count, *(struct sockaddr_in6 *)&ss, (uv_udp_send_cb)on_udp_send);
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
    PyMem_Free(ctx);
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

    r = uv_udp_set_membership(&self->udp_h, multicast_address, interface_address, membership);
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
    struct sockaddr_storage sockname;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    namelen = sizeof(sockname);

    r = uv_udp_getsockname(&self->udp_h, (struct sockaddr *)&sockname, &namelen);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UDPError);
        return NULL;
    }

    return makesockaddr((struct sockaddr *)&sockname, namelen);
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

    r = uv_udp_set_multicast_ttl(&self->udp_h, ttl);
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

    r = uv_udp_set_broadcast(&self->udp_h, (enable == Py_True) ? 1 : 0);
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

    r = uv_udp_set_multicast_loop(&self->udp_h, (enable == Py_True) ? 1 : 0);
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

    r = uv_udp_set_ttl(&self->udp_h, ttl);
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

    uv_udp_open(&self->udp_h, (uv_os_sock_t)fd);

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

    r = uv_udp_init(loop->uv_loop, &self->udp_h);
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

