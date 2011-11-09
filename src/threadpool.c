
static PyObject* PyExc_ThreadPoolError;

static int threadpool_created = 0;

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} tpool_req_data_t;


static void
work_cb(uv_work_t *req)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    tpool_req_data_t *data = (tpool_req_data_t*)(req->data);

    PyObject *args = (data->args)?data->args:PyTuple_New(0);
    PyObject *kw = data->kwargs;

    PyObject *result;
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
    ASSERT(req);

    tpool_req_data_t *data = (tpool_req_data_t*)req->data;
    Py_DECREF(data->func);
    Py_XDECREF(data->args);
    Py_XDECREF(data->kwargs);

    PyMem_Free(req->data);
    req->data = NULL;
    PyMem_Free(req);
    PyGILState_Release(gstate);
}


static PyObject *
ThreadPool_func_run(ThreadPool *self, PyObject *args)
{
    int r = 0;
    uv_work_t *work_req = NULL;
    tpool_req_data_t *req_data = NULL;
    PyObject *func;
    PyObject *func_args = NULL;
    PyObject *func_kwargs = NULL;

    if (!PyArg_ParseTuple(args, "O|OO:run", &func, &func_args, &func_kwargs)) {
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
   
    Py_INCREF(func);
    Py_XINCREF(func_args);
    Py_XINCREF(func_kwargs);

    req_data->func = func;
    req_data->args = func_args;
    req_data->kwargs = func_kwargs;

    work_req->data = (void *)req_data;
    r = uv_queue_work(UV_LOOP(self), work_req, work_cb, after_work_cb);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_ThreadPoolError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (work_req) {
        work_req->data = NULL;
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


static int
ThreadPool_tp_init(ThreadPool *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;

    if (self->initialized) {
        PyErr_SetString(PyExc_ThreadPoolError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    self->initialized = True;

    return 0;
}


static PyObject *
ThreadPool_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (threadpool_created) {
        PyErr_SetString(PyExc_RuntimeError, "Only one ThreadPool may be instantiated due to a limitation in libuv");
        return NULL;
    }
    threadpool_created = 1;

    ThreadPool *self = (ThreadPool *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
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
    { "run", (PyCFunction)ThreadPool_func_run, METH_VARARGS, "Run the given function in the thread pool." },
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,      /*tp_flags*/
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


