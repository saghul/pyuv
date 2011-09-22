
static PyObject* PyExc_AsyncError;


static void
on_async_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_async_callback(uv_async_t *async, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(async);
    ASSERT(status == 0);

    Async *self = (Async *)(async->data);
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result;

    if (self->callback != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->callback, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->callback);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Async_func_send(Async *self)
{
    if (!self->uv_async) {
        PyErr_SetString(PyExc_AsyncError, "async was destroyed");
        return NULL;
    }

    uv_async_send(self->uv_async); 
    Py_RETURN_NONE;
}


static PyObject *
Async_func_destroy(Async *self)
{
    if (!self->uv_async) {
        PyErr_SetString(PyExc_AsyncError, "async was already destroyed");
        return NULL;
    }

    self->uv_async->data = NULL;
    uv_close((uv_handle_t *)self->uv_async, on_async_close);
    self->uv_async = NULL;

    Py_RETURN_NONE;
}


static int
Async_tp_init(Async *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;
    PyObject *callback;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O|O:__init__", kwlist, &LoopType, &loop, &callback, &data)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return -1;
    } else {
        tmp = self->callback;
        Py_INCREF(callback);
        self->callback = callback;
        Py_XDECREF(tmp);
    }

    if (data) {
        tmp = self->data;
        Py_INCREF(data);
        self->data = data;
        Py_XDECREF(tmp);
    }

    uv_async_t *uv_async = PyMem_Malloc(sizeof(uv_async_t));
    if (!uv_async) {
        PyErr_NoMemory();
        return -1;
    }
    int r = uv_async_init(SELF_LOOP, uv_async, on_async_callback);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_AsyncError, -1);
    }
    uv_async->data = (void *)self;
    self->uv_async = uv_async;

    return 0;
}


static PyObject *
Async_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Async *self = (Async *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Async_tp_traverse(Async *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->loop);
    return 0;
}


static int
Async_tp_clear(Async *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Async_tp_dealloc(Async *self)
{
    if (self->uv_async) {
        self->uv_async->data = NULL;
        uv_close((uv_handle_t *)self->uv_async, on_async_close);
        self->uv_async = NULL;
    }
    Async_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Async_tp_methods[] = {
    { "send", (PyCFunction)Async_func_send, METH_NOARGS, "Send the Async signal." },
    { "destroy", (PyCFunction)Async_func_destroy, METH_NOARGS, "Destroy the Async." },
    { NULL }
};


static PyMemberDef Async_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Async, loop), READONLY, "Loop where this Async is running on."},
    {"data", T_OBJECT_EX, offsetof(Async, data), 0, "Arbitrary data."},
    {NULL}
};


static PyTypeObject AsyncType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Async",                                                   /*tp_name*/
    sizeof(Async),                                                  /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Async_tp_dealloc,                                   /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,      /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Async_tp_traverse,                                /*tp_traverse*/
    (inquiry)Async_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Async_tp_methods,                                               /*tp_methods*/
    Async_tp_members,                                               /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Async_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    Async_tp_new,                                                   /*tp_new*/
};



