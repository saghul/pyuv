
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


static PyMethodDef
Request_tp_methods[] = {
    { "cancel", (PyCFunction)Request_func_cancel, METH_NOARGS, "Cancel the request." },
    { NULL }
};


static PyTypeObject RequestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Request",                                                 /*tp_name*/
    sizeof(Request),                                                /*tp_basicsize*/
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
    Request_tp_methods,                                             /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    0,                                                              /*tp_init*/
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
    Py_TPFLAGS_DEFAULT,                                             /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
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
    0,                                                              /*tp_init*/
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
    Py_TPFLAGS_DEFAULT,                                             /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
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
    0,                                                              /*tp_init*/
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
    Py_TPFLAGS_DEFAULT,                                             /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
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
    0,                                                              /*tp_init*/
    0,                                                              /*tp_alloc*/
    FSRequest_tp_new,                                               /*tp_new*/
};

