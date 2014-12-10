
static void
pyuv__signal_cb(uv_signal_t *handle, int signum)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Signal *self;
    PyObject *result;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, Signal, signal_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    result = PyObject_CallFunctionObjArgs(self->callback, self, PyInt_FromLong((long)signum), NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Signal_func_start(Signal *self, PyObject *args)
{
    int err, signum;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "Oi:start", &callback, &signum)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    err = uv_signal_start(&self->signal_h, (uv_signal_cb)pyuv__signal_cb, signum);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_SignalError);
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
Signal_func_stop(Signal *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_signal_stop(&self->signal_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_SignalError);
        return NULL;
    }

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static int
Signal_tp_init(Signal *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_signal_init(loop->uv_loop, &self->signal_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_SignalError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Signal_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Signal *self;

    self = (Signal *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->signal_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->signal_h;

    return (PyObject *)self;
}


static int
Signal_tp_traverse(Signal *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Signal_tp_clear(Signal *self)
{
    Py_CLEAR(self->callback);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
Signal_tp_methods[] = {
    { "start", (PyCFunction)Signal_func_start, METH_VARARGS, "Start the Signal handle." },
    { "stop", (PyCFunction)Signal_func_stop, METH_NOARGS, "Stop the Signal handle." },
    { NULL }
};


static PyTypeObject SignalType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Signal",                                           /*tp_name*/
    sizeof(Signal),                                                 /*tp_basicsize*/
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
    (traverseproc)Signal_tp_traverse,                               /*tp_traverse*/
    (inquiry)Signal_tp_clear,                                       /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Signal_tp_methods,                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Signal_tp_init,                                       /*tp_init*/
    0,                                                              /*tp_alloc*/
    Signal_tp_new,                                                  /*tp_new*/
};

