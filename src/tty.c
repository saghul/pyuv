
static PyObject* PyExc_TTYError;


static PyObject *
TTY_func_set_mode(TTY *self, PyObject *args)
{
    int r, mode;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TTYError, "already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:set_mode", &mode)) {
        return NULL;
    }

    r = uv_tty_set_mode((uv_tty_t *)base->uv_handle, mode);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TTYError);
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
TTY_func_isatty(PyObject *cls, PyObject *args)
{
    int fd;

    UNUSED_ARG(cls);

    if (!PyArg_ParseTuple(args, "i:isatty", &fd)) {
        return NULL;
    }

    if (uv_guess_handle(fd) == UV_TTY) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static PyObject *
TTY_func_get_winsize(TTY *self)
{
    int r, width, height;

    IOStream *base = (IOStream *)self;

    if (!base->uv_handle) {
        PyErr_SetString(PyExc_TTYError, "already closed");
        return NULL;
    }

    r = uv_tty_get_winsize((uv_tty_t *)base->uv_handle, &width, &height);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TTYError);
        return NULL;
    }

    return Py_BuildValue("(ii)", width, height);
}


static int
TTY_tp_init(TTY *self, PyObject *args, PyObject *kwargs)
{
    int fd, r;
    uv_tty_t *uv_stream;
    Loop *loop;
    PyObject *tmp = NULL;

    IOStream *base = (IOStream *)self;

    UNUSED_ARG(kwargs);

    if (base->uv_handle) {
        PyErr_SetString(PyExc_IOStreamError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!i:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

    if (fd != 0 && fd != 1 && fd != 2) {
        PyErr_SetString(PyExc_TTYError, "Incorrect file descriptor specified");
        return -1;
    }

    tmp = (PyObject *)base->loop;
    Py_INCREF(loop);
    base->loop = loop;
    Py_XDECREF(tmp);

    uv_stream = PyMem_Malloc(sizeof(uv_tty_t));
    if (!uv_stream) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_tty_init(UV_LOOP(base), uv_stream, fd, (fd == 0)?1:0);
    if (r != 0) {
        raise_uv_exception(base->loop, PyExc_TTYError);
        Py_DECREF(loop);
        return -1;
    }
    uv_stream->data = (void *)self;
    base->uv_handle = (uv_stream_t *)uv_stream;

    return 0;
}


static PyObject *
TTY_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TTY *self = (TTY *)IOStreamType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
TTY_tp_traverse(TTY *self, visitproc visit, void *arg)
{
    IOStreamType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
TTY_tp_clear(TTY *self)
{
    IOStreamType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
TTY_tp_methods[] = {
    { "set_mode", (PyCFunction)TTY_func_set_mode, METH_VARARGS, "Set TTY handle mode." },
    { "reset_mode", (PyCFunction)TTY_func_reset_mode, METH_CLASS|METH_NOARGS, "Reset TTY settings. To be called when program exits." },
    { "get_winsize", (PyCFunction)TTY_func_get_winsize, METH_NOARGS, "Get the currecnt Window size." },
    { "isatty", (PyCFunction)TTY_func_isatty, METH_CLASS|METH_VARARGS, "Check if the given file descriptor is associated with a terminal." },
    { NULL }
};


static PyTypeObject TTYType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TTY",                                                    /*tp_name*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                       /*tp_flags*/
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


