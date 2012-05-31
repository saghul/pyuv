
static PyObject* PyExc_PrepareError;


static void
on_prepare_callback(uv_prepare_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Prepare *self;
    PyObject *result;

    ASSERT(handle);
    ASSERT(status == 0);

    self = (Prepare *)handle->data;
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
Prepare_func_start(Prepare *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_prepare_start((uv_prepare_t *)UV_HANDLE(self), on_prepare_callback);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PrepareError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Prepare_func_stop(Prepare *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_prepare_stop((uv_prepare_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PrepareError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    Py_RETURN_NONE;
}


static int
Prepare_tp_init(Prepare *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_prepare_t *uv_prepare = NULL;
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_PrepareError, "Object already initialized");
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
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_PrepareError);
        Py_DECREF(loop);
        return -1;
    }
    uv_prepare->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_prepare;

    return 0;
}


static PyObject *
Prepare_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Prepare *self = (Prepare *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Prepare_tp_traverse(Prepare *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Prepare_tp_clear(Prepare *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Prepare_tp_methods[] = {
    { "start", (PyCFunction)Prepare_func_start, METH_VARARGS, "Start the Prepare." },
    { "stop", (PyCFunction)Prepare_func_stop, METH_NOARGS, "Stop the Prepare." },
    { NULL }
};


static PyTypeObject PrepareType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Prepare",                                                 /*tp_name*/
    sizeof(Prepare),                                                /*tp_basicsize*/
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
    (traverseproc)Prepare_tp_traverse,                              /*tp_traverse*/
    (inquiry)Prepare_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Prepare_tp_methods,                                             /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Prepare_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    Prepare_tp_new,                                                 /*tp_new*/
};


