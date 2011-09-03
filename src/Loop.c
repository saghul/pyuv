
/*
 * TODO:
 * - do we need to call uv_init with multiple loops?
 * - Loop is not a singleton anymore!
 *
 */

static PyObject *
Loop_func_run(Loop *self)
{
    Py_BEGIN_ALLOW_THREADS
    uv_run(self->uv_loop);
    Py_END_ALLOW_THREADS
    /* TODO: check for some error? */
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_ref(Loop *self)
{
    uv_ref(self->uv_loop);
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_unref(Loop *self)
{
    uv_unref(self->uv_loop);
    Py_RETURN_NONE;
}


static int
Loop_tp_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_Check(kwargs) && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Loop.__init__() takes no parameters");
	return -1;
    }
    return 0;
}


static PyObject *
Loop_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static Loop *self = NULL;   /* Loop is a singleton */

    if (!self) {
        self = (Loop *)PyType_GenericNew(type, args, kwargs);
        if (!self) {
            return NULL;
        }
        self->uv_loop = uv_loop_new();
        /* Initialize libuv */
        uv_init();
    } else {
        Py_INCREF(self);
    }
    return (PyObject *)self;
}


static int
Loop_tp_traverse(Loop *self, visitproc visit, void *arg)
{
    return 0;
}


static int
Loop_tp_clear(Loop *self)
{
    return 0;
}


static void
Loop_tp_dealloc(Loop *self)
{
    Loop_tp_clear(self);
    if (self->uv_loop) {
        uv_loop_delete(self->uv_loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Loop_tp_methods[] = {
    { "run", (PyCFunction)Loop_func_run, METH_NOARGS, "Run the event loop." },
    { "ref", (PyCFunction)Loop_func_ref, METH_NOARGS, "Increase the event loop reference count." },
    { "unref", (PyCFunction)Loop_func_unref, METH_NOARGS, "Decrease the event loop reference count." },
    { NULL }
};


static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Loop",                                /*tp_name*/
    sizeof(Loop),                               /*tp_basicsize*/
    0,                                          /*tp_itemsize*/
    (destructor)Loop_tp_dealloc,                /*tp_dealloc*/
    0,                                          /*tp_print*/
    0,                                          /*tp_getattr*/
    0,                                          /*tp_setattr*/
    0,                                          /*tp_compare*/
    0,                                          /*tp_repr*/
    0,                                          /*tp_as_number*/
    0,                                          /*tp_as_sequence*/
    0,                                          /*tp_as_mapping*/
    0,                                          /*tp_hash */
    0,                                          /*tp_call*/
    0,                                          /*tp_str*/
    0,                                          /*tp_getattro*/
    0,                                          /*tp_setattro*/
    0,                                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,      /*tp_flags*/
    0,                                          /*tp_doc*/
    (traverseproc)Loop_tp_traverse,             /*tp_traverse*/
    (inquiry)Loop_tp_clear,                     /*tp_clear*/
    0,                                          /*tp_richcompare*/
    0,                                          /*tp_weaklistoffset*/
    0,                                          /*tp_iter*/
    0,                                          /*tp_iternext*/
    Loop_tp_methods,                            /*tp_methods*/
    0,                                          /*tp_members*/
    0,                                          /*tp_getsets*/
    0,                                          /*tp_base*/
    0,                                          /*tp_dict*/
    0,                                          /*tp_descr_get*/
    0,                                          /*tp_descr_set*/
    0,                                          /*tp_dictoffset*/
    (initproc)Loop_tp_init,                     /*tp_init*/
    0,                                          /*tp_alloc*/
    Loop_tp_new,                                /*tp_new*/
};


