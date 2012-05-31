
static PyObject* PyExc_TimerError;


static void
on_timer_callback(uv_timer_t *timer, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Timer *self;
    PyObject *result;

    ASSERT(timer);
    ASSERT(status == 0);

    self = (Timer *)(timer->data);
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

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
    int r;
    double timeout, repeat;
    PyObject *tmp, *callback;

    static char *kwlist[] = {"callback", "timeout", "repeat", NULL};

    tmp = NULL;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

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

    r = uv_timer_start((uv_timer_t *)UV_HANDLE(self), on_timer_callback, (int64_t)(timeout * 1000), (int64_t)(repeat * 1000));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TimerError);
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
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_timer_stop((uv_timer_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TimerError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Timer_func_again(Timer *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_timer_again((uv_timer_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TimerError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Timer_repeat_get(Timer *self, void *closure)
{
    UNUSED_ARG(closure);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);
    return PyFloat_FromDouble(uv_timer_get_repeat((uv_timer_t *)UV_HANDLE(self))/1000.0);
}


static int
Timer_repeat_set(Timer *self, PyObject *value, void *closure)
{
    double repeat;

    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, -1);

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

    uv_timer_set_repeat((uv_timer_t *)UV_HANDLE(self), (int64_t)(repeat * 1000));

    return 0;
}


static int
Timer_tp_init(Timer *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_timer_t *uv_timer = NULL;
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_TimerError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    uv_timer = PyMem_Malloc(sizeof(uv_timer_t));
    if (!uv_timer) {
        PyErr_NoMemory();
        return -1;
    }

    r = uv_timer_init(UV_HANDLE_LOOP(self), uv_timer);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_TimerError);
        PyMem_Free(uv_timer);
        Py_DECREF(loop);
        return -1;
    }
    uv_timer->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_timer;

    return 0;
}


static PyObject *
Timer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Timer *self = (Timer *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Timer_tp_traverse(Timer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Timer_tp_clear(Timer *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Timer_tp_methods[] = {
    { "start", (PyCFunction)Timer_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Timer." },
    { "stop", (PyCFunction)Timer_func_stop, METH_NOARGS, "Stop the Timer." },
    { "again", (PyCFunction)Timer_func_again, METH_NOARGS, "Stop the timer, and if it is repeating restart it using the repeat value as the timeout." },
    { NULL }
};


static PyGetSetDef Timer_tp_getsets[] = {
    {"repeat", (getter)Timer_repeat_get, (setter)Timer_repeat_set, "Timer repeat value.", NULL},
    {NULL}
};


static PyTypeObject TimerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Timer",                                                   /*tp_name*/
    sizeof(Timer),                                                  /*tp_basicsize*/
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
    (traverseproc)Timer_tp_traverse,                                /*tp_traverse*/
    (inquiry)Timer_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Timer_tp_methods,                                               /*tp_methods*/
    0,                                                              /*tp_members*/
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


