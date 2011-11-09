
static PyObject* PyExc_PipeClientError;
static PyObject* PyExc_PipeClientConnectionError;
static PyObject* PyExc_PipeServerError;


/* PipeServer */

static void
on_pipe_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(server);

    PipeServer *self = (PipeServer *)(server->data);
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(SELF_LOOP);
        PyErr_SetString(PyExc_PipeServerError, uv_strerror(err));
            PyErr_WriteUnraisable(PyExc_PipeServerError);
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
on_pipe_server_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static PyObject *
PipeServer_func_bind(PipeServer *self, PyObject *args)
{
    int r = 0;
    char *name;

    if (self->closed) {
        PyErr_SetString(PyExc_PipeServerError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "s:bind", &name)) {
        return NULL;
    }

    r = uv_pipe_bind(self->uv_pipe_server, name);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PipeServerError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
PipeServer_func_listen(PipeServer *self, PyObject *args)
{
    int r;
    int backlog = 128;
    PyObject *callback;
    PyObject *tmp = NULL;

    if (self->closed) {
        PyErr_SetString(PyExc_PipeServerError, "already closed");
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

    r = uv_listen((uv_stream_t *)self->uv_pipe_server, backlog, on_pipe_connection);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PipeServerError);
        return NULL;
    }

    tmp = self->on_new_connection_cb;
    Py_INCREF(callback);
    self->on_new_connection_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
PipeServer_func_close(PipeServer *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_PipeServerError, "already closed");
        return NULL;
    }

    self->closed = True;
    uv_close((uv_handle_t *)self->uv_pipe_server, on_pipe_server_close);

    Py_RETURN_NONE;
}


static PyObject *
PipeServer_func_accept(PipeServer *self)
{
    int r = 0;

    if (self->closed) {
        PyErr_SetString(PyExc_PipeServerError, "already closed");
        return NULL;
    }

    PipeClientConnection *connection;
    connection = (PipeClientConnection *)PyObject_CallFunction((PyObject *)&PipeClientConnectionType, "O", self->loop);

    r = uv_accept((uv_stream_t *)self->uv_pipe_server, ((IOStream *)connection)->uv_stream);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PipeServerError);
        return NULL;
    }

    return (PyObject *)connection;
}


static int
PipeServer_tp_init(PipeServer *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;
    uv_pipe_t *uv_pipe_server = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_PipeServerError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_pipe_server = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_pipe_server) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }
    int r = uv_pipe_init(SELF_LOOP, uv_pipe_server, 0);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PipeServerError);
        Py_DECREF(loop);
        return -1;
    }
    uv_pipe_server->data = (void *)self;
    self->uv_pipe_server = uv_pipe_server;

    self->initialized = True;
    self->closed = False;

    return 0;
}


static PyObject *
PipeServer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PipeServer *self = (PipeServer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static int
PipeServer_tp_traverse(PipeServer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_new_connection_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
PipeServer_tp_clear(PipeServer *self)
{
    Py_CLEAR(self->on_new_connection_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
PipeServer_tp_dealloc(PipeServer *self)
{
    if (!self->closed) {
        uv_close((uv_handle_t *)self->uv_pipe_server, on_pipe_server_close);
    }
    PipeServer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
PipeServer_tp_methods[] = {
    { "bind", (PyCFunction)PipeServer_func_bind, METH_VARARGS, "Bind to the specified Pipe name." },
    { "listen", (PyCFunction)PipeServer_func_listen, METH_VARARGS, "Start listening for connections on the Pipe." },
    { "close", (PyCFunction)PipeServer_func_close, METH_NOARGS, "Stop listening for connections." },
    { "accept", (PyCFunction)PipeServer_func_accept, METH_NOARGS, "Accept incoming connection." },
    { NULL }
};


static PyMemberDef PipeServer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(PipeServer, loop), READONLY, "Loop where this PipeServer is running on."},
    {NULL}
};


static PyTypeObject PipeServerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.PipeServer",                                              /*tp_name*/
    sizeof(PipeServer),                                             /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)PipeServer_tp_dealloc,                              /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,      /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)PipeServer_tp_traverse,                           /*tp_traverse*/
    (inquiry)PipeServer_tp_clear,                                   /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    PipeServer_tp_methods,                                          /*tp_methods*/
    PipeServer_tp_members,                                          /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)PipeServer_tp_init,                                   /*tp_init*/
    0,                                                              /*tp_alloc*/
    PipeServer_tp_new,                                              /*tp_new*/
};


