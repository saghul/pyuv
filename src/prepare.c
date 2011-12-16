
static PyObject* PyExc_PrepareError;


static void
on_prepare_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Prepare *self;
    PyObject *result;
    
    ASSERT(handle);
    self = (Prepare *)handle->data;
    ASSERT(self);

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
on_prepare_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


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
Prepare_func_start(Prepare *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;

    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_prepare_start(self->uv_handle, on_prepare_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PrepareError);
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
    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is already closed");
        return NULL;
    }

    r = uv_prepare_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PrepareError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Prepare_func_close(Prepare *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is already closed");
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

    self->closed = True;
    uv_close((uv_handle_t *)self->uv_handle, on_prepare_close);

    Py_RETURN_NONE;
}


static PyObject *
Prepare_active_get(Prepare *self, void *closure)
{
    return PyBool_FromLong((long)uv_is_active((uv_handle_t *)self->uv_handle));
}


static int
Prepare_tp_init(Prepare *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    uv_prepare_t *uv_prepare = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_PrepareError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    Py_INCREF(loop);
    self->loop = loop;

    uv_prepare = PyMem_Malloc(sizeof(uv_prepare_t));
    if (!uv_prepare) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_prepare_init(UV_LOOP(self), uv_prepare);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PrepareError);
        Py_DECREF(loop);
        return -1;
    }
    uv_prepare->data = (void *)self;
    self->uv_handle = uv_prepare;

    self->initialized = True;

    return 0;
}


static PyObject *
Prepare_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Prepare *self = (Prepare *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    self->closed = False;
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
Prepare_tp_traverse(Prepare *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
Prepare_tp_clear(Prepare *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Prepare_tp_dealloc(Prepare *self)
{
    if (!self->closed && self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_prepare_dealloc_close);
    }
    Prepare_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Prepare_tp_methods[] = {
    { "start", (PyCFunction)Prepare_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Prepare." },
    { "stop", (PyCFunction)Prepare_func_stop, METH_NOARGS, "Stop the Prepare." },
    { "close", (PyCFunction)Prepare_func_close, METH_VARARGS, "Close the Prepare." },
    { NULL }
};


static PyMemberDef Prepare_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Prepare, loop), READONLY, "Loop where this Prepare is running on."},
    {"data", T_OBJECT, offsetof(Prepare, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef Prepare_tp_getsets[] = {
    {"active", (getter)Prepare_active_get, 0, "Indicates if handle is active", NULL},
    {NULL}
};


static PyTypeObject PrepareType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Prepare",                                                 /*tp_name*/
    sizeof(Prepare),                                                /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Prepare_tp_dealloc,                                 /*tp_dealloc*/
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
    Prepare_tp_members,                                             /*tp_members*/
    Prepare_tp_getsets,                                             /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Prepare_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    Prepare_tp_new,                                                 /*tp_new*/
};


