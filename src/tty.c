
static PyObject *
TTY_func_set_mode(TTY *self, PyObject *args)
{
    int err, mode;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:set_mode", &mode)) {
        return NULL;
    }

    err = uv_tty_set_mode(&self->tty_h, mode);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TTYError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
TTY_func_reset_mode(PyObject *cls)
{
    UNUSED_ARG(cls);
    uv_tty_reset_mode();
    Py_RETURN_NONE;
}


static PyObject *
TTY_func_get_winsize(TTY *self)
{
    int err, width, height;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_tty_get_winsize(&self->tty_h, &width, &height);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TTYError);
        return NULL;
    }

    return Py_BuildValue("(ii)", width, height);
}


static int
TTY_tp_init(TTY *self, PyObject *args, PyObject *kwargs)
{
    int fd, err;
    Loop *loop;
    PyObject *readable;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!iO!:__init__", &LoopType, &loop, &fd, &PyBool_Type, &readable)) {
        return -1;
    }

    err = uv_tty_init(loop->uv_loop, &self->tty_h, fd, (readable == Py_True) ? 1 : 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_TTYError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
TTY_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TTY *self;

    self = (TTY *)StreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->tty_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->tty_h;

    return (PyObject *)self;
}


static int
TTY_tp_traverse(TTY *self, visitproc visit, void *arg)
{
    return StreamType.tp_traverse((PyObject *)self, visit, arg);
}


static int
TTY_tp_clear(TTY *self)
{
    return StreamType.tp_clear((PyObject *)self);
}


static PyMethodDef
TTY_tp_methods[] = {
    { "set_mode", (PyCFunction)TTY_func_set_mode, METH_VARARGS, "Set TTY handle mode." },
    { "reset_mode", (PyCFunction)TTY_func_reset_mode, METH_CLASS|METH_NOARGS, "Reset TTY settings. To be called when program exits." },
    { "get_winsize", (PyCFunction)TTY_func_get_winsize, METH_NOARGS, "Get the currecnt Window size." },
    { NULL }
};


static PyTypeObject TTYType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.TTY",                                             /*tp_name*/
    sizeof(TTY),                                                   /*tp_basicsize*/
    0,                                                             /*tp_itemsize*/
    0,                                                             /*tp_dealloc*/
    0,                                                             /*tp_print*/
    0,                                                             /*tp_getattr*/
    0,                                                             /*tp_setattr*/
    0,                                                             /*tp_compare*/
    0,                                                             /*tp_repr*/
    0,                                                             /*tp_as_number*/
    0,                                                             /*tp_as_sequence*/
    0,                                                             /*tp_as_mapping*/
    0,                                                             /*tp_hash */
    0,                                                             /*tp_call*/
    0,                                                             /*tp_str*/
    0,                                                             /*tp_getattro*/
    0,                                                             /*tp_setattro*/
    0,                                                             /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,                                                             /*tp_doc*/
    (traverseproc)TTY_tp_traverse,                                 /*tp_traverse*/
    (inquiry)TTY_tp_clear,                                         /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    0,                                                             /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    TTY_tp_methods,                                                /*tp_methods*/
    0,                                                             /*tp_members*/
    0,                                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    0,                                                             /*tp_dictoffset*/
    (initproc)TTY_tp_init,                                         /*tp_init*/
    0,                                                             /*tp_alloc*/
    TTY_tp_new,                                                    /*tp_new*/
};

