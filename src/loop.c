
static PyObject *default_loop = NULL;


static PyObject *
new_loop(PyTypeObject *type, PyObject *args, PyObject *kwargs, int is_default)
{
    PyObject *obj;
    Loop *loop;
    uv_loop_t *uv_loop;

    if ((args && PyTuple_GET_SIZE(args)) || (kwargs && PyDict_Check(kwargs) && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "Loop initialization takes no parameters");
        return NULL;
    }

    obj = PyType_GenericNew(type, args, kwargs);
    if (!obj) {
        return NULL;
    }

    loop = (Loop *) obj;
    if (is_default) {
        uv_loop = uv_default_loop();
    } else {
        uv_loop = &loop->loop_struct;
    }

    if (uv_loop_init(uv_loop) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Error initializing loop");
        Py_DECREF(obj);
        return NULL;
    }

    uv_loop->data = loop;
    loop->uv_loop = uv_loop;
    loop->is_default = is_default;
    loop->weakreflist = NULL;
    loop->buffer.in_use = False;

    return obj;
}


static PyObject *
Loop_func_run(Loop *self, PyObject *args)
{
    int mode, r;

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
pyuv__tp_work_cb(uv_work_t *req)
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
pyuv__tp_done_cb(uv_work_t *req, int status)
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
            errorno = PyInt_FromLong((long)status);
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
    int err;
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

    err = uv_queue_work(self->uv_loop, &work_req->req, pyuv__tp_work_cb, pyuv__tp_done_cb);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_Exception);
        goto error;
    }

    Py_INCREF(work_req);
    return (PyObject *)work_req;

error:
    Py_DECREF(work_req);
    return NULL;
}


static PyObject *
Loop_func_excepthook(Loop *self, PyObject *args)
{
    PyObject *exc, *value, *tb;

    if (!PyArg_ParseTuple(args, "OOO:excepthook", &exc, &value, &tb)) {
        return NULL;
    }
    Py_INCREF(exc);
    Py_INCREF(value);
    Py_INCREF(tb);
    PyErr_Restore(exc, value, tb);
    PySys_WriteStderr("Unhandled exception in callback\n");
    PyErr_PrintEx(0);
    PyErr_Clear();
    Py_RETURN_NONE;
}


static PyObject *
Loop_func_default_loop(PyObject *cls)
{
    PyTypeObject *type = (PyTypeObject *) cls;
    if (!default_loop) {
        default_loop = new_loop((PyTypeObject *)cls, NULL, NULL, True);
        if (!default_loop) {
            return NULL;
        }
        if (type->tp_init != PyBaseObject_Type.tp_init) {
            if (type->tp_init(default_loop, PyTuple_New(0), NULL) < 0) {
                Py_XDECREF(default_loop);
                default_loop = NULL;
                return NULL;
            }
        }
    }
    Py_INCREF(default_loop);
    return default_loop;
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


static void
handles_walk_cb(uv_handle_t* handle, void* arg)
{
    PyObject *handles, *obj;

    handles = arg;
    obj = handle->data;

    if (IS_PYUV_HANDLE(obj)) {
        if (!PyErr_Occurred()) {
            PyList_Append(handles, obj);
        }
    }
}

static PyObject *
Loop_handles_get(Loop *self, void *closure)
{
    PyObject *handles;
    UNUSED_ARG(closure);

    handles = PyList_New(0);

    if (!handles) {
        return NULL;
    }

    uv_walk(self->uv_loop, (uv_walk_cb)handles_walk_cb, handles);

    if (PyErr_Occurred()) {
        Py_DECREF(handles);
        handles = NULL;
    }

    return handles;
}


static PyObject *
Loop_alive_get(Loop *self, void *closure)
{
    UNUSED_ARG(closure);
    return PyBool_FromLong((long)uv_loop_alive(self->uv_loop));
}


static PyObject *
Loop_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return new_loop(type, args, kwargs, False);
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
        uv_loop_close(self->uv_loop);
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }
    Py_TYPE(self)->tp_clear((PyObject *)self);
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


static PyMethodDef
Loop_tp_methods[] = {
    { "run", (PyCFunction)Loop_func_run, METH_VARARGS, "Run the event loop." },
    { "stop", (PyCFunction)Loop_func_stop, METH_NOARGS, "Stop running the event loop." },
    { "now", (PyCFunction)Loop_func_now, METH_NOARGS, "Return event loop time, expressed in nanoseconds." },
    { "update_time", (PyCFunction)Loop_func_update_time, METH_NOARGS, "Update event loop's notion of time by querying the kernel." },
    { "fileno", (PyCFunction)Loop_func_fileno, METH_NOARGS, "Get the loop backend file descriptor." },
    { "get_timeout", (PyCFunction)Loop_func_get_timeout, METH_NOARGS, "Get the poll timeout, or -1 for no timeout." },
    { "default_loop", (PyCFunction)Loop_func_default_loop, METH_CLASS|METH_NOARGS, "Instantiate the default loop." },
    { "queue_work", (PyCFunction)Loop_func_queue_work, METH_VARARGS, "Queue the given function to be run in the thread pool." },
    { "excepthook", (PyCFunction)Loop_func_excepthook, METH_VARARGS, "Loop uncaught exception handler" },
    { NULL }
};


static PyGetSetDef Loop_tp_getsets[] = {
    {"__dict__", (getter)Loop_dict_get, (setter)Loop_dict_set, NULL},
    {"alive", (getter)Loop_alive_get, NULL, "Indicates if the loop is still running / alive", NULL},
    {"default", (getter)Loop_default_get, NULL, "Is this the default loop?", NULL},
    {"handles", (getter)Loop_handles_get, NULL, "Returns a list with all handles in the Loop", NULL},
    {NULL}
};


static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Loop",                                             /*tp_name*/
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

