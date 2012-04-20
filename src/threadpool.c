
static PyObject* PyExc_ThreadPoolError;

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
    PyObject *result;
    int status;
    PyObject *after_work_func;
} tpool_req_data_t;


static void
work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;
    PyObject *args, *kw, *result;

    ASSERT(req);

    data = (tpool_req_data_t*)(req->data);
    args = (data->args) ? data->args : PyTuple_New(0);
    kw = data->kwargs;

    result = PyObject_Call(data->func, args, kw);
    if (result == NULL) {
        PyErr_WriteUnraisable(data->func);
        result = Py_None;
        Py_INCREF(Py_None);
        data->status = -1;
    } else {
        data->status = 0;
    }
    data->result = result;

    PyGILState_Release(gstate);
}


static void
after_work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;
    PyObject *result;

    ASSERT(req);

    data = (tpool_req_data_t*)req->data;

    if (data->after_work_func) {
        result = PyObject_CallFunctionObjArgs(data->after_work_func, PyInt_FromLong((long)data->status), data->result, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(data->after_work_func);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(data->func);
    Py_XDECREF(data->args);
    Py_XDECREF(data->kwargs);
    Py_XDECREF(data->after_work_func);
    Py_DECREF(data->result);

    PyMem_Free(req->data);
    PyMem_Free(req);
    PyGILState_Release(gstate);
}


static PyObject *
ThreadPool_func_queue_work(ThreadPool *self, PyObject *args, PyObject *kwargs)
{
    int r;
    uv_work_t *work_req = NULL;
    tpool_req_data_t *req_data = NULL;
    PyObject *func, *func_args, *func_kwargs, *after_work_func;

    static char *kwlist[] = {"func", "after_work_func", "func_args", "func_kwargs", NULL};

    work_req = NULL;
    req_data = NULL;
    func = func_args = func_kwargs = after_work_func = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOO:queue_work", kwlist, &func, &after_work_func, &func_args, &func_kwargs)) {
        return NULL;
    }

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (after_work_func != NULL && !PyCallable_Check(after_work_func)) {
        PyErr_SetString(PyExc_TypeError, "after_work_func must be a callable");
        return NULL;
    }

    if (func_args != NULL && !PyTuple_Check(func_args)) {
        PyErr_SetString(PyExc_TypeError, "args must be a tuple");
        return NULL;
    }

    if (func_kwargs != NULL && !PyDict_Check(func_kwargs)) {
        PyErr_SetString(PyExc_TypeError, "kwargs must be a dictionary");
        return NULL;
    }

    work_req = PyMem_Malloc(sizeof(uv_work_t));
    if (!work_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(tpool_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(func);
    Py_XINCREF(after_work_func);
    Py_XINCREF(func_args);
    Py_XINCREF(func_kwargs);

    req_data->func = func;
    req_data->after_work_func = after_work_func;
    req_data->args = func_args;
    req_data->kwargs = func_kwargs;
    req_data->result = NULL;

    work_req->data = (void *)req_data;
    r = uv_queue_work(UV_LOOP(self), work_req, work_cb, after_work_cb);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_ThreadPoolError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (work_req) {
        PyMem_Free(work_req);
    }
    if (req_data) {
        PyMem_Free(req_data);
    }
    Py_DECREF(func);
    Py_XDECREF(func_args);
    Py_XDECREF(func_kwargs);
    return NULL;
}


static PyObject *
ThreadPool_func_set_max_parallel_threads(PyObject *cls, PyObject *args)
{
    int nthreads;

    UNUSED_ARG(cls);

    if (!PyArg_ParseTuple(args, "i:set_max_parallel_threads", &nthreads)) {
        return NULL;
    }

    if (nthreads <= 0) {
        PyErr_SetString(PyExc_ValueError, "value must be higher than 0.");
        return NULL;
    }

#ifdef PYUV_WINDOWS
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
#else
    eio_set_max_parallel(nthreads);
    Py_RETURN_NONE;
#endif
}


static PyObject *
ThreadPool_func_set_min_parallel_threads(PyObject *cls, PyObject *args)
{
    int nthreads;

    UNUSED_ARG(cls);

    if (!PyArg_ParseTuple(args, "i:set_max_parallel_threads", &nthreads)) {
        return NULL;
    }

    if (nthreads <= 0) {
        PyErr_SetString(PyExc_ValueError, "value must be higher than 0.");
        return NULL;
    }

#ifdef PYUV_WINDOWS
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
#else
    eio_set_min_parallel(nthreads);
    Py_RETURN_NONE;
#endif
}


static PyObject *
ThreadPool_func_get_nthreads(PyObject *cls)
{
#ifdef PYUV_WINDOWS
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
#else
    return PyInt_FromLong((long)eio_nthreads());
#endif
}


static int
ThreadPool_tp_init(ThreadPool *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    return 0;
}


static PyObject *
ThreadPool_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    ThreadPool *self = (ThreadPool *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
ThreadPool_tp_traverse(ThreadPool *self, visitproc visit, void *arg)
{
    Py_VISIT(self->loop);
    return 0;
}


static int
ThreadPool_tp_clear(ThreadPool *self)
{
    Py_CLEAR(self->loop);
    return 0;
}


static void
ThreadPool_tp_dealloc(ThreadPool *self)
{
    ThreadPool_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
ThreadPool_tp_methods[] = {
    { "queue_work", (PyCFunction)ThreadPool_func_queue_work, METH_VARARGS|METH_KEYWORDS, "Queue the given function to be run in the thread pool." },
    { "set_max_parallel_threads", (PyCFunction)ThreadPool_func_set_max_parallel_threads, METH_CLASS|METH_VARARGS, "Set the maximum number of allowed threads in the pool." },
    { "set_min_parallel_threads", (PyCFunction)ThreadPool_func_set_min_parallel_threads, METH_CLASS|METH_VARARGS, "Set the minimum number of allowed threads in the pool." },
    { "get_nthreads", (PyCFunction)ThreadPool_func_get_nthreads, METH_CLASS|METH_NOARGS, "Return the number of woker threads currently running." },
    { NULL }
};


static PyMemberDef ThreadPool_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(ThreadPool, loop), READONLY, "Loop where this ThreadPool is running on."},
    {NULL}
};


static PyTypeObject ThreadPoolType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.ThreadPool",                                              /*tp_name*/
    sizeof(ThreadPool),                                             /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)ThreadPool_tp_dealloc,                              /*tp_dealloc*/
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
    (traverseproc)ThreadPool_tp_traverse,                           /*tp_traverse*/
    (inquiry)ThreadPool_tp_clear,                                   /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    ThreadPool_tp_methods,                                          /*tp_methods*/
    ThreadPool_tp_members,                                          /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)ThreadPool_tp_init,                                   /*tp_init*/
    0,                                                              /*tp_alloc*/
    ThreadPool_tp_new,                                              /*tp_new*/
};


