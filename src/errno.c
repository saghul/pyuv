
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
    UNUSED_ARG(obj);

    if (!PyArg_ParseTuple(args, "i:strerror", &errorno)) {
        return NULL;
    }
    return Py_BuildValue("s", uv_strerror(errorno));
}


static PyMethodDef
Errno_methods[] = {
    { "strerror", (PyCFunction)Errno_func_strerror, METH_VARARGS, "Get string representation of an error code." },
    { NULL }
};


#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_errno_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv._cpyuv.errno",    /*m_name*/
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
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_errno_module);
#else
    module = Py_InitModule("pyuv._cpyuv.errno", Errno_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    module_dict = PyModule_GetDict(module);
    errorcode_dict = PyDict_New();
    if (!module_dict || !errorcode_dict || PyDict_SetItemString(module_dict, "errorcode", errorcode_dict) < 0) {
        return NULL;
    }

#define XX(name, s) inscode(module_dict, errorcode_dict, PYUV_STRINGIFY(UV_##name), UV_##name);
    UV_ERRNO_MAP(XX)
#undef XX

    Py_DECREF(errorcode_dict);

    return module;
}

