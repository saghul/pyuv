
static PyObject *
Request_func_cancel(Request *self)
{
    if (self->req_ptr && uv_cancel(self->req_ptr) == 0) {
         Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


static PyObject *
Request_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Request *self = (Request *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->req_ptr = NULL;
    return (PyObject *)self;
}


static int
Request_tp_init(Request *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp;

    UNUSED_ARG(kwargs);

    if (self->initialized) {
        PyErr_SetString(PyExc_RuntimeError, "Object was already initialized");
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


static PyMemberDef Request_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Request, loop), READONLY, "Loop where this request belongs."},
    {NULL}
};


static PyMethodDef
Request_tp_methods[] = {
    { "cancel", (PyCFunction)Request_func_cancel, METH_NOARGS, "Cancel the request." },
    { NULL }
};


static void
Request_tp_dealloc(Request *self)
{
    Py_TYPE(self)->tp_clear((PyObject *)self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static int
Request_tp_traverse(Request *self, visitproc visit, void *arg)
{
    Py_VISIT(self->loop);
    return 0;
}


static int
Request_tp_clear(Request *self)
{
    Py_CLEAR(self->loop);
    return 0;
}


static PyTypeObject RequestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Request",                                                 /*tp_name*/
    sizeof(Request),                                                /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Request_tp_dealloc,                                 /*tp_dealloc*/
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
    (traverseproc)Request_tp_traverse,                              /*tp_traverse*/
    (inquiry)Request_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Request_tp_methods,                                             /*tp_methods*/
    Request_tp_members,                                             /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Request_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    Request_tp_new,                                                 /*tp_new*/
};


static PyObject *
GAIRequest_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    GAIRequest *self = (GAIRequest *)RequestType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    UV_REQUEST(self) = (uv_req_t *)&self->req;
    return (PyObject *)self;
}


static int
GAIRequest_tp_init(GAIRequest *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;
    PyObject *callback, *tmp, *loopargs;

    UNUSED_ARG(kwargs);

    if (!PyArg_ParseTuple(args, "O!O:__init__", &LoopType, &loop, &callback)) {
        return -1;
    }

    loopargs = PySequence_GetSlice(args, 0, 1);
    if (!loopargs) {
        return -1;
    }

    r = RequestType.tp_init((PyObject *)self, loopargs, kwargs);
    if (r < 0) {
        Py_DECREF(loopargs);
        return r;
    }

    tmp = (PyObject *)self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_DECREF(loopargs);

    return 0;
}


static int
GAIRequest_tp_traverse(GAIRequest *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return RequestType.tp_traverse((PyObject *)self, visit, arg);
}


static int
GAIRequest_tp_clear(GAIRequest *self)
{
    Py_CLEAR(self->callback);
    return RequestType.tp_clear((PyObject *)self);
}


static PyTypeObject GAIRequestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.GAIRequest",                                              /*tp_name*/
    sizeof(GAIRequest),                                             /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)GAIRequest_tp_traverse,                           /*tp_traverse*/
    (inquiry)GAIRequest_tp_clear,                                   /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    0,                                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)GAIRequest_tp_init,                                   /*tp_init*/
    0,                                                              /*tp_alloc*/
    GAIRequest_tp_new,                                              /*tp_new*/
};


static PyObject *
WorkRequest_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    WorkRequest *self = (WorkRequest *)RequestType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    UV_REQUEST(self) = (uv_req_t *)&self->req;
    return (PyObject *)self;
}


static int
WorkRequest_tp_init(WorkRequest *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;
    PyObject *work_cb, *done_cb, *tmp, *loopargs;

    UNUSED_ARG(kwargs);

    if (!PyArg_ParseTuple(args, "O!OO:__init__", &LoopType, &loop, &work_cb, &done_cb)) {
        return -1;
    }

    loopargs = PySequence_GetSlice(args, 0, 1);
    if (!loopargs) {
        return -1;
    }

    r = RequestType.tp_init((PyObject *)self, loopargs, kwargs);
    if (r < 0) {
        Py_DECREF(loopargs);
        return r;
    }

    tmp = (PyObject *)self->work_cb;
    Py_INCREF(work_cb);
    self->work_cb = work_cb;
    Py_XDECREF(tmp);

    tmp = (PyObject *)self->done_cb;
    Py_INCREF(done_cb);
    self->done_cb = done_cb;
    Py_XDECREF(tmp);

    Py_DECREF(loopargs);

    return 0;
}


static int
WorkRequest_tp_traverse(WorkRequest *self, visitproc visit, void *arg)
{
    Py_VISIT(self->work_cb);
    Py_VISIT(self->done_cb);
    return RequestType.tp_traverse((PyObject *)self, visit, arg);
}


static int
WorkRequest_tp_clear(WorkRequest *self)
{
    Py_CLEAR(self->work_cb);
    Py_CLEAR(self->done_cb);
    return RequestType.tp_clear((PyObject *)self);
}


static PyTypeObject WorkRequestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.WorkRequest",                                             /*tp_name*/
    sizeof(WorkRequest),                                            /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)WorkRequest_tp_traverse,                          /*tp_traverse*/
    (inquiry)WorkRequest_tp_clear,                                  /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    0,                                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)WorkRequest_tp_init,                                  /*tp_init*/
    0,                                                              /*tp_alloc*/
    WorkRequest_tp_new,                                             /*tp_new*/
};


static PyObject *
FSRequest_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FSRequest *self = (FSRequest *)RequestType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    UV_REQUEST(self) = (uv_req_t *)&self->req;
    return (PyObject *)self;
}


static int
FSRequest_tp_init(FSRequest *self, PyObject *args, PyObject *kwargs)
{
    int r;
    Loop *loop;
    PyObject *callback, *tmp, *loopargs;

    UNUSED_ARG(kwargs);

    if (!PyArg_ParseTuple(args, "O!O:__init__", &LoopType, &loop, &callback)) {
        return -1;
    }

    loopargs = PySequence_GetSlice(args, 0, 1);
    if (!loopargs) {
        return -1;
    }

    r = RequestType.tp_init((PyObject *)self, loopargs, kwargs);
    if (r < 0) {
        Py_DECREF(loopargs);
        return r;
    }

    tmp = (PyObject *)self->callback;
    Py_INCREF(callback);
    self->callback = callback;
    Py_XDECREF(tmp);

    Py_DECREF(loopargs);

    return 0;
}


static int
FSRequest_tp_traverse(FSRequest *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return RequestType.tp_traverse((PyObject *)self, visit, arg);
}


static int
FSRequest_tp_clear(FSRequest *self)
{
    Py_CLEAR(self->callback);
    return RequestType.tp_clear((PyObject *)self);
}


static PyTypeObject FSRequestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.FSRequest",                                               /*tp_name*/
    sizeof(FSRequest),                                              /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)FSRequest_tp_traverse,                            /*tp_traverse*/
    (inquiry)FSRequest_tp_clear,                                    /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    0,                                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)FSRequest_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    FSRequest_tp_new,                                               /*tp_new*/
};

