
static PyObject* PyExc_AsyncError;


static void
on_async_callback(uv_async_t *async, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Async *self;
    PyObject *result;

    ASSERT(async);
    ASSERT(status == 0);

    self = (Async *)async->data;
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
Async_func_send(Async *self)
{
    int r;

    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_async_send((uv_async_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_LOOP((Handle *)self), PyExc_AsyncError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
Async_tp_init(Async *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_async_t *uv_async = NULL;
    Loop *loop;
    PyObject *callback;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_AsyncError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!O:__init__", &LoopType, &loop, &callback)) {
        return -1;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    uv_async = PyMem_Malloc(sizeof(uv_async_t));
    if (!uv_async) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_async_init(UV_HANDLE_LOOP(self), uv_async, on_async_callback);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_AsyncError);
        Py_DECREF(loop);
        return -1;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    uv_async->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_async;

    return 0;
}


static PyObject *
Async_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Async *self = (Async *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Async_tp_traverse(Async *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Async_tp_clear(Async *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Async_tp_methods[] = {
    { "send", (PyCFunction)Async_func_send, METH_NOARGS, "Send the Async signal." },
    { NULL }
};


static PyTypeObject AsyncType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Async",                                                   /*tp_name*/
    sizeof(Async),                                                  /*tp_basicsize*/
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
    (traverseproc)Async_tp_traverse,                                /*tp_traverse*/
    (inquiry)Async_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Async_tp_methods,                                               /*tp_methods*/
    0,                                                              /*tp_members*/
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



