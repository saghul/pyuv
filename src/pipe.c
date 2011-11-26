
static PyObject* PyExc_PipeError;


static void
on_pipe_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(server);

    Pipe *self = (Pipe *)server->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    IOStream *base = (IOStream *)self;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
        PyErr_SetString(PyExc_PipeError, uv_strerror(err));
            PyErr_WriteUnraisable(PyExc_PipeError);
    } else {
        PyObject *result;
        result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_new_connection_cb);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_pipe_client_connection(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    iostream_req_data_t* req_data = (iostream_req_data_t *)req->data;

    Pipe *self = (Pipe *)req_data->obj;
    PyObject *callback = req_data->callback;

    PyObject *py_status;
    PyObject *result;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    IOStream *base = (IOStream *)self;

    if (status != 0) {
        uv_err_t err = uv_last_error(UV_LOOP(base));
        py_status = PyInt_FromLong(err.code);
    } else {
        py_status = PyInt_FromLong(0);
    }

    result = PyObject_CallFunctionObjArgs(callback, self, py_status, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);

    Py_DECREF(callback);
    PyMem_Free(req_data);
    req->data = NULL;
    PyMem_Free(req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Pipe_func_bind(Pipe *self, PyObject *args)
{
    int r = 0;
    char *name;

    IOStream *base = (IOStream *)self;

    if (base->closed) {
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
    int r;
    int backlog = 128;
    PyObject *callback;
    PyObject *tmp = NULL;

    IOStream *base = (IOStream *)self;

    if (base->closed) {
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
Pipe_func_accept(Pipe *self)
{
    int r = 0;

    IOStream *base = (IOStream *)self;

    if (base->closed) {
        PyErr_SetString(PyExc_PipeError, "already closed");
        return NULL;
    }

    Pipe *connection;
    connection = (Pipe *)PyObject_CallFunction((PyObject *)&PipeType, "O", base->loop);

    r = uv_accept(base->uv_handle, ((IOStream *)connection)->uv_handle);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        return NULL;
    }

    return (PyObject *)connection;
}


static PyObject *
Pipe_func_connect(Pipe *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    uv_connect_t *connect_req = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *callback;

    IOStream *base = (IOStream *)self;

    if (base->closed) {
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
        connect_req->data = NULL;
        PyMem_Free(connect_req);
    }
    if (req_data) {
        Py_DECREF(callback);
        req_data->obj = NULL;
        req_data->callback = NULL;
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
Pipe_func_open(Pipe *self, PyObject *args)
{
    int fd;

    IOStream *base = (IOStream *)self;

    if (base->closed) {
        PyErr_SetString(PyExc_PipeError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:open", &fd)) {
        return NULL;
    }

    uv_pipe_open((uv_pipe_t *)base->uv_handle, fd);

    Py_RETURN_NONE;
}


static int
Pipe_tp_init(Pipe *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    uv_pipe_t *uv_stream;

    IOStream *base = (IOStream *)self;

    if (IOStreamType.tp_init((PyObject *)self, args, kwargs) < 0) {
        return -1;
    }

    uv_stream = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(base->loop);
        return -1;
    }

    r = uv_pipe_init(UV_LOOP(base), (uv_pipe_t *)uv_stream, 0);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_PipeError);
        Py_DECREF(base->loop);
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
    { "accept", (PyCFunction)Pipe_func_accept, METH_NOARGS, "Accept incoming connection." },
    { "connect", (PyCFunction)Pipe_func_connect, METH_VARARGS, "Start connecion to the remote Pipe." },
    { "open", (PyCFunction)Pipe_func_open, METH_VARARGS, "Open the specified file descriptor and manage it as a Pipe." },
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /*tp_flags*/
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

