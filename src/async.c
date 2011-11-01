
static PyObject* PyExc_AsyncError;


static void
on_async_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    /* Decrement reference count of the object this handle was keeping alive */
    PyObject *obj = (PyObject *)handle->data;
    Py_DECREF(obj);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_async_callback(uv_async_t *async, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(async);
    ASSERT(status == 0);

    Async *self = (Async *)async->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    PyObject *result;
    result = PyObject_CallFunctionObjArgs(self->callback, self, self->data,NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->callback);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Async_func_send(Async *self, PyObject *args)
{
    int r = 0;
    PyObject *tmp = NULL;
    PyObject *callback;
    PyObject *data = Py_None;

    if (self->closed) {
        PyErr_SetString(PyExc_AsyncError, "async is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "O|O:send", &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_async_send(self->uv_async);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_AsyncError);
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
Async_func_close(Async *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_AsyncError, "Async is already closed");
        return NULL;
    }

    self->closed = True;
    uv_close((uv_handle_t *)self->uv_async, on_async_close);

    Py_RETURN_NONE;
}


static int
Async_tp_init(Async *self, PyObject *args, PyObject *kwargs)
{
    int r = 0;
    Loop *loop;
    PyObject *tmp = NULL;
    uv_async_t *uv_async = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_AsyncError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_async = PyMem_Malloc(sizeof(uv_async_t));
    if (!uv_async) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_async_init(SELF_LOOP, uv_async, on_async_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_AsyncError);
        Py_DECREF(loop);
        return -1;
    }
    uv_async->data = (void *)self;
    self->uv_async = uv_async;

    /* Increment reference count while libuv keeps this object around. It'll be decremented on handle close. */
    Py_INCREF(self);

    self->initialized = True;
    self->closed = False;

    return 0;
}


static PyObject *
Async_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Async *self = (Async *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
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
    Async_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Async_tp_methods[] = {
    { "send", (PyCFunction)Async_func_send, METH_VARARGS, "Send the Async signal." },
    { "close", (PyCFunction)Async_func_close, METH_NOARGS, "Close this Async handle." },
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



