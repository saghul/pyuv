
static void
on_idle_callback(uv_idle_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Idle *self;
    PyObject *result;

    ASSERT(handle);
    ASSERT(status == 0);

    self = PYUV_CONTAINER_OF(handle, Idle, idle_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    result = PyObject_CallFunctionObjArgs(self->callback, self, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Idle_func_start(Idle *self, PyObject *args)
{
    int err;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    err = uv_idle_start(&self->idle_h, on_idle_callback);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_IdleError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_INCREF(self);
    return (PyObject *)self;
}


static PyObject *
Idle_func_stop(Idle *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_idle_stop(&self->idle_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_IdleError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    Py_INCREF(self);
    return (PyObject *)self;
}


static int
Idle_tp_init(Idle *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_idle_init(loop->uv_loop, &self->idle_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_IdleError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Idle_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Idle *self;

    self = (Idle *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->idle_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->idle_h;

    return (PyObject *)self;
}


static int
Idle_tp_traverse(Idle *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Idle_tp_clear(Idle *self)
{
    Py_CLEAR(self->callback);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
Idle_tp_methods[] = {
    { "start", (PyCFunction)Idle_func_start, METH_VARARGS, "Start the Idle." },
    { "stop", (PyCFunction)Idle_func_stop, METH_NOARGS, "Stop the Idle." },
    { NULL }
};


static PyTypeObject IdleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Idle",                                                    /*tp_name*/
    sizeof(Idle),                                                   /*tp_basicsize*/
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
    (traverseproc)Idle_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Idle_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Idle_tp_methods,                                                /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Idle_tp_init,                                         /*tp_init*/
    0,                                                              /*tp_alloc*/
    Idle_tp_new,                                                    /*tp_new*/
};

