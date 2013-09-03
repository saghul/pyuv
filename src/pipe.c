
static void
on_pipe_connection(uv_stream_t* handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *result, *errorno;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, Pipe, pipe_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        errorno = error_to_obj(status);
    } else {
        PYUV_SET_NONE(errorno);
    }
    result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(errorno);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_pipe_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *callback, *result, *errorno;

    ASSERT(req);
    self = PYUV_CONTAINER_OF(req->handle, Pipe, pipe_h);
    callback = (PyObject *)req->data;

    ASSERT(self);

    if (status != 0) {
        errorno = error_to_obj(status);
    } else {
        PYUV_SET_NONE(errorno);
    }
    result = PyObject_CallFunctionObjArgs(callback, self, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(req);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_pipe_read2(uv_pipe_t* handle, int nread, uv_buf_t buf, uv_handle_type pending)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *result, *data, *py_pending, *errorno;

    ASSERT(handle);
    data = errorno = NULL;
    self = PYUV_CONTAINER_OF(handle, Pipe, pipe_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    py_pending = obj_or_none(enomem_if_null(PyInt_FromLong(pending), &errorno));
    if (errorno == NULL) {
        if (nread >= 0) {
            data = enomem_if_null(PyBytes_FromStringAndSize(buf.base, nread), &errorno);
            errorno = obj_or_none(errorno);
        } else {
            PYUV_SET_NONE(data);
            errorno = error_to_obj(nread);
        }
    }
    data = obj_or_none(data);

    result = PyObject_CallFunctionObjArgs(((Stream *)self)->on_read_cb, self, data, py_pending, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(data);
    Py_DECREF(py_pending);
    Py_DECREF(errorno);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Pipe_func_bind(Pipe *self, PyObject *args)
{
    int err;
    char *name;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "s:bind", &name)) {
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

    err = uv_listen((uv_stream_t *)&self->pipe_h, backlog, on_pipe_connection);
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

    uv_pipe_connect(connect_req, &self->pipe_h, name, on_pipe_client_connection);

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
Pipe_func_start_read2(Pipe *self, PyObject *args)
{
    int err;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start_read2", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    err = uv_read2_start((uv_stream_t *)&self->pipe_h, (uv_alloc_cb)on_stream_alloc, (uv_read2_cb)on_pipe_read2);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        return NULL;
    }

    tmp = ((Stream *)self)->on_read_cb;
    Py_INCREF(callback);
    ((Stream *)self)->on_read_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_write2(Pipe *self, PyObject *args)
{
    uv_buf_t buf;
    stream_write_ctx *ctx;
    Py_buffer *view;
    PyObject *callback, *send_handle;

    callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    ctx = PyMem_Malloc(sizeof *ctx);
    if (!ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    view = &ctx->view[0];

    if (!PyArg_ParseTuple(args, PYUV_BYTES"*O|O:write", view, &send_handle, &callback)) {
        PyMem_Free(ctx);
        return NULL;
    }

    if (PyObject_IsSubclass((PyObject *)send_handle->ob_type, (PyObject *)&StreamType)) {
        if (UV_HANDLE(send_handle)->type != UV_TCP && UV_HANDLE(send_handle)->type != UV_NAMED_PIPE) {
            PyErr_SetString(PyExc_TypeError, "Only TCP and Pipe objects are supported for write2");
            goto error;
        }
    } else if (PyObject_IsSubclass((PyObject *)send_handle->ob_type, (PyObject *)&UDPType)) {
        /* empty */
    } else {
        PyErr_SetString(PyExc_TypeError, "Only Stream and UDP objects are supported");
        goto error;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        goto error;
    }

    buf = uv_buf_init(view->buf, view->len);

    return pyuv_stream_write((Stream *)self, ctx, view, &buf, 1, callback, send_handle);

error:
        PyBuffer_Release(view);
        PyMem_Free(ctx);
        return NULL;
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
    { "start_read2", (PyCFunction)Pipe_func_start_read2, METH_VARARGS, "Extended read methods for receiving handles over a pipe. The pipe must be initialized with ipc set to True." },
    { "write2", (PyCFunction)Pipe_func_write2, METH_VARARGS, "Write data and send handle over a pipe." },
    { NULL }
};


static PyTypeObject PipeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Pipe",                                                   /*tp_name*/
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
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)Pipe_tp_init,                                        /*tp_init*/
    0,                                                             /*tp_alloc*/
    Pipe_tp_new,                                                   /*tp_new*/
};

