
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


static PyObject *
Util_func_get_free_memory(PyObject *self)
{
    PyObject *val;
#if SIZEOF_TIME_T > SIZEOF_LONG
    val = PyLong_FromLongLong((PY_LONG_LONG)uv_get_free_memory());
#else
    val = PyInt_FromLong((long)uv_get_free_memory());
#endif
    return val;
}


static PyObject *
Util_func_get_total_memory(PyObject *self)
{
    PyObject *val;
#if SIZEOF_TIME_T > SIZEOF_LONG
    val = PyLong_FromLongLong((PY_LONG_LONG)uv_get_total_memory());
#else
    val = PyInt_FromLong((long)uv_get_total_memory());
#endif
    return val;
}


static PyObject *
Util_func_loadavg(PyObject *self)
{
    double avg[3];
    uv_loadavg(avg);
    return Py_BuildValue("(ddd)", avg[0], avg[1], avg[2]);
}


static PyMethodDef
Util_methods[] = {
    { "hrtime", (PyCFunction)Util_func_hrtime, METH_NOARGS, "High resolution time." },
    { "get_free_memory", (PyCFunction)Util_func_get_free_memory, METH_NOARGS, "Get system free memory." },
    { "get_total_memory", (PyCFunction)Util_func_get_total_memory, METH_NOARGS, "Get system total memory." },
    { "loadavg", (PyCFunction)Util_func_loadavg, METH_NOARGS, "Get system load average." },
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


