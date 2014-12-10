
static void
pyuv__poll_cb(uv_poll_t *handle, int status, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Poll *self;
    PyObject *result, *py_events, *py_errorno;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, Poll, poll_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status == 0) {
        py_events = PyInt_FromLong((long)events);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else  {
        py_events = Py_None;
        Py_INCREF(Py_None);
        py_errorno = PyInt_FromLong((long)status);
    }

    result = PyObject_CallFunctionObjArgs(self->callback, self, py_events, py_errorno, NULL);
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
    int err, events;
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

    err = uv_poll_start(&self->poll_h, events, pyuv__poll_cb);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PollError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    PYUV_HANDLE_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
Poll_func_stop(Poll *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_poll_stop(&self->poll_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PollError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static PyObject *
Poll_func_fileno(Poll *self)
{
    int err;
    uv_os_fd_t fd;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_fileno(UV_HANDLE(self), &fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PollError);
        return NULL;
    }

    /* This is safe. See note in Stream_func_fileno() */
    return PyInt_FromLong((long) fd);
}


static int
Poll_tp_init(Poll *self, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!l:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

    err = uv_poll_init_socket(loop->uv_loop, &self->poll_h, (uv_os_sock_t)fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PollError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Poll_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Poll *self;

    self = (Poll *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->poll_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->poll_h;

    return (PyObject *)self;
}


static int
Poll_tp_traverse(Poll *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Poll_tp_clear(Poll *self)
{
    Py_CLEAR(self->callback);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
Poll_tp_methods[] = {
    { "start", (PyCFunction)Poll_func_start, METH_VARARGS, "Start the Poll." },
    { "stop", (PyCFunction)Poll_func_stop, METH_NOARGS, "Stop the Poll." },
    { "fileno", (PyCFunction)Poll_func_fileno, METH_NOARGS, "File descriptor being monitored." },
    { NULL }
};


static PyTypeObject PollType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Poll",                                             /*tp_name*/
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

