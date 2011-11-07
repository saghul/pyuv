
static PyObject* PyExc_IdleError;


static void
on_idle_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_idle_callback(uv_idle_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    ASSERT(status == 0);

    Idle *self = (Idle *)handle->data;
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
Idle_func_start(Idle *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;
    PyObject *data = Py_None;

    static char *kwlist[] = {"callback", "data", NULL};

    if (self->closed) {
        PyErr_SetString(PyExc_IdleError, "Idle is closed");
        return NULL;
    }


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:__init__", kwlist, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_idle_start(self->uv_idle, on_idle_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IdleError);
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
Idle_func_stop(Idle *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_IdleError, "Idle is already closed");
        return NULL;
    }

    int r = uv_idle_stop(self->uv_idle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IdleError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Idle_func_close(Idle *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_IdleError, "Idle is already closed");
        return NULL;
    }

    self->closed = True;
    uv_close((uv_handle_t *)self->uv_idle, on_idle_close);

    Py_RETURN_NONE;
}


static int
Idle_tp_init(Idle *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_idle_t *uv_idle = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_IdleError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_idle = PyMem_Malloc(sizeof(uv_idle_t));
    if (!uv_idle) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_idle_init(SELF_LOOP, uv_idle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_IdleError);
        Py_DECREF(loop);
        return -1;
    }
    uv_idle->data = (void *)self;
    self->uv_idle = uv_idle;

    self->initialized = True;
    self->closed = False;

    return 0;
}


static PyObject *
Idle_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Idle *self = (Idle *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static int
Idle_tp_traverse(Idle *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->loop);
    return 0;
}


static int
Idle_tp_clear(Idle *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Idle_tp_dealloc(Idle *self)
{
    if (!self->closed) {
        uv_close((uv_handle_t *)self->uv_idle, on_idle_close);
    }
    Idle_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Idle_tp_methods[] = {
    { "start", (PyCFunction)Idle_func_start, METH_VARARGS|METH_KEYWORDS, "Start the Idle." },
    { "stop", (PyCFunction)Idle_func_stop, METH_NOARGS, "Stop the Idle." },
    { "close", (PyCFunction)Idle_func_close, METH_NOARGS, "Close the Idle." },
    { NULL }
};


static PyMemberDef Idle_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Idle, loop), READONLY, "Loop where this Idle is running on."},
    {"data", T_OBJECT_EX, offsetof(Idle, data), 0, "Arbitrary data."},
    {NULL}
};


static PyTypeObject IdleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Idle",                                                    /*tp_name*/
    sizeof(Idle),                                                   /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Idle_tp_dealloc,                                    /*tp_dealloc*/
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
    (traverseproc)Idle_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Idle_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Idle_tp_methods,                                                /*tp_methods*/
    Idle_tp_members,                                                /*tp_members*/
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


