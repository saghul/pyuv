
static void
pyuv__fspoll_cb(uv_fs_poll_t *handle, int status, const uv_stat_t *prev, const uv_stat_t *curr)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    FSPoll *self;
    PyObject *result, *errorno, *prev_stat_data, *curr_stat_data;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, FSPoll, fspoll_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status < 0) {
        errorno = PyLong_FromLong((long)status);
        prev_stat_data = Py_None;
        curr_stat_data = Py_None;
        Py_INCREF(Py_None);
        Py_INCREF(Py_None);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
        prev_stat_data = PyStructSequence_New(&StatResultType);
        if (!prev_stat_data) {
            PyErr_Clear();
            prev_stat_data = Py_None;
            Py_INCREF(Py_None);
        } else {
            stat_to_pyobj((uv_stat_t *)prev, prev_stat_data);
        }
        curr_stat_data = PyStructSequence_New(&StatResultType);
        if (!curr_stat_data) {
            PyErr_Clear();
            curr_stat_data = Py_None;
            Py_INCREF(Py_None);
        } else {
            stat_to_pyobj((uv_stat_t *)curr, curr_stat_data);
        }
    }

    result = PyObject_CallFunctionObjArgs(self->callback, self, prev_stat_data, curr_stat_data, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
FSPoll_func_start(FSPoll *self, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    double interval;
    PyObject *tmp, *callback;

    static char *kwlist[] = {"path", "interval", "callback", NULL};

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sdO:start", kwlist, &path, &interval, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (interval < 0.0) {
        PyErr_SetString(PyExc_ValueError, "a positive value or zero is required");
        return NULL;
    }

    err = uv_fs_poll_start(&self->fspoll_h, pyuv__fspoll_cb, path, (unsigned int)interval*1000);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSPollError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    PYUV_HANDLE_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
FSPoll_func_stop(FSPoll *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_fs_poll_stop(&self->fspoll_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSPollError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static PyObject *
FSPoll_path_get(FSPoll *self, void *closure)
{
#ifdef _WIN32
    /* MAX_PATH is in characters, not bytes. Make sure we have enough headroom. */
    char buf[MAX_PATH * 4];
#else
    char buf[PATH_MAX];
#endif
    size_t buf_len;
    int err;

    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    buf_len = sizeof(buf);
    err = uv_fs_poll_getpath(&self->fspoll_h, buf, &buf_len);
    if (err < 0) {
        return PyBytes_FromString("");
    }

    return PyUnicode_DecodeFSDefaultAndSize(buf, buf_len);
}


static int
FSPoll_tp_init(FSPoll *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_fs_poll_init(loop->uv_loop, &self->fspoll_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSPollError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
FSPoll_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FSPoll *self;

    self = (FSPoll *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->fspoll_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->fspoll_h;

    return (PyObject *)self;
}


static int
FSPoll_tp_traverse(FSPoll *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
FSPoll_tp_clear(FSPoll *self)
{
    Py_CLEAR(self->callback);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
FSPoll_tp_methods[] = {
    { "start", (PyCFunction)FSPoll_func_start, METH_VARARGS | METH_KEYWORDS, "Start the FSPoll handle." },
    { "stop", (PyCFunction)FSPoll_func_stop, METH_NOARGS, "Stop the FSPoll handle." },
    { NULL }
};


static PyGetSetDef FSPoll_tp_getsets[] = {
    {"path", (getter)FSPoll_path_get, NULL, "Path being monitored.", NULL},
    {NULL}
};


static PyTypeObject FSPollType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.fs.FSPoll",                                        /*tp_name*/
    sizeof(FSPoll),                                                 /*tp_basicsize*/
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
    (traverseproc)FSPoll_tp_traverse,                               /*tp_traverse*/
    (inquiry)FSPoll_tp_clear,                                       /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    FSPoll_tp_methods,                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    FSPoll_tp_getsets,                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)FSPoll_tp_init,                                       /*tp_init*/
    0,                                                              /*tp_alloc*/
    FSPoll_tp_new,                                                  /*tp_new*/
};



