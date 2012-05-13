
static PyObject* PyExc_PollError;


static void
on_poll_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Poll *self;
    PyObject *result;

    ASSERT(handle);

    self = (Poll *)handle->data;
    ASSERT(self);

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_close_cb);
        }
        Py_XDECREF(result);
    }

    handle->data = NULL;
    PyMem_Free(handle);

    /* Refcount was increased in func_close */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_poll_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_poll_callback(uv_poll_t *handle, int status, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    uv_err_t err;
    Poll *self;
    PyObject *result, *py_events, *py_errorno;

    ASSERT(handle);

    self = (Poll *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status == 0) {
        py_events = PyInt_FromLong((long)events);
        py_errorno = Py_None;
        Py_INCREF(Py_None);
    } else  {
        py_events = Py_None;
        Py_INCREF(Py_None);
        err = uv_last_error(UV_LOOP(self));
        py_errorno = PyInt_FromLong((long)err.code);
    }

    result = PyObject_CallFunctionObjArgs(self->callback, self, py_events, py_errorno,NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(self->callback);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
Poll_func_start(Poll *self, PyObject *args)
{
    int r, events;
    PyObject *tmp, *callback;

    tmp = NULL;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_PollError, "Poll is closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "iO:start", &events, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    r = uv_poll_start(self->uv_handle, events, on_poll_callback);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PollError);
        return NULL;
    }

    tmp = self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
Poll_func_stop(Poll *self)
{
    int r;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_PollError, "Poll is already closed");
        return NULL;
    }

    r = uv_poll_stop(self->uv_handle);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PollError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Poll_func_close(Poll *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_PollError, "Poll is already closed");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:close", &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    Py_INCREF(callback);
    self->on_close_cb = callback;

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    uv_close((uv_handle_t *)self->uv_handle, on_poll_close);
    self->uv_handle = NULL;
    self->fd = -1;

    Py_RETURN_NONE;
}


static PyObject *
Poll_active_get(Poll *self, void *closure)
{
    UNUSED_ARG(closure);
    return PyBool_FromLong((long)uv_is_active((uv_handle_t *)self->uv_handle));
}


static PyObject *
Poll_fd_get(Poll *self, void *closure)
{
    UNUSED_ARG(closure);
    return PyInt_FromLong((long)self->fd);
}


static PyObject *
Poll_slow_get(Poll *self, void *closure)
{
    UNUSED_ARG(closure);
#ifdef PYUV_WINDOWS
    #define UV_HANDLE_POLL_SLOW  0x02000000 /* copied from src/win/internal.h*/
    if (!(self->uv_handle->flags & UV_HANDLE_POLL_SLOW)) {
        Py_RETURN_FALSE;
    } else {
        Py_RETURN_TRUE;
    }
    #undef UV_HANDLE_POLL_SLOW
#else
    Py_RETURN_FALSE;
#endif
}


static int
Poll_tp_init(Poll *self, PyObject *args, PyObject *kwargs)
{
    int r;
    long fdnum;
    uv_poll_t *uv_poll = NULL;
    Loop *loop;
    PyObject *fd, *tmp;

    tmp = NULL;

    UNUSED_ARG(kwargs);

    if (self->uv_handle) {
        PyErr_SetString(PyExc_PollError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!O:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

#ifdef PYUV_WINDOWS
    if (!PyObject_TypeCheck(fd, PySocketModule.Sock_Type)) {
        PyErr_SetString(PyExc_TypeError, "only socket objects are supported in this configuration");
        return -1;
    }
#endif

    fdnum = PyObject_AsFileDescriptor(fd);
    if (fdnum == -1) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    uv_poll = PyMem_Malloc(sizeof(uv_poll_t));
    if (!uv_poll) {
        PyErr_NoMemory();
        Py_DECREF(loop);
        return -1;
    }

    r = uv_poll_init_socket(UV_LOOP(self), uv_poll, (uv_os_sock_t)fdnum);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_PollError);
        Py_DECREF(loop);
        return -1;
    }
    uv_poll->data = (void *)self;
    self->uv_handle = uv_poll;
    self->fd = fdnum;

    return 0;
}


static PyObject *
Poll_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Poll *self = (Poll *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    self->weakreflist = NULL;
    return (PyObject *)self;
}


static int
Poll_tp_traverse(Poll *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
Poll_tp_clear(Poll *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Poll_tp_dealloc(Poll *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_poll_dealloc_close);
        self->uv_handle = NULL;
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }
    Poll_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Poll_tp_methods[] = {
    { "start", (PyCFunction)Poll_func_start, METH_VARARGS, "Start the Poll." },
    { "stop", (PyCFunction)Poll_func_stop, METH_NOARGS, "Stop the Poll." },
    { "close", (PyCFunction)Poll_func_close, METH_VARARGS, "Close the Poll." },
    { NULL }
};


static PyMemberDef Poll_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Poll, loop), READONLY, "Loop where this Poll is running on."},
    {"data", T_OBJECT, offsetof(Poll, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef Poll_tp_getsets[] = {
    {"active", (getter)Poll_active_get, 0, "Indicates if handle is active", NULL},
    {"fd", (getter)Poll_fd_get, 0, "File descriptor being monitored", NULL},
    {"slow", (getter)Poll_slow_get, 0, "Indicates if the handle is running in slow mode (Windows)", NULL},
    {NULL}
};


static PyTypeObject PollType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Poll",                                                    /*tp_name*/
    sizeof(Poll),                                                   /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Poll_tp_dealloc,                                    /*tp_dealloc*/
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
    (traverseproc)Poll_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Poll_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    offsetof(Poll, weakreflist),                                    /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Poll_tp_methods,                                                /*tp_methods*/
    Poll_tp_members,                                                /*tp_members*/
    Poll_tp_getsets,                                                /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Poll_tp_init,                                         /*tp_init*/
    0,                                                              /*tp_alloc*/
    Poll_tp_new,                                                    /*tp_new*/
};


