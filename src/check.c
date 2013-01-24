
static void
on_check_callback(uv_check_t *handle, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Check *self;
    PyObject *result;

    ASSERT(handle);
    ASSERT(status == 0);

    self = (Check *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    result = PyObject_CallFunctionObjArgs(self->callback, self, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Check_func_start(Check *self, PyObject *args)
{
    int r;
    PyObject *tmp, *callback;

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "O:start", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_check_start((uv_check_t *)UV_HANDLE(self), on_check_callback);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_CheckError);
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
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_check_stop((uv_check_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_CheckError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    Py_RETURN_NONE;
}


static int
Check_tp_init(Check *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    r = uv_check_init(loop->uv_loop, (uv_check_t *)UV_HANDLE(self));
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_CheckError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
Check_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    uv_check_t *uv_check;

    uv_check = PyMem_Malloc(sizeof *uv_check);
    if (!uv_check) {
        PyErr_NoMemory();
        return NULL;
    }

    Check *self = (Check *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        PyMem_Free(uv_check);
        return NULL;
    }

    uv_check->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_check;

    return (PyObject *)self;
}


static int
Check_tp_traverse(Check *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Check_tp_clear(Check *self)
{
    Py_CLEAR(self->callback);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Check_tp_methods[] = {
    { "start", (PyCFunction)Check_func_start, METH_VARARGS, "Start the Check." },
    { "stop", (PyCFunction)Check_func_stop, METH_NOARGS, "Stop the Check." },
    { NULL }
};


static PyTypeObject CheckType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Check",                                                   /*tp_name*/
    sizeof(Check),                                                  /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Check_tp_traverse,                                /*tp_traverse*/
    (inquiry)Check_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Check_tp_methods,                                               /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Check_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    Check_tp_new,                                                   /*tp_new*/
};


