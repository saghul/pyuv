
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
Util_func_resident_set_memory(PyObject *self)
{
    size_t rss;
    uv_err_t err;

    err = uv_resident_set_memory(&rss);
    if (err.code == UV_OK) {
        return PyInt_FromSsize_t(rss);
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
Util_func_interface_addresses(PyObject *self)
{
    int i;
    int count;
    char ip4[INET_ADDRSTRLEN];
    char ip6[INET6_ADDRSTRLEN];
    uv_interface_address_t* interfaces;
    uv_err_t err;
    PyObject *result, *item;

    err = uv_interface_addresses(&interfaces, &count);
    if (err.code == UV_OK) {
        result = PyList_New(0);
        if (!result) {
            uv_free_interface_addresses(interfaces, count);
            PyErr_NoMemory();
            return NULL;
        }
        for (i = 0; i < count; i++) {
            item = PyDict_New();
            if (!item)
                continue;
            PyDict_SetItemString(item, "name", PyString_FromString(interfaces[i].name));
            PyDict_SetItemString(item, "is_internal", PyBool_FromLong((long)interfaces[i].is_internal));
            if (interfaces[i].address.address4.sin_family == AF_INET) {
                uv_ip4_name(&interfaces[i].address.address4, ip4, INET_ADDRSTRLEN);
                PyDict_SetItemString(item, "address", PyString_FromString(ip4));
            } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
                uv_ip6_name(&interfaces[i].address.address6, ip6, INET6_ADDRSTRLEN);
                PyDict_SetItemString(item, "address", PyString_FromString(ip6));
            }
            if (PyList_Append(result, item))
                continue;
            Py_DECREF(item);
        }
        uv_free_interface_addresses(interfaces, count);
        return result;
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static void
set_cpu_info_time_val(PyObject *dict, char *key, uint64_t v)
{
    PyObject *val;
#if SIZEOF_TIME_T > SIZEOF_LONG
    val = PyLong_FromLongLong((PY_LONG_LONG)v);
#else
    val = PyInt_FromLong((long)v);
#endif
    PyDict_SetItemString(dict, key, val);
}

static PyObject *
Util_func_cpu_info(PyObject *self)
{
    int i;
    int count;
    uv_cpu_info_t* cpus;
    uv_err_t err;
    PyObject *result, *item, *times;

    err = uv_cpu_info(&cpus, &count);
    if (err.code == UV_OK) {
        result = PyList_New(0);
        if (!result) {
            uv_free_cpu_info(cpus, count);
            PyErr_NoMemory();
            return NULL;
        }
        for (i = 0; i < count; i++) {
            item = PyDict_New();
            times = PyDict_New();
            if (!item || !times)
                continue;
            PyDict_SetItemString(item, "model", PyString_FromString(cpus[i].model));
            PyDict_SetItemString(item, "speed", PyInt_FromLong((long)cpus[i].speed));
            PyDict_SetItemString(item, "times", times);
            if (PyList_Append(result, item))
                continue;
            Py_DECREF(item);
            set_cpu_info_time_val(times, "sys", cpus[i].cpu_times.sys);
            set_cpu_info_time_val(times, "user", cpus[i].cpu_times.user);
            set_cpu_info_time_val(times, "idle", cpus[i].cpu_times.idle);
            set_cpu_info_time_val(times, "irq", cpus[i].cpu_times.irq);
            set_cpu_info_time_val(times, "nice", cpus[i].cpu_times.nice);
        }
        uv_free_cpu_info(cpus, count);
        return result;
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
    { "resident_set_memory", (PyCFunction)Util_func_resident_set_memory, METH_NOARGS, "Gets resident memory size for the current process." },
    { "interface_addresses", (PyCFunction)Util_func_interface_addresses, METH_NOARGS, "Gets network interface addresses." },
    { "cpu_info", (PyCFunction)Util_func_cpu_info, METH_NOARGS, "Gets system CPU information." },
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


