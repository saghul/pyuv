
static PyObject* PyExc_UVError;

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


static PyObject *
Util_func_uptime(PyObject *self)
{
    double uptime;
    uv_err_t err;

    err = uv_uptime(&uptime);
    if (err.code == UV_OK) {
        return PyFloat_FromDouble(uptime);
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_set_process_title(PyObject *self, PyObject *args)
{
    char *title;
    uv_err_t err;

    if (!PyArg_ParseTuple(args, "s:set_process_title", &title)) {
        return NULL;
    }

    err = uv_set_process_title(title);
    if (err.code == UV_OK) {
        Py_RETURN_NONE;
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_get_process_title(PyObject *self)
{
    char buffer[512];
    uv_err_t err;

    err = uv_get_process_title(buffer, sizeof(buffer));
    if (err.code == UV_OK) {
        return PyString_FromString(buffer);
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyMethodDef
Util_methods[] = {
    { "hrtime", (PyCFunction)Util_func_hrtime, METH_NOARGS, "High resolution time." },
    { "get_free_memory", (PyCFunction)Util_func_get_free_memory, METH_NOARGS, "Get system free memory." },
    { "get_total_memory", (PyCFunction)Util_func_get_total_memory, METH_NOARGS, "Get system total memory." },
    { "loadavg", (PyCFunction)Util_func_loadavg, METH_NOARGS, "Get system load average." },
    { "uptime", (PyCFunction)Util_func_uptime, METH_NOARGS, "Get system uptime." },
    { "set_process_title", (PyCFunction)Util_func_set_process_title, METH_VARARGS, "Sets current process title" },
    { "get_process_title", (PyCFunction)Util_func_get_process_title, METH_NOARGS, "Gets current process title" },
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


