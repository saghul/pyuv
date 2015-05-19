
/* Taken from http://bugs.python.org/issue8212
 * A function to do the necessary adjustments if we find that code
 * run during a tp_dealloc or tp_free has resurrected an
 * object.  It adjusts the total reference count and adds a new
 * reference to the type.
 */
static void
resurrect_object(PyObject *self)
{
    /* The object lives again. We must now undo the _Py_ForgetReference
     * done in _Py_Dealloc in object.c.
     */
    Py_ssize_t refcnt = Py_REFCNT(self);
    ASSERT(Py_REFCNT(self) != 0);
    _Py_NewReference(self);
    Py_REFCNT(self) = refcnt;
    /* If Py_REF_DEBUG, _Py_NewReference bumped _Py_RefTotal, so
     * we need to undo that. */
    _Py_DEC_REFTOTAL;
    /* If Py_TRACE_REFS, _Py_NewReference re-added self to the object
     * chain, so no more to do there.
     * If COUNT_ALLOCS, the original decref bumped tp_frees, and
     * _Py_NewReference bumped tp_allocs:  both of those need to be
     * undone.
     */
#ifdef COUNT_ALLOCS
    --Py_TYPE(self)->tp_frees;
    --Py_TYPE(self)->tp_allocs;
#endif

     /* When called from a heap type's dealloc (subtype_dealloc avove), the type will be
      * decref'ed on return.  This counteracts that.  There is no way to otherwise
      * let subtype_dealloc know that calling a parent class' tp_dealloc slot caused
      * the instance to be resurrected.
      */
    if (PyType_HasFeature(Py_TYPE(self), Py_TPFLAGS_HEAPTYPE))
        Py_INCREF(Py_TYPE(self));
    return;
}


static void
pyuv__handle_close_cb(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Handle *self;
    PyObject *result;
    ASSERT(handle);

    /* Can't use container_of here */
    self = (Handle *)handle->data;

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            handle_uncaught_exception(self->loop);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self->on_close_cb);
    self->on_close_cb = NULL;

    Py_DECREF(self->loop);
    self->loop = (Loop *)Py_None;
    Py_INCREF(Py_None);

    /* Remove extra reference, in case it was added along the way */
    PYUV_HANDLE_DECREF(self);

    /* Refcount was increased in the caller function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
pyuv__handle_dealloc_close_cb(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Handle *self;

    ASSERT(handle);

    /* Can't use container_of here */
    self = (Handle *)handle->data;
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
initialize_handle(Handle *self, Loop *loop)
{
    PyObject *tmp;
    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);
    self->flags = 0;
    self->initialized = True;
}


static PyObject *
Handle_func_close(Handle *self, PyObject *args)
{
    PyObject *callback = Py_None;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

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

    uv_close(self->uv_handle, pyuv__handle_close_cb);

    Py_RETURN_NONE;
}


static PyObject *
Handle_active_get(Handle *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyBool_FromLong((long)uv_is_active(self->uv_handle));
}


static PyObject *
Handle_closed_get(Handle *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyBool_FromLong((long)uv_is_closing(self->uv_handle));
}


static PyObject *
Handle_ref_get(Handle *self, void *closure)
{
    UNUSED_ARG(closure);

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);

    return PyBool_FromLong((long)uv_has_ref(self->uv_handle));
}


static int
Handle_ref_set(Handle *self, PyObject* val, void* c)
{
    long ref;

    UNUSED_ARG(c);

    ref = PyLong_AsLong(val);
    if (ref == -1 && PyErr_Occurred()) {
        return -1;
    }

    if (ref) {
        uv_ref(self->uv_handle);
    } else {
        uv_unref(self->uv_handle);
    }

    return 0;
}


static PyObject *
Handle_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Handle *self = (Handle *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->handle_magic = PYUV_HANDLE_MAGIC;
    self->initialized = False;
    self->uv_handle = NULL;
    self->weakreflist = NULL;
    return (PyObject *)self;
}


static int
Handle_tp_traverse(Handle *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    Py_VISIT(self->dict);
    return 0;
}


