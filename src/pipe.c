
static PyObject* PyExc_PipeError;


static void
on_pipe_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    PyObject *result, *py_errorno;
    ASSERT(server);

    self = (Pipe *)server->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    } else {
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_new_connection_cb);
    }
    Py_XDECREF(result);
    Py_DECREF(py_errorno);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_pipe_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    stream_req_data_t* req_data;
    Pipe *self;
    PyObject *callback, *result, *py_errorno;
    ASSERT(req);

    req_data = (stream_req_data_t *)req->data;
    self = (Pipe *)req->handle->data;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong(err.code);
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

    Py_DECREF(callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_pipe_read2(uv_pipe_t* handle, int nread, uv_buf_t buf, uv_handle_type pending)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    Stream *self;
    PyObject *result, *data, *py_errorno, *py_pending;
    ASSERT(handle);

    self = (Stream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    py_pending = PyInt_FromLong((long)pending);

    if (nread >= 0) {
        data = PyBytes_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_pending, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);
    Py_DECREF(data);
    Py_DECREF(py_pending);
    Py_DECREF(py_errorno);

    /* In case of error libuv may not call alloc_cb */
    if (buf.base != NULL) {
        PyMem_Free(buf.base);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Pipe_func_bind(Pipe *self, PyObject *args)
{
    int r;
    char *name;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "s:bind", &name)) {
        return NULL;
    }

    r = uv_pipe_bind((uv_pipe_t *)UV_HANDLE(self), name);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_listen(Pipe *self, PyObject *args)
{
    int r, backlog;
    PyObject *callback, *tmp;

    backlog = 128;
    tmp = NULL;

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

    r = uv_listen((uv_stream_t *)UV_HANDLE(self), backlog, on_pipe_connection);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
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
    int r;
    PyObject *client;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:accept", &client)) {
        return NULL;
    }

    if (!PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&StreamType)) {
        PyErr_SetString(PyExc_TypeError, "Only stream objects are supported for accept");
        return NULL;
    }

    r = uv_accept((uv_stream_t *)UV_HANDLE(self), (uv_stream_t *)UV_HANDLE(client));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_connect(Pipe *self, PyObject *args)
{
    char *name;
    uv_connect_t *connect_req = NULL;
    stream_req_data_t *req_data = NULL;
    PyObject *callback;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "sO:connect", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    connect_req = (uv_connect_t *)PyMem_Malloc(sizeof(uv_connect_t));
    if (!connect_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (stream_req_data_t*) PyMem_Malloc(sizeof(stream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    req_data->callback = callback;
    connect_req->data = (void *)req_data;

    uv_pipe_connect(connect_req, (uv_pipe_t *)UV_HANDLE(self), name, on_pipe_client_connection);

    Py_RETURN_NONE;

error:
    if (connect_req) {
        PyMem_Free(connect_req);
    }
    if (req_data) {
        Py_DECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
Pipe_func_open(Pipe *self, PyObject *args)
{
    int fd;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:open", &fd)) {
        return NULL;
    }

    uv_pipe_open((uv_pipe_t *)UV_HANDLE(self), fd);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_pending_instances(Pipe *self, PyObject *args)
{
    /* This function applies to Windows only */
    int count;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:pending_instances", &count)) {
        return NULL;
    }

    uv_pipe_pending_instances((uv_pipe_t *)UV_HANDLE(self), count);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_start_read2(Pipe *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start_read2", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_read2_start((uv_stream_t *)UV_HANDLE(self), (uv_alloc_cb)on_stream_alloc, (uv_read2_cb)on_pipe_read2);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
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
    int i, r, buf_count;
    Py_buffer pbuf;
    PyObject *callback, *send_handle;
    uv_buf_t *bufs = NULL;
    uv_write_t *wr = NULL;
    stream_req_data_t *req_data = NULL;
    stream_write_data_t *write_data = NULL;

    buf_count = 0;
    callback = Py_None;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "s*O|O:write2", &pbuf, &send_handle, &callback)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(send_handle, &TCPType) && !PyObject_TypeCheck(send_handle, &PipeType)) {
        PyErr_SetString(PyExc_TypeError, "Only TCP and Pipe objects are supported for write2");
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    wr = (uv_write_t *)PyMem_Malloc(sizeof(uv_write_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (stream_req_data_t*) PyMem_Malloc(sizeof(stream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (stream_write_data_t *) PyMem_Malloc(sizeof(stream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    req_data->callback = callback;
    wr->data = (void *)req_data;

    bufs = (uv_buf_t *) PyMem_Malloc(sizeof(uv_buf_t));
    if (!bufs) {
        PyErr_NoMemory();
        goto error;
    }

    bufs[0] = uv_buf_init(pbuf.buf, pbuf.len);
    buf_count = 1;

    write_data->bufs = bufs;
    write_data->buf_count = buf_count;
    write_data->view = pbuf;
    req_data->data = write_data;

    r = uv_write2(wr, (uv_stream_t *)UV_HANDLE(self), bufs, buf_count, (uv_stream_t *)UV_HANDLE(send_handle), on_stream_write);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    PyBuffer_Release(&pbuf);
    if (bufs) {
        for (i = 0; i < buf_count; i++) {
            PyMem_Free(bufs[i].base);
        }
        PyMem_Free(bufs);
    }
    if (write_data) {
        PyMem_Free(write_data);
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


static int
Pipe_tp_init(Pipe *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_pipe_t *uv_pipe;
    Loop *loop;
    PyObject *tmp = NULL;
    PyObject *ipc = Py_False;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_PipeError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!|O!:__init__", &LoopType, &loop, &PyBool_Type, &ipc)) {
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    uv_pipe = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_pipe) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_pipe_init(UV_HANDLE_LOOP(self), uv_pipe, (ipc == Py_True) ? 1 : 0);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PipeError);
        Py_DECREF(loop);
        return -1;
    }
    uv_pipe->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_pipe;

    return 0;
}


static PyObject *
Pipe_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Pipe *self = (Pipe *)StreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Pipe_tp_traverse(Pipe *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    StreamType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Pipe_tp_clear(Pipe *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    StreamType.tp_clear((PyObject *)self);
    return 0;
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                       /*tp_flags*/
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

