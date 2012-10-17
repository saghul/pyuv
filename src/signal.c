
static void
on_signal_callback(uv_signal_t *handle, int signum)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Signal *self;
    PyObject *result;

    ASSERT(handle);

    self = (Signal *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    result = PyObject_CallFunctionObjArgs(self->callback, self, PyInt_FromLong((long)signum), NULL);
    if (result == NULL) {
        handle_uncaught_exception(((Handle *)self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Signal_func_start(Signal *self, PyObject *args)
{
    int r, signum;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "Oi:start", &callback, &signum)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_signal_start((uv_signal_t *)UV_HANDLE(self), (uv_signal_cb)on_signal_callback, signum);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_SignalError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Signal_func_stop(Signal *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_signal_stop((uv_signal_t *)UV_HANDLE(self));
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
    uv_signal_t *uv_signal = NULL;
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

    uv_signal = PyMem_Malloc(sizeof(uv_signal_t));
    if (!uv_signal) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_signal_init(UV_HANDLE_LOOP(self), uv_signal);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_SignalError);
        Py_DECREF(loop);
        return -1;
    }
    uv_signal->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_signal;

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
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Signal_tp_clear(Signal *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Signal_tp_methods[] = {
    { "start", (PyCFunction)Signal_func_start, METH_VARARGS, "Start the Signal handle." },
    { "stop", (PyCFunction)Signal_func_stop, METH_NOARGS, "Stop the Signal handle." },
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


#define PYUV_CHECK_SIGNALS(handle)                      \
    do {                                                \
        PyErr_CheckSignals();                           \
        if (PyErr_Occurred()) {                         \
            handle_uncaught_exception(handle->loop);    \
        }                                               \
    } while (0)                                         \


#define RAISE_IF_SIGNAL_CHECKER_CLOSED(self)                                                                                                                                \
    do {                                                                                                                                                                    \
        if (!self->prepare_handle || uv_is_closing((uv_handle_t *)self->prepare_handle) || !self->check_handle || uv_is_closing((uv_handle_t *)self->check_handle)) {       \
            PyErr_SetString(PyExc_SignalCheckerError, "Signal checker is closed");                                                                                          \
            return NULL;                                                                                                                                                    \
        }                                                                                                                                                                   \
    } while (0)                                                                                                                                                             \


static void
on_signal_checker_prepare_cb(uv_prepare_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    SignalChecker *self;

    ASSERT(handle);
    self = (SignalChecker *)handle->data;
    ASSERT(self);

    PYUV_CHECK_SIGNALS(self);

    PyGILState_Release(gstate);
}


static void
on_signal_checker_check_cb(uv_check_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    SignalChecker *self;

    ASSERT(handle);
    self = (SignalChecker *)handle->data;
    ASSERT(self);

    PYUV_CHECK_SIGNALS(self);

    PyGILState_Release(gstate);
}


static PyObject *
SignalChecker_func_start(SignalChecker *self)
{
    int r;
    Bool error = False;

    RAISE_IF_SIGNAL_CHECKER_CLOSED(self);

    r = uv_prepare_start(self->prepare_handle, on_signal_checker_prepare_cb);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
        error = True;
    }
    r = uv_check_start(self->check_handle, on_signal_checker_check_cb);
    if (r != 0) {
        if (!error) {
            RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
            error = True;
        }
    }

    if (error) {
        uv_prepare_stop(self->prepare_handle);
        uv_check_stop(self->check_handle);
        return NULL;
    }

    uv_unref((uv_handle_t *)self->prepare_handle);
    uv_unref((uv_handle_t *)self->check_handle);

    Py_RETURN_NONE;
}


static PyObject *
SignalChecker_func_stop(SignalChecker *self)
{
    int r;
    Bool error = False;

    RAISE_IF_SIGNAL_CHECKER_CLOSED(self);

    r = uv_prepare_stop(self->prepare_handle);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
        error = True;
    }
    r = uv_check_stop(self->check_handle);
    if (r != 0) {
        if (!error) {
            RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
            error = True;
        }
    }

    if (error) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
SignalChecker_func_close(SignalChecker *self, PyObject *args)
{
    RAISE_IF_SIGNAL_CHECKER_CLOSED(self);

    Py_DECREF(self->loop);
    self->loop = (Loop *)Py_None;
    Py_INCREF(Py_None);

    uv_close((uv_handle_t *)self->prepare_handle, on_handle_dealloc_close);
    uv_close((uv_handle_t *)self->check_handle, on_handle_dealloc_close);

    Py_RETURN_NONE;
}


static PyObject *
SignalChecker_active_get(SignalChecker *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!self->prepare_handle || !self->check_handle) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)(uv_is_active((uv_handle_t *)self->prepare_handle) && uv_is_active((uv_handle_t *)self->check_handle)));
    }
}


