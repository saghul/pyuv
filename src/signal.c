
/*
 * This is not a true signal handler but it does the job. It uses a Prepare handle, which
 * gets executed one per loop iteration, before blocking for I/O, to call PyErr_CheckSignals.
 * That will cause standard registered signal handlers to be called by Python.
 */

static PyObject* PyExc_SignalError;


static void
on_signal_callback(uv_prepare_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Signal *self;

    ASSERT(handle);
    ASSERT(status == 0);

    self = (Signal *)handle->data;
    ASSERT(self);

    if (PyErr_CheckSignals() < 0 && PyErr_Occurred()) {
        PyErr_Print();
    }

    PyGILState_Release(gstate);
}


static PyObject *
Signal_func_start(Signal *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_prepare_start((uv_prepare_t *)UV_HANDLE(self), on_signal_callback);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_SignalError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Signal_func_stop(Signal *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_prepare_stop((uv_prepare_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_SignalError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
Signal_tp_init(Signal *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_prepare_t *uv_prepare = NULL;
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_SignalError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    uv_prepare = PyMem_Malloc(sizeof(uv_prepare_t));
    if (!uv_prepare) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_prepare_init(UV_HANDLE_LOOP(self), uv_prepare);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_SignalError);
        Py_DECREF(loop);
        return -1;
    }
    uv_prepare->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_prepare;

    return 0;
}


static PyObject *
Signal_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Signal *self = (Signal *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Signal_tp_traverse(Signal *self, visitproc visit, void *arg)
{
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Signal_tp_clear(Signal *self)
{
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Signal_tp_methods[] = {
    { "start", (PyCFunction)Signal_func_start, METH_NOARGS, "Start the Signal." },
    { "stop", (PyCFunction)Signal_func_stop, METH_NOARGS, "Stop the Signal." },
    { NULL }
};


static PyTypeObject SignalType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Signal",                                                  /*tp_name*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
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


