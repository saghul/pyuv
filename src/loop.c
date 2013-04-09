
static Loop *default_loop = NULL;


static void
_loop_cleanup(void)
{
    Py_XDECREF(default_loop);
}


static int
init_loop(Loop *loop, int is_default)
{
    uv_loop_t *uv_loop;

    uv_loop = NULL;

    if (is_default) {
        uv_loop = uv_default_loop();
    } else {
        uv_loop = uv_loop_new();
    }

    if (!uv_loop) {
        PyErr_NoMemory();
        return -1;
    }
    uv_loop->data = loop;

    loop->uv_loop = uv_loop;
    loop->is_default = is_default;
    loop->weakreflist = NULL;
    loop->excepthook_cb = NULL;

    return 0;
}


static PyObject *
new_loop(PyTypeObject *type, PyObject *args, PyObject *kwargs, int is_default)
{
    if ((args && PyTuple_GET_SIZE(args)) || (kwargs && PyDict_Check(kwargs) && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Loop initialization takes no parameters");
        return NULL;
    }

    if (is_default) {
        if (!default_loop) {
            default_loop = (Loop *)PyType_GenericNew(type, args, kwargs);
            if (!default_loop) {
                return NULL;
            }
            if (init_loop(default_loop, True) != 0) {
                return NULL;
            }
            Py_AtExit(_loop_cleanup);
        }
        Py_INCREF(default_loop);
        return (PyObject *)default_loop;
    } else {
        Loop *self = (Loop *)PyType_GenericNew(type, args, kwargs);
        if (!self) {
            return NULL;
        }
        if (init_loop(self, False) != 0) {
            return NULL;
        }
        return (PyObject *)self;
    }
}


static PyObject *
Loop_func_run(Loop *self, PyObject *args)
{
    int r;
    int mode;

    mode = UV_RUN_DEFAULT;

    if (!PyArg_ParseTuple(args, "|i:run", &mode)) {
        return NULL;
    }

    if (mode != UV_RUN_DEFAULT && mode != UV_RUN_ONCE && mode != UV_RUN_NOWAIT) {
        PyErr_SetString(PyExc_ValueError, "invalid mode specified");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    r = uv_run(self->uv_loop, mode);
    Py_END_ALLOW_THREADS

    if (PyErr_Occurred()) {
        handle_uncaught_exception(self);
    }

    return PyBool_FromLong((long)r);
}


static PyObject *
Loop_func_stop(Loop *self)
{
    uv_stop(self->uv_loop);
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_now(Loop *self)
{
    return PyLong_FromUnsignedLongLong(uv_now(self->uv_loop));
}


static PyObject *
Loop_func_update_time(Loop *self)
{
    uv_update_time(self->uv_loop);
    Py_RETURN_NONE;
}


static void
walk_cb(uv_handle_t* handle, void* arg)
{
    PyObject *result;
    PyObject *callback = (PyObject *)arg;
    PyObject *obj = (PyObject *)handle->data;

    ASSERT(obj);

    Py_INCREF(obj);
    result = PyObject_CallFunctionObjArgs(callback, obj, NULL);
    if (result == NULL) {
        handle_uncaught_exception(((Handle *)obj)->loop);
    }
    Py_DECREF(obj);
    Py_XDECREF(result);
}

static PyObject *
Loop_func_walk(Loop *self, PyObject *args)
{
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "O:walk", &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    Py_INCREF(callback);
    uv_walk(self->uv_loop, (uv_walk_cb)walk_cb, callback);
    Py_DECREF(callback);

    Py_RETURN_NONE;
}


static PyObject *
Loop_func_fileno(Loop *self)
{
    return PyInt_FromLong((long)uv_backend_fd(self->uv_loop));
}


static PyObject *
Loop_func_get_timeout(Loop *self)
{
    return PyFloat_FromDouble(uv_backend_timeout(self->uv_loop)/1000.0);
}


static void
threadpool_work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    WorkRequest *work_req;
    PyObject *result;

    ASSERT(req);
    work_req = PYUV_CONTAINER_OF(req, WorkRequest, req);

    result = PyObject_CallFunctionObjArgs(work_req->work_cb, NULL);
    if (result == NULL) {
        ASSERT(PyErr_Occurred());
        PyErr_Print();
    }
    Py_XDECREF(result);

    PyGILState_Release(gstate);
}

static void
threadpool_done_cb(uv_work_t *req, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    WorkRequest *work_req;
    Loop *loop;
    PyObject *result, *errorno;

    ASSERT(req);

    work_req = PYUV_CONTAINER_OF(req, WorkRequest, req);
    loop = REQUEST(work_req)->loop;

    if (work_req->done_cb != Py_None) {
        if (status < 0) {
            uv_err_t err = uv_last_error(req->loop);
            errorno = PyInt_FromLong((long)err.code);
        } else {
            errorno = Py_None;
            Py_INCREF(Py_None);
        }

        result = PyObject_CallFunctionObjArgs(work_req->done_cb, errorno, NULL);
        if (result == NULL) {
            handle_uncaught_exception(loop);
        }
        Py_XDECREF(result);
        Py_DECREF(errorno);
    }

    UV_REQUEST(work_req) = NULL;
    Py_DECREF(work_req);

    PyGILState_Release(gstate);
}

static PyObject *
Loop_func_queue_work(Loop *self, PyObject *args)
{
    int r;
    WorkRequest *work_req;
    PyObject *work_cb, *done_cb;

    done_cb = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:queue_work", &work_cb, &done_cb)) {
        return NULL;
    }

    if (!PyCallable_Check(work_cb)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (done_cb != Py_None && !PyCallable_Check(done_cb)) {
        PyErr_SetString(PyExc_TypeError, "done_cb must be a callable or None");
        return NULL;
    }

    work_req = (WorkRequest *)PyObject_CallFunctionObjArgs((PyObject *)&WorkRequestType, self, work_cb, done_cb, NULL);
    if (!work_req) {
        PyErr_NoMemory();
        return NULL;
    }

    r = uv_queue_work(self->uv_loop, &work_req->req, threadpool_work_cb, threadpool_done_cb);
    if (r != 0) {
        RAISE_UV_EXCEPTION(self->uv_loop, PyExc_Exception);
        goto error;
    }

    Py_INCREF(work_req);
    return (PyObject *)work_req;

error:
    Py_DECREF(work_req);
    return NULL;
}


static PyObject *
Loop_func_default_loop(PyObject *cls)
{
    UNUSED_ARG(cls);
    return new_loop(&LoopType, NULL, NULL, 1);
}


static PyObject *
Loop_default_get(Loop *self, void *closure)
{
    UNUSED_ARG(closure);
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
    Py_VISIT(self->dict);
    return 0;
}


static int
Loop_tp_clear(Loop *self)
{
    Py_CLEAR(self->dict);
    return 0;
}


static void
Loop_tp_dealloc(Loop *self)
{
    if (self->uv_loop) {
        self->uv_loop->data = NULL;
        uv_loop_delete(self->uv_loop);
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }
    Loop_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject*
Loop_dict_get(Loop *self, void* c)
{
    UNUSED_ARG(c);

    if (self->dict == NULL) {
        self->dict = PyDict_New();
        if (self->dict == NULL) {
            return NULL;
        }
    }
    Py_INCREF(self->dict);
    return self->dict;
}


static int
Loop_dict_set(Loop *self, PyObject* val, void* c)
{
    PyObject* tmp;

    UNUSED_ARG(c);

    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "__dict__ may not be deleted");
        return -1;
    }
    if (!PyDict_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "__dict__ must be a dictionary");
        return -1;
    }
    tmp = self->dict;
    Py_INCREF(val);
    self->dict = val;
    Py_XDECREF(tmp);
    return 0;
}