static PyObject *
SignalChecker_closed_get(SignalChecker *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!self->prepare_handle || !self->check_handle) {
        Py_RETURN_TRUE;
    } else {
        return PyBool_FromLong((long)(uv_is_closing((uv_handle_t *)self->prepare_handle) && uv_is_closing((uv_handle_t *)self->check_handle)));
    }
}


static int
SignalChecker_tp_init(SignalChecker *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Bool error = False;
    uv_prepare_t *uv_prepare = NULL;
    uv_check_t *uv_check = NULL;
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (self->prepare_handle || self->check_handle) {
        PyErr_SetString(PyExc_SignalCheckerError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_prepare = PyMem_Malloc(sizeof(uv_prepare_t));
    if (!uv_prepare) {
        PyErr_NoMemory();
        goto error;
    }

    uv_check = PyMem_Malloc(sizeof(uv_check_t));
    if (!uv_check) {
        PyErr_NoMemory();
        goto error;
    }

    r = uv_prepare_init(UV_LOOP(self), uv_prepare);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
        error = True;
    }
    r = uv_check_init(UV_LOOP(self), uv_check);
    if (r != 0) {
        if (!error) {
            RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_SignalCheckerError);
            error = True;
        }
    }

    if (error) {
        goto error;
    }

    uv_prepare->data = (void *)self;
    uv_check->data = (void *)self;
    self->prepare_handle = uv_prepare;
    self->check_handle = uv_check;
    return 0;

error:
    if (uv_prepare) {
        PyMem_Free(uv_prepare);
    }
    if (uv_check) {
        PyMem_Free(uv_check);
    }
    Py_DECREF(loop);
    return -1;
}


static PyObject *
SignalChecker_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    SignalChecker *self = (SignalChecker *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->prepare_handle = NULL;
    self->check_handle = NULL;
    return (PyObject *)self;
}


static int
SignalChecker_tp_traverse(SignalChecker *self, visitproc visit, void *arg)
{
    Py_VISIT(self->loop);
    return 0;
}


static int
SignalChecker_tp_clear(SignalChecker *self)
{
    Py_CLEAR(self->loop);
    return 0;
}


static void
SignalChecker_tp_dealloc(SignalChecker *self)
{
    if (self->prepare_handle) {
        uv_close((uv_handle_t *)self->prepare_handle, on_handle_dealloc_close);
    }
    if (self->check_handle) {
        uv_close((uv_handle_t *)self->check_handle, on_handle_dealloc_close);
    }
    Py_TYPE(self)->tp_clear((PyObject *)self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
SignalChecker_tp_methods[] = {
    { "start", (PyCFunction)SignalChecker_func_start, METH_NOARGS, "Start the SignalChecker." },
    { "stop", (PyCFunction)SignalChecker_func_stop, METH_NOARGS, "Stop the SignalChecker." },
    { "close", (PyCFunction)SignalChecker_func_close, METH_NOARGS, "Close the SignalChecker." },
    { NULL }
};


static PyMemberDef SignalChecker_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(SignalChecker, loop), READONLY, "Loop where this handle belongs."},
    {NULL}
};


static PyGetSetDef SignalChecker_tp_getsets[] = {
    {"active", (getter)SignalChecker_active_get, NULL, "Indicates if this handle is active.", NULL},
    {"closed", (getter)SignalChecker_closed_get, NULL, "Indicates if this handle is closing or already closed.", NULL},
    {NULL}
};


static PyTypeObject SignalCheckerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.SignalChecker",                                           /*tp_name*/
    sizeof(SignalChecker),                                          /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)SignalChecker_tp_dealloc,                           /*tp_dealloc*/
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
    (traverseproc)SignalChecker_tp_traverse,                        /*tp_traverse*/
    (inquiry)SignalChecker_tp_clear,                                /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    SignalChecker_tp_methods,                                       /*tp_methods*/
    SignalChecker_tp_members,                                       /*tp_members*/
    SignalChecker_tp_getsets,                                       /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)SignalChecker_tp_init,                                /*tp_init*/
    0,                                                              /*tp_alloc*/
    SignalChecker_tp_new,                                           /*tp_new*/
};


