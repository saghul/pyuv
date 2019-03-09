
static void
pyuv__fsevent_cb(uv_fs_event_t *handle, const char *filename, int events, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    FSEvent *self;
    PyObject *result, *py_filename, *py_events, *errorno;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, FSEvent, fsevent_h);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (filename) {
        py_filename = Py_BuildValue("s", filename);
    } else {
        py_filename = Py_None;
        Py_INCREF(Py_None);
    }

    if (status < 0) {
        errorno = PyLong_FromLong((long)status);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    py_events = PyLong_FromLong((long)events);

    result = PyObject_CallFunctionObjArgs(self->callback, self, py_filename, py_events, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }
    Py_XDECREF(result);
    Py_DECREF(py_events);
    Py_DECREF(py_filename);
    Py_DECREF(errorno);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
FSEvent_func_start(FSEvent *self, PyObject *args, PyObject *kwargs)
{
    int err, flags;
    char *path;
    PyObject *tmp, *callback;

    static char *kwlist[] = {"path", "flags", "callback", NULL};

    tmp = NULL;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "siO:start", kwlist, &path, &flags, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    err = uv_fs_event_start(&self->fsevent_h, pyuv__fsevent_cb, path, flags);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSEventError);
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
FSEvent_func_stop(FSEvent *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_fs_event_stop(&self->fsevent_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSEventError);
        return NULL;
    }

    Py_XDECREF(self->callback);
    self->callback = NULL;

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static PyObject *
FSEvent_path_get(FSEvent *self, void *closure)
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
    err = uv_fs_event_getpath(&self->fsevent_h, buf, &buf_len);
    if (err < 0) {
        return Py_BuildValue("s", "");
    }

    return PyUnicode_DecodeFSDefaultAndSize(buf, buf_len);
}


static int
FSEvent_tp_init(FSEvent *self, PyObject *args, PyObject *kwargs)
{
    int err;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    err = uv_fs_event_init(loop->uv_loop, &self->fsevent_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSEventError);
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
FSEvent_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FSEvent *self;

    self = (FSEvent *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->fsevent_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->fsevent_h;

    return (PyObject *)self;
}


static int
FSEvent_tp_traverse(FSEvent *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
FSEvent_tp_clear(FSEvent *self)
{
    Py_CLEAR(self->callback);
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
FSEvent_tp_methods[] = {
    { "start", (PyCFunction)FSEvent_func_start, METH_VARARGS | METH_KEYWORDS, "Start the FSEvent handle." },
    { "stop", (PyCFunction)FSEvent_func_stop, METH_NOARGS, "Stop the FSEvent handle." },
    { NULL }
};


static PyGetSetDef FSEvent_tp_getsets[] = {
    {"path", (getter)FSEvent_path_get, NULL, "Path being monitored.", NULL},
    {NULL}
};


static PyTypeObject FSEventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.fs.FSEvent",                                       /*tp_name*/
    sizeof(FSEvent),                                                /*tp_basicsize*/
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
    (traverseproc)FSEvent_tp_traverse,                              /*tp_traverse*/
    (inquiry)FSEvent_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    FSEvent_tp_methods,                                             /*tp_methods*/
    0,                                                              /*tp_members*/
    FSEvent_tp_getsets,                                             /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)FSEvent_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    FSEvent_tp_new,                                                 /*tp_new*/
};



