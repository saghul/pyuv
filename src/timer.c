
static PyObject* PyExc_TimerError;


static void
on_timer_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    Timer *self = (Timer *)handle->data;
    ASSERT(self);

    PyObject *result;

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_close_cb);
        }
        Py_XDECREF(result);
    }

    handle->data = NULL;
    PyMem_Free(handle);

    /* Refcount was increased in func_close */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_timer_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_timer_callback(uv_timer_t *timer, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(timer);
    ASSERT(status == 0);

    Timer *self = (Timer *)(timer->data);
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result;
    result = PyObject_CallFunctionObjArgs(self->callback, self, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->callback);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Timer_func_start(Timer *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    double timeout;
    double repeat;
    PyObject *tmp = NULL;
    PyObject *callback;

    static char *kwlist[] = {"callback", "timeout", "repeat", NULL};

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is closed");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Odd:__init__", kwlist, &callback, &timeout, &repeat)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (timeout < 0.0) {
        PyErr_SetString(PyExc_ValueError, "a positive value or zero is required");
        return NULL;
    }

    if (repeat < 0.0) {
        PyErr_SetString(PyExc_ValueError, "a positive value or zero is required");
        return NULL;
    }

    r = uv_timer_start(self->uv_handle, on_timer_callback, (int64_t)(timeout * 1000), (int64_t)(repeat * 1000));
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TimerError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Timer_func_stop(Timer *self)
{
    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is closed");
        return NULL;
    }

    int r = uv_timer_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TimerError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Timer_func_again(Timer *self)
{
    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is closed");
        return NULL;
    }

    int r = uv_timer_again(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TimerError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Timer_func_close(Timer *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:close", &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    Py_INCREF(callback);
    self->on_close_cb = callback;

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    uv_close((uv_handle_t *)self->uv_handle, on_timer_close);
    self->uv_handle = NULL;

    Py_RETURN_NONE;
}


static PyObject *
Timer_active_get(Timer *self, void *closure)
{
    if (!self->uv_handle) {
        Py_RETURN_FALSE;
    } else {
        return PyBool_FromLong((long)uv_is_active((uv_handle_t *)self->uv_handle));
    }
}


static PyObject *
Timer_repeat_get(Timer *self, void *closure)
{
    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is closed");
        return NULL;
    }

    return PyFloat_FromDouble(uv_timer_get_repeat(self->uv_handle)/1000.0);
}


static int
Timer_repeat_set(Timer *self, PyObject *value, void *closure)
{
    double repeat;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Timer is closed");
        return -1;
    }

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }

    repeat = PyFloat_AsDouble(value);
    if (repeat == -1 && PyErr_Occurred()) {
        return -1;
    }

    if (repeat < 0.0) {
        PyErr_SetString(PyExc_ValueError, "a positive float or 0.0 is required");
        return -1;
    }

    uv_timer_set_repeat(self->uv_handle, (int64_t)(repeat * 1000));

    return 0;
}


static int
Timer_tp_init(Timer *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_timer_t *uv_timer = NULL;

    if (self->uv_handle) {
        PyErr_SetString(PyExc_TimerError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_timer = PyMem_Malloc(sizeof(uv_timer_t));
    if (!uv_timer) {
        PyErr_NoMemory();
        return -1;
    }

    r = uv_timer_init(UV_LOOP(self), uv_timer);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_TimerError);
        PyMem_Free(uv_timer);
        Py_DECREF(loop);
        return -1;
    }
    uv_timer->data = (void *)self;
    self->uv_handle = uv_timer;

    return 0;
}


static PyObject *
Timer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Timer *self = (Timer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
Timer_tp_traverse(Timer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
Timer_tp_clear(Timer *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Timer_tp_dealloc(Timer *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_timer_dealloc_close);
    }
    Timer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Timer_tp_methods[] = {
    { "start", (PyCFunction)Timer_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Timer." },
    { "stop", (PyCFunction)Timer_func_stop, METH_NOARGS, "Stop the Timer." },
    { "again", (PyCFunction)Timer_func_again, METH_NOARGS, "Stop the timer, and if it is repeating restart it using the repeat value as the timeout." },
    { "close", (PyCFunction)Timer_func_close, METH_VARARGS, "Close the Timer." },
    { NULL }
};


static PyMemberDef Timer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Timer, loop), READONLY, "Loop where this Timer is running on."},
    {"data", T_OBJECT, offsetof(Timer, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef Timer_tp_getsets[] = {
    {"active", (getter)Timer_active_get, 0, "Indicates if handle is active.", NULL},
    {"repeat", (getter)Timer_repeat_get, (setter)Timer_repeat_set, "Timer repeat value.", NULL},
    {NULL}
};


static PyTypeObject TimerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Timer",                                                   /*tp_name*/
    sizeof(Timer),                                                  /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Timer_tp_dealloc,                                   /*tp_dealloc*/
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
    (traverseproc)Timer_tp_traverse,                                /*tp_traverse*/
    (inquiry)Timer_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Timer_tp_methods,                                               /*tp_methods*/
    Timer_tp_members,                                               /*tp_members*/
    Timer_tp_getsets,                                               /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Timer_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    Timer_tp_new,                                                   /*tp_new*/
};


