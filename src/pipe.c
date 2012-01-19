
static PyObject* PyExc_PipeError;


static void
on_pipe_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Pipe *self;
    IOStream *base;
    PyObject *result, *py_errorno;
    ASSERT(server);

    self = (Pipe *)server->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    base = (IOStream *)self;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
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

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_pipe_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    iostream_req_data_t* req_data;
    Pipe *self;
    IOStream *base;
    PyObject *callback, *result, *py_errorno;
    ASSERT(req);

    req_data = (iostream_req_data_t *)req->data;
    self = (Pipe *)req_data->obj;
    callback = req_data->callback;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    base = (IOStream *)self;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
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
    IOStream *self;
    PyObject *result, *data, *py_errorno, *py_pending;
    ASSERT(handle);

    self = (IOStream *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    py_pending = PyInt_FromLong((long)pending);

    if (nread >= 0) {
        data = PyString_FromStringAndSize(buf.base, nread);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else if (nread < 0) {
        data = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->on_read_cb, self, data, py_pending, py_errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->on_read_cb);
    }
    Py_XDECREF(result);

    PyMem_Free(buf.base);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Pipe_func_bind(Pipe *self, PyObject *args)
{
    int r;
    char *name;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "s:bind", &name)) {
        return NULL;
    }

    r = uv_pipe_bind((uv_pipe_t *)base->uv_handle, name);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_listen(Pipe *self, PyObject *args)
{
    int r, backlog;
    PyObject *callback, *tmp;

    IOStream *base = (IOStream *)self;

    backlog = 128;
    tmp = NULL;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "already closed");
        return NULL;
    }

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

    r = uv_listen(base->uv_handle, backlog, on_pipe_connection);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        return NULL;
    }

    tmp = self->on_new_connection_cb;
    Py_INCREF(callback);
    self->on_new_connection_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_accept(TCP *self, PyObject *args)
{
    int r;
    PyObject *client;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:accept", &client)) {
        return NULL;
    }

    if (!PyObject_IsSubclass((PyObject *)client->ob_type, (PyObject *)&IOStreamType)) {
        PyErr_SetString(PyExc_TypeError, "Only stream objects are supported for accept");
        return NULL;
    }

    r = uv_accept(base->uv_handle, ((IOStream *)client)->uv_handle);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_connect(Pipe *self, PyObject *args)
{
    char *name;
    uv_connect_t *connect_req = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *callback;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "closed");
        return NULL;
    }

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

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)self;
    Py_INCREF(callback);
    req_data->callback = callback;
    connect_req->data = (void *)req_data;

    uv_pipe_connect(connect_req, (uv_pipe_t *)base->uv_handle, name, on_pipe_client_connection);

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

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:open", &fd)) {
        return NULL;
    }

    uv_pipe_open((uv_pipe_t *)base->uv_handle, fd);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_pending_instances(Pipe *self, PyObject *args)
{
    /* This function applies to Windows only */
    int count;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:pending_instances", &count)) {
        return NULL;
    }

    uv_pipe_pending_instances((uv_pipe_t *)base->uv_handle, count);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_start_read2(Pipe *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    IOStream *base = (IOStream *)self;

    tmp = NULL;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "Pipe is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start_read2", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_read2_start((uv_stream_t *)base->uv_handle, (uv_alloc_cb)on_iostream_alloc, (uv_read2_cb)on_pipe_read2);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        return NULL;
    }

    tmp = base->on_read_cb;
    Py_INCREF(callback);
    base->on_read_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Pipe_func_write2(Pipe *self, PyObject *args)
{
    int i, n, r, buf_count;
    char *data_str;
    char *tmp;
    Py_ssize_t data_len;
    PyObject *item, *data, *callback;
    TCP *send_handle;
    uv_buf_t tmpbuf;
    uv_buf_t *bufs = NULL;
    uv_write_t *wr = NULL;
    iostream_req_data_t *req_data = NULL;
    iostream_write_data_t *write_data = NULL;

    IOStream *base = (IOStream *)self;

    buf_count = 0;
    callback = Py_None;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "Pipe is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "OO!|O:write2", &data, &TCPType, &send_handle, &callback)) {
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

    wr = (uv_write_t *)PyMem_Malloc(sizeof(uv_write_t));
    if (!wr) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = (iostream_req_data_t*) PyMem_Malloc(sizeof(iostream_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    write_data = (iostream_write_data_t *) PyMem_Malloc(sizeof(iostream_write_data_t));
    if (!write_data) {
        PyErr_NoMemory();
        goto error;
    }

    req_data->obj = (PyObject *)base;
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
            strcpy(tmp, data_str);
            tmpbuf = uv_buf_init(tmp, data_len);
            bufs[i] = tmpbuf;
            buf_count++;
        }

    }

    write_data->bufs = bufs;
    write_data->buf_count = buf_count;
    req_data->data = (void *)write_data;

    r = uv_write2(wr, base->uv_handle, bufs, buf_count, (uv_stream_t *)((IOStream *)send_handle)->uv_handle, on_iostream_write);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        goto error;
    }

    Py_RETURN_NONE;

error:
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
    uv_pipe_t *uv_stream;
    Loop *loop;
    PyObject *tmp = NULL;
    PyObject *ipc = Py_False;

    IOStream *base = (IOStream *)self;

    UNUSED_ARG(kwargs);

    if (base->uv_handle) {
        PyErr_SetString(PyExc_PipeError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!|O!:__init__", &LoopType, &loop, &PyBool_Type, &ipc)) {
        return -1;
    }

    tmp = (PyObject *)base->loop;
    Py_INCREF(loop);
    base->loop = loop;
    Py_XDECREF(tmp);

    uv_stream = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_pipe_init(UV_LOOP(base), (uv_pipe_t *)uv_stream, (ipc == Py_True) ? 1 : 0);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        Py_DECREF(loop);
        return -1;
    }
    uv_stream->data = (void *)self;
    base->uv_handle = (uv_stream_t *)uv_stream;

    return 0;
}


static PyObject *
Pipe_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Pipe *self = (Pipe *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Pipe_tp_traverse(Pipe *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    IOStreamType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Pipe_tp_clear(Pipe *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    IOStreamType.tp_clear((PyObject *)self);
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

