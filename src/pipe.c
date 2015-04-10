
static void
pyuv__pipe_listen_cb(uv_stream_t* handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *result, *py_errorno;
    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, Pipe, pipe_h);

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
pyuv__pipe_connect_cb(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *callback, *result, *py_errorno;
    ASSERT(req);

    self = PYUV_CONTAINER_OF(req->handle, Pipe, pipe_h);
    callback = (PyObject *)req->data;

    ASSERT(self);

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
Pipe_func_bind(Pipe *self, PyObject *args)
{
    int err;
    char *name;
    Py_ssize_t len;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "s#:bind", &name, &len)) {
        return NULL;
    }

    err = uv_pipe_bind(&self->pipe_h, name);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_listen(Pipe *self, PyObject *args)
{
    int err, backlog;
    PyObject *callback, *tmp;

    backlog = 511;
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

    err = uv_listen((uv_stream_t *)&self->pipe_h, backlog, pyuv__pipe_listen_cb);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    tmp = self->on_new_connection_cb;
    Py_INCREF(callback);
    self->on_new_connection_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_accept(Pipe *self, PyObject *args)
{
    int err;
    PyObject *client;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:accept", &client)) {
        return NULL;
    }

    if (PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&StreamType)) {
        if (UV_HANDLE(client)->type != UV_TCP && UV_HANDLE(client)->type != UV_NAMED_PIPE) {
            PyErr_SetString(PyExc_TypeError, "Only TCP and Pipe objects are supported for accept");
            return NULL;
        }
    } else if (PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&UDPType)) {
        /* empty */
    } else {
        PyErr_SetString(PyExc_TypeError, "Only Stream and UDP objects are supported for accept");
        return NULL;
    }

    err = uv_accept((uv_stream_t *)&self->pipe_h, (uv_stream_t *)UV_HANDLE(client));
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_connect(Pipe *self, PyObject *args)
{
    char *name;
    uv_connect_t *connect_req = NULL;
    PyObject *callback;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "sO:connect", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    Py_INCREF(callback);

    connect_req = PyMem_Malloc(sizeof *connect_req);
    if (!connect_req) {
        Py_DECREF(callback);
        PyErr_NoMemory();
        return NULL;
    }

    connect_req->data = callback;

    uv_pipe_connect(connect_req, &self->pipe_h, name, pyuv__pipe_connect_cb);

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_open(Pipe *self, PyObject *args)
{
    int err;
    long fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "l:open", &fd)) {
        return NULL;
    }

    err = uv_pipe_open(&self->pipe_h, (uv_file)fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_pending_instances(Pipe *self, PyObject *args)
{
    /* This function applies to Windows only */
    int count;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:pending_instances", &count)) {
        return NULL;
    }

    uv_pipe_pending_instances(&self->pipe_h, count);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_pending_handle_type(Pipe *self)
{
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    return PyInt_FromLong(uv_pipe_pending_type(&self->pipe_h));
}


static PyObject *
Pipe_func_write(Pipe *self, PyObject *args)
{
    PyObject *data;
    PyObject *callback, *send_handle;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    callback = send_handle = Py_None;

    if (!PyArg_ParseTuple(args, "O|OO:write", &data, &callback, &send_handle)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "'callback' must be a callable or None");
        return NULL;
    }

    if (send_handle == Py_None) {
        send_handle = NULL;
    } else if (PyObject_IsSubclass((PyObject *)send_handle->ob_type, (PyObject *)&StreamType)) {
        if (UV_HANDLE(send_handle)->type != UV_TCP && UV_HANDLE(send_handle)->type != UV_NAMED_PIPE) {
            PyErr_SetString(PyExc_TypeError, "Only TCP and Pipe objects are supported");
            return NULL;
        }
    } else if (PyObject_IsSubclass((PyObject *)send_handle->ob_type, (PyObject *)&UDPType)) {
        /* empty */
    } else {
        PyErr_SetString(PyExc_TypeError, "Only Stream and UDP objects are supported");
        return NULL;
    }

    if (PyObject_CheckBuffer(data)) {
        return pyuv__stream_write_bytes((Stream *)self, data, callback, send_handle);
    } else if (!PyUnicode_Check(data) && PySequence_Check(data)) {
        return pyuv__stream_write_sequence((Stream *)self, data, callback, send_handle);
    } else {
        PyErr_SetString(PyExc_TypeError, "only bytes and sequences are supported");
        return NULL;
    }
}


static PyObject *
Pipe_func_getsockname(Pipe *self)
{
#ifdef _WIN32
    /* MAX_PATH is in characters, not bytes. Make sure we have enough headroom. */
    char buf[MAX_PATH * 4];
#else
    char buf[PATH_MAX];
#endif
    size_t buf_len;
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    buf_len = sizeof(buf);
    err = uv_pipe_getsockname(&self->pipe_h, buf, &buf_len);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    if (buf_len == 0) {
        return PyBytes_FromString("");
    } else if (buf[0] == '\0') {
        /* Linux abstract namespace */
        return PyBytes_FromStringAndSize(buf, buf_len);
    } else {
#ifdef PYUV_PYTHON3
        return PyUnicode_DecodeFSDefaultAndSize(buf, buf_len);
#else
        return PyBytes_FromStringAndSize(buf, buf_len);
#endif
    }

}


