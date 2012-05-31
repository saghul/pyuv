
static PyObject* PyExc_ThreadPoolError;

typedef struct {
    PyObject *work_cb;
    PyObject *after_work_cb;
    PyObject *result;
    PyObject *error;
} tpool_req_data_t;


static void
threadpool_work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;
    PyObject *result, *error, *err_type, *err_value, *err_tb;

    err_type = err_value = err_tb = NULL;

    ASSERT(req);

    data = (tpool_req_data_t*)(req->data);

    result = PyObject_CallFunctionObjArgs(data->work_cb, NULL);
    if (result == NULL) {
        PyErr_Fetch(&err_type, &err_value, &err_tb);
        PyErr_NormalizeException(&err_type, &err_value, &err_tb);
        error = PyTuple_New(3);
        if (!error) {
            PyErr_WriteUnraisable(data->work_cb);
            error = Py_None;
            Py_INCREF(Py_None);
        } else {
            if (!err_type) {
                err_type = Py_None;
                Py_INCREF(Py_None);
            }
            if (!err_value) {
                err_value = Py_None;
                Py_INCREF(Py_None);
            }
            if (!err_tb) {
                err_tb = Py_None;
                Py_INCREF(Py_None);
            }
            PyTuple_SET_ITEM(error, 0, err_type);
            PyTuple_SET_ITEM(error, 1, err_value);
            PyTuple_SET_ITEM(error, 2, err_tb);
        }
        result = Py_None;
        Py_INCREF(Py_None);
    } else {
        error = Py_None;
        Py_INCREF(Py_None);
    }
    data->result = result;
    data->error = error;

    PyGILState_Release(gstate);
}


static void
threadpool_after_work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;
    PyObject *result;

    ASSERT(req);

    data = (tpool_req_data_t*)req->data;

    if (data->after_work_cb) {
        result = PyObject_CallFunctionObjArgs(data->after_work_cb, data->result, data->error, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(data->after_work_cb);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(data->work_cb);
    Py_XDECREF(data->after_work_cb);
    Py_DECREF(data->result);
    Py_DECREF(data->error);

    PyMem_Free(req->data);
    PyMem_Free(req);
    PyGILState_Release(gstate);
}


static PyObject *
ThreadPool_func_queue_work(ThreadPool *self, PyObject *args)
{
    int r;
    uv_work_t *work_req = NULL;
    tpool_req_data_t *req_data = NULL;
    PyObject *work_cb, *after_work_cb;

    work_req = NULL;
    req_data = NULL;
    work_cb = after_work_cb = NULL;

    if (!PyArg_ParseTuple(args, "O|O:queue_work", &work_cb, &after_work_cb)) {
        return NULL;
    }

    if (!PyCallable_Check(work_cb)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (after_work_cb != NULL && !PyCallable_Check(after_work_cb)) {
        PyErr_SetString(PyExc_TypeError, "after_work_cb must be a callable");
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

    Py_INCREF(work_cb);
    Py_XINCREF(after_work_cb);

    req_data->work_cb = work_cb;
    req_data->after_work_cb = after_work_cb;
    req_data->result = NULL;
    req_data->error = NULL;

    work_req->data = (void *)req_data;
    r = uv_queue_work(UV_LOOP(self), work_req, threadpool_work_cb, threadpool_after_work_cb);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_LOOP(self), PyExc_ThreadPoolError);
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
    Py_DECREF(work_cb);
    Py_XDECREF(after_work_cb);
    return NULL;
}


static PyObject *
ThreadPool_func_set_parallel_threads(PyObject *cls, PyObject *args)
{
    int nthreads;

    UNUSED_ARG(cls);

    if (!PyArg_ParseTuple(args, "i:set_parallel_threads", &nthreads)) {
        return NULL;
    }

    if (nthreads <= 0) {
        PyErr_SetString(PyExc_ValueError, "value must be higher than 0.");
        return NULL;
    }

#ifndef PYUV_WINDOWS
    eio_set_min_parallel(nthreads);
    eio_set_max_parallel(nthreads);
#endif

    Py_RETURN_NONE;
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
    { "queue_work", (PyCFunction)ThreadPool_func_queue_work, METH_VARARGS, "Queue the given function to be run in the thread pool." },
    { "set_parallel_threads", (PyCFunction)ThreadPool_func_set_parallel_threads, METH_CLASS|METH_VARARGS, "Set the maximum number of allowed threads in the pool." },
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