static PyObject*
Loop_excepthook_get(Loop *self, void* c)
{
    UNUSED_ARG(c);
    return self->excepthook_cb;
}


static int
Loop_excepthook_set(Loop *self, PyObject* val, void* c)
{
    PyObject* tmp;

    UNUSED_ARG(c);

    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "excepthook may not be deleted");
        return -1;
    }
    if (val != Py_None && !PyCallable_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return -1;
    }
    tmp = self->excepthook_cb;
    Py_INCREF(val);
    self->excepthook_cb = val;
    Py_XDECREF(tmp);
    return 0;
}


static PyMethodDef
Loop_tp_methods[] = {
    { "run", (PyCFunction)Loop_func_run, METH_VARARGS, "Run the event loop." },
    { "stop", (PyCFunction)Loop_func_stop, METH_NOARGS, "Stop running the event loop." },
    { "now", (PyCFunction)Loop_func_now, METH_NOARGS, "Return event loop time, expressed in nanoseconds." },
    { "update_time", (PyCFunction)Loop_func_update_time, METH_NOARGS, "Update event loop's notion of time by querying the kernel." },
    { "walk", (PyCFunction)Loop_func_walk, METH_VARARGS, "Walk all handles in the loop." },
    { "fileno", (PyCFunction)Loop_func_fileno, METH_NOARGS, "Get the loop backend file descriptor." },
    { "get_timeout", (PyCFunction)Loop_func_get_timeout, METH_NOARGS, "Get the poll timeout, or -1 for no timeout." },
    { "default_loop", (PyCFunction)Loop_func_default_loop, METH_CLASS|METH_NOARGS, "Instantiate the default loop." },
    { "queue_work", (PyCFunction)Loop_func_queue_work, METH_VARARGS, "Queue the given function to be run in the thread pool." },
    { NULL }
};


static PyGetSetDef Loop_tp_getsets[] = {
    {"__dict__", (getter)Loop_dict_get, (setter)Loop_dict_set, NULL},
    {"default", (getter)Loop_default_get, NULL, "Is this the default loop?", NULL},
    {"excepthook", (getter)Loop_excepthook_get, (setter)Loop_excepthook_set, "Loop uncaught exception handler", NULL},
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Loop_tp_traverse,                                 /*tp_traverse*/
    (inquiry)Loop_tp_clear,                                         /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    offsetof(Loop, weakreflist),                                    /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Loop_tp_methods,                                                /*tp_methods*/
    0,                                                              /*tp_members*/
    Loop_tp_getsets,                                                /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    offsetof(Loop, dict),                                           /*tp_dictoffset*/
    0,                                                              /*tp_init*/
    0,                                                              /*tp_alloc*/
    Loop_tp_new,                                                    /*tp_new*/
};