static PyObject *
Pipe_func_getpeername(Pipe *self)
{
#ifdef _WIN32
    /* MAX_PATH is in characters, not bytes. Make sure we have enough headroom. */
    char buf[MAX_PATH * 4];
#else
    char buf[PATH_MAX];
#endif
    size_t buf_len;
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    buf_len = sizeof(buf);
    err = uv_pipe_getpeername(&self->pipe_h, buf, &buf_len);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    if (buf_len == 0) {
        return PyBytes_FromString("");
    } else if (buf[0] == '\0') {
        /* Linux abstract namespace */
        return PyBytes_FromStringAndSize(buf, buf_len);
    } else {
#ifdef PYUV_PYTHON3
        return PyUnicode_DecodeFSDefaultAndSize(buf, buf_len);
#else
        return PyBytes_FromStringAndSize(buf, buf_len);
#endif
    }

}


static PyObject *
Pipe_sndbuf_get(Pipe *self, void *closure)
{
    int err;
    int sndbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    sndbuf_value = 0;
    err = uv_send_buffer_size(UV_HANDLE(self), &sndbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }
    return PyInt_FromLong((long) sndbuf_value);
}


static int
Pipe_sndbuf_set(Pipe *self, PyObject *value, void *closure)
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
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return -1;
    }
    return 0;
}


static PyObject *
Pipe_rcvbuf_get(Pipe *self, void *closure)
{
    int err;
    int rcvbuf_value;

    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    rcvbuf_value = 0;
    err = uv_recv_buffer_size(UV_HANDLE(self), &rcvbuf_value);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }
    return PyInt_FromLong((long) rcvbuf_value);
}


static int
Pipe_rcvbuf_set(Pipe *self, PyObject *value, void *closure)
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
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return -1;
    }
    return 0;
}


static int
Pipe_tp_init(Pipe *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;
    PyObject *ipc = Py_False;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!|O!:__init__", &LoopType, &loop, &PyBool_Type, &ipc)) {
        return -1;
    }

    err = uv_pipe_init(loop->uv_loop, &self->pipe_h, (ipc == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Pipe_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Pipe *self;

    self = (Pipe *)StreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->pipe_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->pipe_h;

    return (PyObject *)self;
}


static int
Pipe_tp_traverse(Pipe *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    return StreamType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Pipe_tp_clear(Pipe *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    return StreamType.tp_clear((PyObject *)self);
}


static PyMethodDef
Pipe_tp_methods[] = {
    { "bind", (PyCFunction)Pipe_func_bind, METH_VARARGS, "Bind to the specified Pipe name." },
    { "listen", (PyCFunction)Pipe_func_listen, METH_VARARGS, "Start listening for connections on the Pipe." },
    { "accept", (PyCFunction)Pipe_func_accept, METH_VARARGS, "Accept incoming connection." },
    { "connect", (PyCFunction)Pipe_func_connect, METH_VARARGS, "Start connecion to the remote Pipe." },
    { "open", (PyCFunction)Pipe_func_open, METH_VARARGS, "Open the specified file descriptor and manage it as a Pipe." },
    { "pending_instances", (PyCFunction)Pipe_func_pending_instances, METH_VARARGS, "Set the number of pending pipe instance handles when the pipe server is waiting for connections." },
    { "pending_handle_type", (PyCFunction)Pipe_func_pending_handle_type, METH_NOARGS, "Returns the type of the next pending handle. Can be called multiple times." },
    { "write", (PyCFunction)Pipe_func_write, METH_VARARGS, "Write data and send handle over a pipe." },
    { "getsockname", (PyCFunction)Pipe_func_getsockname, METH_NOARGS, "Get bound pipe name." },
    { "getpeername", (PyCFunction)Pipe_func_getpeername, METH_NOARGS, "Get bound pipe name." },
    { NULL }
};


static PyGetSetDef Pipe_tp_getsets[] = {
    {"send_buffer_size", (getter)Pipe_sndbuf_get, (setter)Pipe_sndbuf_set, "Send buffer size.", NULL},
    {"receive_buffer_size", (getter)Pipe_rcvbuf_get, (setter)Pipe_rcvbuf_set, "Receive buffer size.", NULL},
    {NULL}
};


static PyTypeObject PipeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Pipe",                                            /*tp_name*/
    sizeof(Pipe),                                                  /*tp_basicsize*/
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
    (traverseproc)Pipe_tp_traverse,                                /*tp_traverse*/
    (inquiry)Pipe_tp_clear,                                        /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    Pipe_tp_methods,                                               /*tp_methods*/
    0,                                                             /*tp_members*/
    Pipe_tp_getsets,                                               /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)Pipe_tp_init,                                        /*tp_init*/
    0,                                                             /*tp_alloc*/
    Pipe_tp_new,                                                   /*tp_new*/
};

