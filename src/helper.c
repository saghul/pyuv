/* Some helper functions */


/* Add a type to a module */
int
PyUVModule_AddType(PyObject *module, const char *name, PyTypeObject *type)
{
    if (PyType_Ready(type)) {
        return -1;
    }
    Py_INCREF(type);
    if (PyModule_AddObject(module, name, (PyObject *)type)) {
        Py_DECREF(type);
        return -1;
    }
    return 0;
}


/* Add a type to a module */
int
PyUVModule_AddObject(PyObject *module, const char *name, PyObject *value)
{
    Py_INCREF(value);
    if (PyModule_AddObject(module, name, value)) {
        Py_DECREF(value);
        return -1;
    }
    return 0;
}


/* Raise appropriate exception when an error is produced inside libuv */
inline void
raise_uv_exception(Loop *loop, PyObject *exc_type)
{
    uv_err_t err = uv_last_error(loop->uv_loop);
    PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
    if (exc_data != NULL) {
        PyErr_SetObject(exc_type, exc_data);
        Py_DECREF(exc_data);
    }
}


