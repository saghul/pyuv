
static PyObject* PyExc_PrepareError;


static void
on_prepare_close(uv_handle_t *handle)
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
    ASSERT(handle);
    ASSERT(status == 0);

    Prepare *self = (Prepare *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result;

    result = PyObject_CallFunctionObjArgs(self->callback, self, self->data, NULL);
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
    PyObject *data = Py_None;

    static char *kwlist[] = {"callback", "data", NULL};

    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is closed");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:__init__", kwlist, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_prepare_start(self->uv_prepare, on_prepare_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PrepareError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    tmp = self->data;
    Py_INCREF(data);
    self->data = data;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Prepare_func_stop(Prepare *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is already closed");
        return NULL;
    }

    int r = uv_prepare_stop(self->uv_prepare);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PrepareError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Prepare_func_close(Prepare *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_PrepareError, "Prepare is already closed");
        return NULL;
    }

    self->closed = True;
    uv_close((uv_handle_t *)self->uv_prepare, on_prepare_close);

    Py_RETURN_NONE;
}


static int
Prepare_tp_init(Prepare *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_prepare_t *uv_prepare = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_PrepareError, "Object already initialized");
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
    self->uv_prepare = uv_prepare;

    self->initialized = True;
    self->closed = False;

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
    return (PyObject *)self;
}


static int
Prepare_tp_traverse(Prepare *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->loop);
    return 0;
}


static int
Prepare_tp_clear(Prepare *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Prepare_tp_dealloc(Prepare *self)
{
    if (!self->closed) {
        uv_close((uv_handle_t *)self->uv_prepare, on_prepare_close);
    }
    Prepare_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Prepare_tp_methods[] = {
    { "start", (PyCFunction)Prepare_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Prepare." },
    { "stop", (PyCFunction)Prepare_func_stop, METH_NOARGS, "Stop the Prepare." },
    { "close", (PyCFunction)Prepare_func_close, METH_NOARGS, "Close the Prepare." },
    { NULL }
};


static PyMemberDef Prepare_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Prepare, loop), READONLY, "Loop where this Prepare is running on."},
    {"data", T_OBJECT_EX, offsetof(Prepare, data), 0, "Arbitrary data."},
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Prepare_tp_traverse,                              /*tp_traverse*/
    (inquiry)Prepare_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Prepare_tp_methods,                                             /*tp_methods*/
    Prepare_tp_members,                                             /*tp_members*/
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


