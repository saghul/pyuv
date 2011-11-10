
static Loop *default_loop = NULL;


static PyObject *
new_loop(PyTypeObject *type, PyObject *args, PyObject *kwargs, int is_default)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_Check(kwargs) && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Loop initialization takes no parameters");
        return NULL;
    }

    if (is_default) {
        if (!default_loop) {
            default_loop = (Loop *)PyType_GenericNew(type, args, kwargs);
            if (!default_loop) {
                return NULL;
            }
            default_loop->uv_loop = uv_default_loop();
            default_loop->is_default = 1;
        } else {
            Py_INCREF(default_loop);
        }
        return (PyObject *)default_loop;
    } else {
        Loop *self = (Loop *)PyType_GenericNew(type, args, kwargs);
        if (!self) {
            return NULL;
        }
        self->uv_loop = uv_loop_new();
        self->is_default = 0;
        return (PyObject *)self;
    }
}


static PyObject *
Loop_func_run(Loop *self)
{
    Py_BEGIN_ALLOW_THREADS
    uv_run(self->uv_loop);
    Py_END_ALLOW_THREADS
    if (PyErr_Occurred()) {
        return NULL;
    }
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


static PyObject *
Loop_func_now(Loop *self)
{
    return PyLong_FromDouble(uv_now(self->uv_loop));
}


static PyObject *
Loop_func_update_time(Loop *self)
{
    uv_update_time(self->uv_loop);
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_default_loop(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    return new_loop(&LoopType, args, kwargs, 1);
}


static PyObject *
Loop_default_get(Loop *self, void *closure)
{
    if (self->is_default) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static PyObject *
Loop_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return new_loop(type, args, kwargs, 0);
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
    if (self->uv_loop && !self->is_default) {
        uv_loop_delete(self->uv_loop);
    }
    Loop_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Loop_tp_methods[] = {
    { "run", (PyCFunction)Loop_func_run, METH_NOARGS, "Run the event loop." },
    { "ref", (PyCFunction)Loop_func_ref, METH_NOARGS, "Increase the event loop reference count." },
    { "unref", (PyCFunction)Loop_func_unref, METH_NOARGS, "Decrease the event loop reference count." },
    { "now", (PyCFunction)Loop_func_now, METH_NOARGS, "Return high resolution time, expressed in nanoseconds." },
    { "update_time", (PyCFunction)Loop_func_update_time, METH_NOARGS, "Update event loop's notion of time by querying the kernel." },
    { "default_loop", (PyCFunction)Loop_func_default_loop, METH_CLASS|METH_VARARGS|METH_KEYWORDS, "Instantiate the default loop." },
    { NULL }
};


static PyGetSetDef Loop_tp_getsets[] = {
    {"default", (getter)Loop_default_get, NULL, "Is this the default loop?", NULL},
    {NULL}
};


static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Loop",                                                    /*tp_name*/
    sizeof(Loop),                                                   /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Loop_tp_dealloc,                                    /*tp_dealloc*/
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
    (traverseproc)Loop_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Loop_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Loop_tp_methods,                                                /*tp_methods*/
    0,                                                              /*tp_members*/
    Loop_tp_getsets,                                                /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    0,                                                              /*tp_init*/
    0,                                                              /*tp_alloc*/
    Loop_tp_new,                                                    /*tp_new*/
};


