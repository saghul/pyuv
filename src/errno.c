
/* Borrowed code from Python (Modules/errnomodule.c) */

static void
inscode(PyObject *module_dict, PyObject *other_dict, const char *name, int code)
{
    PyObject *error_name = Py_BuildValue("s", name);
    PyObject *error_code = PyInt_FromLong((long) code);

    /* Don't bother checking for errors; they'll be caught at the end
     * of the module initialization function by the caller of
     * init_errno().
     */
    if (error_name && error_code) {
        PyDict_SetItem(module_dict, error_name, error_code);
        PyDict_SetItem(other_dict, error_code, error_name);
    }
    Py_XDECREF(error_name);
    Py_XDECREF(error_code);
}


static PyObject *
Errno_func_strerror(PyObject *obj, PyObject *args)
{
    int errorno;
    uv_err_t err;

    UNUSED_ARG(obj);

    if (!PyArg_ParseTuple(args, "i:strerror", &errorno)) {
        return NULL;
    }

    err.code = errorno;
    return Py_BuildValue("s", uv_strerror(err));
}


static PyObject *
Errno_func_ares_strerror(PyObject *obj, PyObject *args)
{
    int errorno;

    UNUSED_ARG(obj);

    if (!PyArg_ParseTuple(args, "i:ares_strerror", &errorno)) {
        return NULL;
    }

    return Py_BuildValue("s", ares_strerror(errorno));
}


static PyMethodDef
Errno_methods[] = {
    { "ares_strerror", (PyCFunction)Errno_func_ares_strerror, METH_VARARGS, "Get string representation of a c-ares error code." },
    { "strerror", (PyCFunction)Errno_func_strerror, METH_VARARGS, "Get string representation of an error code." },
    { NULL }
};


#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_errorno_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv.errno",           /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    Errno_methods,          /*m_methods*/
};
#endif

PyObject *
init_errno(void)
{
    PyObject *module;
    PyObject *module_dict;
    PyObject *errorcode_dict;
    PyObject *ares_errorcode_dict;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_errorno_module);
#else
    module = Py_InitModule("pyuv.errno", Errno_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    module_dict = PyModule_GetDict(module);
    errorcode_dict = PyDict_New();
    ares_errorcode_dict = PyDict_New();
    if (!module_dict || !errorcode_dict || 
        PyDict_SetItemString(module_dict, "errorcode", errorcode_dict) < 0 || PyDict_SetItemString(module_dict, "ares_errorcode", ares_errorcode_dict) < 0) {
        return NULL;
    }

#define XX(val, name, s) inscode(module_dict, errorcode_dict, __MSTR(UV_##name), UV_##name);
    UV_ERRNO_MAP(XX)
#undef XX

    /* TODO: also use some preprocessor trickery for c-ares errnos */
    inscode(module_dict, ares_errorcode_dict, "ARES_SUCCESS", ARES_SUCCESS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENODATA", ARES_ENODATA);
    inscode(module_dict, ares_errorcode_dict, "ARES_EFORMERR", ARES_EFORMERR);
    inscode(module_dict, ares_errorcode_dict, "ARES_ESERVFAIL", ARES_ESERVFAIL);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTFOUND", ARES_ENOTFOUND);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTIMP", ARES_ENOTIMP);
    inscode(module_dict, ares_errorcode_dict, "ARES_EREFUSED", ARES_EREFUSED);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADQUERY", ARES_EBADQUERY);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADNAME", ARES_EBADNAME);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADFAMILY", ARES_EBADFAMILY);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADRESP", ARES_EBADRESP);
    inscode(module_dict, ares_errorcode_dict, "ARES_ECONNREFUSED", ARES_ECONNREFUSED);
    inscode(module_dict, ares_errorcode_dict, "ARES_ETIMEOUT", ARES_ETIMEOUT);
    inscode(module_dict, ares_errorcode_dict, "ARES_EOF", ARES_EOF);
    inscode(module_dict, ares_errorcode_dict, "ARES_EFILE", ARES_EFILE);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOMEM", ARES_ENOMEM);
    inscode(module_dict, ares_errorcode_dict, "ARES_EDESTRUCTION", ARES_EDESTRUCTION);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADSTR", ARES_EBADSTR);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADFLAGS", ARES_EBADFLAGS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENONAME", ARES_ENONAME);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADHINTS", ARES_EBADHINTS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTINITIALIZED", ARES_ENOTINITIALIZED);
    inscode(module_dict, ares_errorcode_dict, "ARES_ELOADIPHLPAPI", ARES_ELOADIPHLPAPI);
    inscode(module_dict, ares_errorcode_dict, "ARES_EADDRGETNETWORKPARAMS", ARES_EADDRGETNETWORKPARAMS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ECANCELLED", ARES_ECANCELLED);

    Py_DECREF(errorcode_dict);
    Py_DECREF(ares_errorcode_dict);

    return module;
}