static int
Handle_tp_clear(Handle *self)
{
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    Py_CLEAR(self->dict);
    return 0;
}


static void
Handle_tp_dealloc(Handle *self)
{
    ASSERT(self->uv_handle);
    if (self->initialized && !uv_is_closing(self->uv_handle)) {
        uv_close(self->uv_handle, pyuv__handle_dealloc_close_cb);
        ASSERT(uv_is_closing(self->uv_handle));
        /* resurrect the Python object until the close callback is called */
        Py_INCREF(self);
        resurrect_object((PyObject *)self);
        return;
    } else {
        /* There are a few cases why the code will take this path:
         *   - A subclass of a handle didn't call it's parent's __init__
         *   - Aclosed handle is deallocated. Refcount is increased in close(),
         *     so it's guaranteed that if we arrived here and the user had called close(),
         *     the callback was already executed.
         *  - A handle goes out of scope and it's closed here in tp_dealloc and resurrected.
         *    Once it's deallocated again it will take this path because the handle is now
         *    closed.
         */
        ;
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
    }
    Py_TYPE(self)->tp_clear((PyObject *)self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject*
Handle_dict_get(Handle *self, void* c)
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
Handle_dict_set(Handle *self, PyObject* val, void* c)
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
Handle_tp_methods[] = {
    { "close", (PyCFunction)Handle_func_close, METH_VARARGS, "Close handle." },
    { NULL }
};


static PyMemberDef Handle_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Handle, loop), READONLY, "Loop where this handle belongs."},
    {NULL}
};


static PyGetSetDef Handle_tp_getsets[] = {
    {"__dict__", (getter)Handle_dict_get, (setter)Handle_dict_set, NULL},
    {"active", (getter)Handle_active_get, NULL, "Indicates if this handle is active.", NULL},
    {"ref", (getter)Handle_ref_get, (setter)Handle_ref_set, "Indicates if this handle is ref'd or not.", NULL},
    {"closed", (getter)Handle_closed_get, NULL, "Indicates if this handle is closing or already closed.", NULL},
    {NULL}
};


static PyTypeObject HandleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.Handle",                                          /*tp_name*/
    sizeof(Handle),                                                /*tp_basicsize*/
    0,                                                             /*tp_itemsize*/
    (destructor)Handle_tp_dealloc,                                 /*tp_dealloc*/
    0,                                                             /*tp_print*/
    0,                                                             /*tp_getattr*/
    0,                                                             /*tp_setattr*/
    0,                                                             /*tp_compare*/
    0,                                                             /*tp_repr*/
    0,                                                             /*tp_as_number*/
    0,                                                             /*tp_as_sequence*/
    0,                                                             /*tp_as_mapping*/
    0,                                                             /*tp_hash */
    0,                                                             /*tp_call*/
    0,                                                             /*tp_str*/
    0,                                                             /*tp_getattro*/
    0,                                                             /*tp_setattro*/
    0,                                                             /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                       /*tp_flags*/
    0,                                                             /*tp_doc*/
    (traverseproc)Handle_tp_traverse,                              /*tp_traverse*/
    (inquiry)Handle_tp_clear,                                      /*tp_clear*/
    0,                                                             /*tp_richcompare*/
    offsetof(Handle, weakreflist),                                 /*tp_weaklistoffset*/
    0,                                                             /*tp_iter*/
    0,                                                             /*tp_iternext*/
    Handle_tp_methods,                                             /*tp_methods*/
    Handle_tp_members,                                             /*tp_members*/
    Handle_tp_getsets,                                             /*tp_getsets*/
    0,                                                             /*tp_base*/
    0,                                                             /*tp_dict*/
    0,                                                             /*tp_descr_get*/
    0,                                                             /*tp_descr_set*/
    offsetof(Handle, dict),                                        /*tp_dictoffset*/
    0,                                                             /*tp_init*/
    0,                                                             /*tp_alloc*/
    Handle_tp_new,                                                 /*tp_new*/
};

