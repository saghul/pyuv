
static PyObject* PyExc_CheckError;


static void
on_check_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    Check *self = (Check *)handle->data;
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
on_check_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_check_callback(uv_check_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    ASSERT(status == 0);

    Check *self = (Check *)handle->data;
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
Check_func_start(Check *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_CheckError, "Check is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O:start", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_check_start(self->uv_handle, on_check_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_CheckError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Check_func_stop(Check *self)
{
    if (!self->uv_handle) {
        PyErr_SetString(PyExc_CheckError, "Check is already closed");
        return NULL;
    }

    int r = uv_check_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_CheckError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Check_func_close(Check *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_CheckError, "Check is already closed");
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

    uv_close((uv_handle_t *)self->uv_handle, on_check_close);
    self->uv_handle = NULL;

    Py_RETURN_NONE;
}


static PyObject *
Check_active_get(Check *self, void *closure)
{
    return PyBool_FromLong((long)uv_is_active((uv_handle_t *)self->uv_handle));
}


static int
Check_tp_init(Check *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_check_t *uv_check = NULL;

    if (self->uv_handle) {
        PyErr_SetString(PyExc_CheckError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_check = PyMem_Malloc(sizeof(uv_check_t));
    if (!uv_check) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_check_init(UV_LOOP(self), uv_check);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_CheckError);
        Py_DECREF(loop);
        return -1;
    }
    uv_check->data = (void *)self;
    self->uv_handle = uv_check;

    return 0;
}


static PyObject *
Check_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Check *self = (Check *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
Check_tp_traverse(Check *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
Check_tp_clear(Check *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Check_tp_dealloc(Check *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_check_dealloc_close);
        self->uv_handle = NULL;
    }
    Check_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Check_tp_methods[] = {
    { "start", (PyCFunction)Check_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Check." },
    { "stop", (PyCFunction)Check_func_stop, METH_NOARGS, "Stop the Check." },
    { "close", (PyCFunction)Check_func_close, METH_VARARGS, "Close the Check." },
    { NULL }
};


static PyMemberDef Check_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Check, loop), READONLY, "Loop where this Check is running on."},
    {"data", T_OBJECT, offsetof(Check, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef Check_tp_getsets[] = {
    {"active", (getter)Check_active_get, 0, "Indicates if handle is active", NULL},
    {NULL}
};


static PyTypeObject CheckType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Check",                                                   /*tp_name*/
    sizeof(Check),                                                  /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Check_tp_dealloc,                                   /*tp_dealloc*/
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
    (traverseproc)Check_tp_traverse,                                /*tp_traverse*/
    (inquiry)Check_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Check_tp_methods,                                               /*tp_methods*/
    Check_tp_members,                                               /*tp_members*/
    Check_tp_getsets,                                               /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Check_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    Check_tp_new,                                                   /*tp_new*/
};


