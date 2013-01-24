
static void
on_poll_callback(uv_poll_t *handle, int status, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    Poll *self;
    PyObject *result, *py_events, *py_errorno;

    ASSERT(handle);

    self = (Poll *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status == 0) {
        py_events = PyInt_FromLong((long)events);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else  {
        py_events = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_HANDLE_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->callback, self, py_events, py_errorno,NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Poll_func_start(Poll *self, PyObject *args)
{
    int r, events;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "iO:start", &events, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_poll_start((uv_poll_t *)UV_HANDLE(self), events, on_poll_callback);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PollError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Poll_func_stop(Poll *self)
{
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_poll_stop((uv_poll_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PollError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    Py_RETURN_NONE;
}


static int
Poll_tp_init(Poll *self, PyObject *args, PyObject *kwargs)
{
    int r;
    long fd;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!l:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

    r = uv_poll_init_socket(loop->uv_loop, (uv_poll_t *)UV_HANDLE(self), (uv_os_sock_t)fd);
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_PollError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Poll_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    uv_poll_t *uv_poll;

    uv_poll = PyMem_Malloc(sizeof *uv_poll);
    if (!uv_poll) {
        PyErr_NoMemory();
        return NULL;
    }

    Poll *self = (Poll *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        PyMem_Free(uv_poll);
        return NULL;
    }

    uv_poll->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_poll;

    return (PyObject *)self;
}


static int
Poll_tp_traverse(Poll *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Poll_tp_clear(Poll *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Poll_tp_methods[] = {
    { "start", (PyCFunction)Poll_func_start, METH_VARARGS, "Start the Poll." },
    { "stop", (PyCFunction)Poll_func_stop, METH_NOARGS, "Stop the Poll." },
    { NULL }
};


static PyTypeObject PollType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Poll",                                                    /*tp_name*/
    sizeof(Poll),                                                   /*tp_basicsize*/
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
    (traverseproc)Poll_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Poll_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Poll_tp_methods,                                                /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Poll_tp_init,                                         /*tp_init*/
    0,                                                              /*tp_alloc*/
    Poll_tp_new,                                                    /*tp_new*/
};

