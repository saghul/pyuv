
static PyObject* PyExc_ThreadPoolError;

typedef struct {
    Loop *loop;
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} tpool_req_data_t;


static void
work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;
    PyObject *args, *kw, *result;

    ASSERT(req);

    data = (tpool_req_data_t*)(req->data);
    args = (data->args)?data->args:PyTuple_New(0);
    kw = data->kwargs;

    result = PyObject_Call(data->func, args, kw);
    if (result == NULL) {
        PyErr_WriteUnraisable(data->func);
    }
    Py_XDECREF(result);

    PyGILState_Release(gstate);
}


static void
after_work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    tpool_req_data_t *data;

    ASSERT(req);

    data = (tpool_req_data_t*)req->data;

    Py_DECREF(data->loop);
    Py_DECREF(data->func);
    Py_XDECREF(data->args);
    Py_XDECREF(data->kwargs);

    PyMem_Free(req->data);
    PyMem_Free(req);
    PyGILState_Release(gstate);
}


static PyObject *
ThreadPool_func_run(PyObject *cls, PyObject *args)
{
    int r;
    uv_work_t *work_req = NULL;
    tpool_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *func, *func_args, *func_kwargs;

    func = func_args = func_kwargs = NULL;

    UNUSED_ARG(cls);

    if (!PyArg_ParseTuple(args, "O!O|OO:run", &LoopType, &loop, &func, &func_args, &func_kwargs)) {
        return NULL;
    }

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
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
   
    Py_INCREF(loop);
    Py_INCREF(func);
    Py_XINCREF(func_args);
    Py_XINCREF(func_kwargs);

    req_data->loop = loop;
    req_data->func = func;
    req_data->args = func_args;
    req_data->kwargs = func_kwargs;

    work_req->data = (void *)req_data;
    r = uv_queue_work(loop->uv_loop, work_req, work_cb, after_work_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_ThreadPoolError);
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
    Py_DECREF(loop);
    Py_DECREF(func);
    Py_XDECREF(func_args);
    Py_XDECREF(func_kwargs);
    return NULL;
}


static PyObject *
ThreadPool_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(type);
    UNUSED_ARG(args);
    UNUSED_ARG(kwargs);

    PyErr_SetString(PyExc_RuntimeError, "ThreadPool instances cannot be created");
    return NULL;
}


static PyMethodDef
ThreadPool_tp_methods[] = {
    { "run", (PyCFunction)ThreadPool_func_run, METH_CLASS|METH_VARARGS, "Run the given function in the thread pool." },
    { NULL }
};


static PyTypeObject ThreadPoolType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.ThreadPool",                                              /*tp_name*/
    sizeof(ThreadPool),                                             /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT,                                             /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    ThreadPool_tp_methods,                                          /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    0,                                                              /*tp_init*/
    0,                                                              /*tp_alloc*/
    ThreadPool_tp_new,                                              /*tp_new*/
};