/* PipeClientConnection */

static int
PipeClientConnection_tp_init(PipeClientConnection *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    uv_pipe_t *uv_stream;

    IOStream *parent = (IOStream *)self;

    if (IOStreamType.tp_init((PyObject *)self, args, kwargs) < 0) {
        return -1;
    }

    uv_stream = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(PARENT_LOOP);
        return -1;
    }

    r = uv_pipe_init(PARENT_LOOP, (uv_pipe_t *)uv_stream, 0);
    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_PipeClientConnectionError);
        Py_DECREF(PARENT_LOOP);
        return -1;
    }
    uv_stream->data = (void *)self;
    parent->uv_stream = (uv_stream_t *)uv_stream;

    return 0;
}


static PyObject *
PipeClientConnection_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PipeClientConnection *self = (PipeClientConnection *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static PyTypeObject PipeClientConnectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.PipeClientConnection",                                   /*tp_name*/
    sizeof(PipeClientConnection),                                  /*tp_basicsize*/
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
    0,                                                             /*tp_traverse*/
    0,                                                             /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    0,                                                             /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)PipeClientConnection_tp_init,                        /*tp_init*/
    0,                                                             /*tp_alloc*/
    PipeClientConnection_tp_new,                                   /*tp_new*/
};


/* PipeClient */

typedef struct {
    uv_connect_t req;
    void *data;
} pipe_connect_req_t;


static void
on_pipe_client_connect(uv_connect_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);

    pipe_connect_req_t* connect_req = (pipe_connect_req_t*) req;
    iostream_req_data_t* req_data = (iostream_req_data_t *)connect_req->data;

    IOStream *self = (IOStream *)req_data->obj;
    PyObject *callback = req_data->callback;

    PyObject *error;
    PyObject *exc_data;
    PyObject *result;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        error = PyBool_FromLong(1);
        uv_err_t err = uv_last_error(SELF_LOOP);
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_PipeClientError, exc_data);
            Py_DECREF(exc_data);
        }
        // TODO: pass exception to callback, how?
        PyErr_WriteUnraisable(callback);
    } else {
        error = PyBool_FromLong(0);
    }

    result = PyObject_CallFunctionObjArgs(callback, self, error, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);

    Py_DECREF(callback);
    PyMem_Free(req_data);
    connect_req->data = NULL;
    PyMem_Free(connect_req);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
PipeClient_func_connect(PipeClient *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    pipe_connect_req_t *connect_req = NULL;
    iostream_req_data_t *req_data = NULL;
    PyObject *callback;

    IOStream *parent = (IOStream *)self;

    if (parent->closed) {
        PyErr_SetString(PyExc_PipeClientError, "closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "sO:connect", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    connect_req = (pipe_connect_req_t*) PyMem_Malloc(sizeof(pipe_connect_req_t));
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

    uv_pipe_connect(&connect_req->req, (uv_pipe_t *)parent->uv_stream, name, on_pipe_client_connect);

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


static int
PipeClient_tp_init(PipeClient *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    uv_pipe_t *uv_stream;

    IOStream *parent = (IOStream *)self;

    if (IOStreamType.tp_init((PyObject *)self, args, kwargs) < 0) {
        return -1;
    }

    uv_stream = PyMem_Malloc(sizeof(uv_pipe_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(PARENT_LOOP);
        return -1;
    }

    r = uv_pipe_init(PARENT_LOOP, (uv_pipe_t *)uv_stream, 0);
    if (r != 0) {
        raise_uv_exception(parent->loop, PyExc_PipeClientError);
        Py_DECREF(PARENT_LOOP);
        return -1;
    }
    uv_stream->data = (void *)self;
    parent->uv_stream = (uv_stream_t *)uv_stream;

    return 0;
}


static PyObject *
PipeClient_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PipeClient *self = (PipeClient *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static PyMethodDef
PipeClient_tp_methods[] = {
    { "connect", (PyCFunction)PipeClient_func_connect, METH_VARARGS, "Start connecion to the remote Pipe." },
    { NULL }
};


static PyTypeObject PipeClientType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.PipeClient",                                             /*tp_name*/
    sizeof(PipeClient),                                            /*tp_basicsize*/
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
    0,                                                             /*tp_traverse*/
    0,                                                             /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    PipeClient_tp_methods,                                         /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)PipeClient_tp_init,                                  /*tp_init*/
    0,                                                             /*tp_alloc*/
    PipeClient_tp_new,                                             /*tp_new*/
};

