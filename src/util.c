
static PyObject *
Util_func_hrtime(PyObject *self)
{
    PyObject *val;
#if SIZEOF_TIME_T > SIZEOF_LONG
    val = PyLong_FromLongLong((PY_LONG_LONG)uv_hrtime());
#else
    val = PyInt_FromLong((long)uv_hrtime());
#endif
    return val;
}


static PyMethodDef
Util_methods[] = {
    { "hrtime", (PyCFunction)Util_func_hrtime, METH_NOARGS, "High resolution time." },
    { NULL }
};


PyObject *
init_util(void)
{
    PyObject *module;
    module = Py_InitModule("pyuv.util", Util_methods);
    if (module == NULL) {
        return NULL;
    }

    return module;
}


