
static PyObject *
Util_func_hrtime(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_hrtime());
}


static PyObject *
Util_func_get_free_memory(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_get_free_memory());
}


static PyObject *
Util_func_get_total_memory(PyObject *obj)
{
    UNUSED_ARG(obj);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)uv_get_total_memory());
}


static PyObject *
Util_func_loadavg(PyObject *obj)
{
    double avg[3];

    UNUSED_ARG(obj);

    uv_loadavg(avg);
    return Py_BuildValue("(ddd)", avg[0], avg[1], avg[2]);
}


static PyObject *
Util_func_uptime(PyObject *obj)
{
    double uptime;
    int err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_uptime(&uptime);
    if (err == 0) {
        return PyFloat_FromDouble(uptime);
    } else {
        exc_data = Py_BuildValue("(is)", err, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_resident_set_memory(PyObject *obj)
{
    size_t rss;
    int err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_resident_set_memory(&rss);
    if (err == 0) {
        return PyInt_FromSsize_t(rss);
    } else {
        exc_data = Py_BuildValue("(is)", err, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_interface_addresses(PyObject *obj)
{
    static char buf[INET6_ADDRSTRLEN+1];
    int i, count;
    uv_interface_address_t* interfaces;
    int err;
    PyObject *result, *item, *exc_data;

    UNUSED_ARG(obj);

    err = uv_interface_addresses(&interfaces, &count);
    if (err < 0) {
        exc_data = Py_BuildValue("(is)", err, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }

    result = PyList_New(count);
    if (!result) {
        uv_free_interface_addresses(interfaces, count);
        return NULL;
    }

    for (i = 0; i < count; i++) {
        item = PyStructSequence_New(&InterfaceAddressesResultType);
        if (!item) {
            Py_DECREF(result);
            uv_free_interface_addresses(interfaces, count);
            return NULL;
        }
        PyStructSequence_SET_ITEM(item, 0, Py_BuildValue("s", interfaces[i].name));
        PyStructSequence_SET_ITEM(item, 1, PyBool_FromLong((long)interfaces[i].is_internal));
        if (interfaces[i].address.address4.sin_family == AF_INET) {
            uv_ip4_name(&interfaces[i].address.address4, buf, sizeof(buf));
        } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
            uv_ip6_name(&interfaces[i].address.address6, buf, sizeof(buf));
        }
        PyStructSequence_SET_ITEM(item, 2, Py_BuildValue("s", buf));
        if (interfaces[i].netmask.netmask4.sin_family == AF_INET) {
            uv_ip4_name(&interfaces[i].netmask.netmask4, buf, sizeof(buf));
        } else if (interfaces[i].netmask.netmask4.sin_family == AF_INET6) {
            uv_ip6_name(&interfaces[i].netmask.netmask6, buf, sizeof(buf));
        }
        PyStructSequence_SET_ITEM(item, 3, Py_BuildValue("s", buf));
        PyOS_snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                                        (unsigned char)interfaces[i].phys_addr[0],
                                        (unsigned char)interfaces[i].phys_addr[1],
                                        (unsigned char)interfaces[i].phys_addr[2],
                                        (unsigned char)interfaces[i].phys_addr[3],
                                        (unsigned char)interfaces[i].phys_addr[4],
                                        (unsigned char)interfaces[i].phys_addr[5]);
        PyStructSequence_SET_ITEM(item, 4, Py_BuildValue("s", buf));
        PyList_SET_ITEM(result, i, item);
    }

    uv_free_interface_addresses(interfaces, count);
    return result;
}


static PyObject *
Util_func_cpu_info(PyObject *obj)
{
    int i, count;
    uv_cpu_info_t* cpus;
    int err;
    PyObject *result, *item, *times, *exc_data;

    UNUSED_ARG(obj);

    err = uv_cpu_info(&cpus, &count);
    if (err == 0) {
        result = PyList_New(count);
        if (!result) {
            uv_free_cpu_info(cpus, count);
            return NULL;
        }
        for (i = 0; i < count; i++) {
            item = PyStructSequence_New(&CPUInfoResultType);
            times = PyStructSequence_New(&CPUInfoTimesResultType);
            if (!item || !times) {
                Py_XDECREF(item);
                Py_XDECREF(times);
                Py_DECREF(result);
                uv_free_cpu_info(cpus, count);
                return NULL;
            }
            PyStructSequence_SET_ITEM(item, 0, Py_BuildValue("s", cpus[i].model));
            PyStructSequence_SET_ITEM(item, 1, PyInt_FromLong((long)cpus[i].speed));
            PyStructSequence_SET_ITEM(item, 2, times);
            PyList_SET_ITEM(result, i, item);
            PyStructSequence_SET_ITEM(times, 0, PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.sys));
            PyStructSequence_SET_ITEM(times, 1, PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.user));
            PyStructSequence_SET_ITEM(times, 2, PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.idle));
            PyStructSequence_SET_ITEM(times, 3, PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.irq));
            PyStructSequence_SET_ITEM(times, 4, PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)cpus[i].cpu_times.nice));
        }
        uv_free_cpu_info(cpus, count);
        return result;
    } else {
        exc_data = Py_BuildValue("(is)", err, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_getrusage(PyObject *obj)
{
    int err;
    uv_rusage_t ru;
    PyObject *result;

    UNUSED_ARG(obj);

    err = uv_getrusage(&ru);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UVError);
        return NULL;
    }

    result = PyStructSequence_New(&RusageResultType);
    if (!result)
        return NULL;

#define pyuv__doubletime(TV) ((double)(TV).tv_sec + 1e-6*(TV).tv_usec)
    PyStructSequence_SET_ITEM(result, 0, PyFloat_FromDouble(pyuv__doubletime(ru.ru_utime)));
    PyStructSequence_SET_ITEM(result, 1, PyFloat_FromDouble(pyuv__doubletime(ru.ru_stime)));
    PyStructSequence_SET_ITEM(result, 2, PyLong_FromLong(ru.ru_maxrss));
    PyStructSequence_SET_ITEM(result, 3, PyLong_FromLong(ru.ru_ixrss));
    PyStructSequence_SET_ITEM(result, 4, PyLong_FromLong(ru.ru_idrss));
    PyStructSequence_SET_ITEM(result, 5, PyLong_FromLong(ru.ru_isrss));
    PyStructSequence_SET_ITEM(result, 6, PyLong_FromLong(ru.ru_minflt));
    PyStructSequence_SET_ITEM(result, 7, PyLong_FromLong(ru.ru_majflt));
    PyStructSequence_SET_ITEM(result, 8, PyLong_FromLong(ru.ru_nswap));
    PyStructSequence_SET_ITEM(result, 9, PyLong_FromLong(ru.ru_inblock));
    PyStructSequence_SET_ITEM(result, 10, PyLong_FromLong(ru.ru_oublock));
    PyStructSequence_SET_ITEM(result, 11, PyLong_FromLong(ru.ru_msgsnd));
    PyStructSequence_SET_ITEM(result, 12, PyLong_FromLong(ru.ru_msgrcv));
    PyStructSequence_SET_ITEM(result, 13, PyLong_FromLong(ru.ru_nsignals));
    PyStructSequence_SET_ITEM(result, 14, PyLong_FromLong(ru.ru_nvcsw));
    PyStructSequence_SET_ITEM(result, 15, PyLong_FromLong(ru.ru_nivcsw));
#undef pyuv__doubletime

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}


static PyObject *
Util_func_guess_handle_type(PyObject *obj, PyObject *args)
{
    int fd;

    UNUSED_ARG(obj);

    if (!PyArg_ParseTuple(args, "i:guess_handle_type", &fd)) {
        return NULL;
    }

    return PyInt_FromLong((long) uv_guess_handle(fd));
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
    { "getrusage", (PyCFunction)Util_func_getrusage, METH_NOARGS, "Get information about OS resource utilization for the current process." },
    { "guess_handle_type", (PyCFunction)Util_func_guess_handle_type, METH_VARARGS, "Guess the handle type, given a file descriptor." },
    { NULL }
};


#ifdef PYUV_WINDOWS
    #define GOT_EINTR (WSAGetLastError() == WSAEINTR)
    #define GOT_EAGAIN (WSAGetLastError() == WSAEWOULDBLOCK)
#else
    #define GOT_EINTR (errno == EINTR)
    #define GOT_EAGAIN (errno == EAGAIN || errno == EWOULDBLOCK)
#endif

static int
pyuv__drain_poll_fd(uv_os_sock_t fd)
{
    static char buffer[1024];
    int r;

    do {
        r = recv(fd, buffer, sizeof(buffer), 0);
    } while (r == -1 && GOT_EINTR);

    if (r != -1 || (r == -1 && GOT_EAGAIN)) {
        return 0;
    } else {
        return -1;
    }
}

#undef GOT_EINTR
#undef GOT_EAGAIN


static void
pyuv__check_signals(uv_poll_t *handle, int status, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    SignalChecker *self;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, SignalChecker, poll_h);

    if (status == 0) {
        ASSERT(events == UV_READABLE);
    }

    /* Drain the fd */
    if (pyuv__drain_poll_fd(self->fd) != 0) {
        uv_poll_stop(handle);
    }

    /* Check for signals */
    PyErr_CheckSignals();
    if (PyErr_Occurred()) {
        handle_uncaught_exception(HANDLE(self)->loop);
    }

    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
SignalChecker_func_start(SignalChecker *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_poll_start(&self->poll_h, UV_READABLE, pyuv__check_signals);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UVError);
        return NULL;
    }

    PYUV_HANDLE_INCREF(self);

    Py_RETURN_NONE;
}


static PyObject *
SignalChecker_func_stop(SignalChecker *self)
{
    int err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    err = uv_poll_stop(&self->poll_h);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UVError);
        return NULL;
    }

    PYUV_HANDLE_DECREF(self);

    Py_RETURN_NONE;
}


static int
SignalChecker_tp_init(SignalChecker *self, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!l:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

    err = uv_poll_init_socket(loop->uv_loop, &self->poll_h, (uv_os_sock_t)fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_UVError);
        return -1;
    }

    self->fd = fd;

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
SignalChecker_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    SignalChecker *self;

    self = (SignalChecker *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->poll_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->poll_h;

    return (PyObject *)self;
}


static int
SignalChecker_tp_traverse(SignalChecker *self, visitproc visit, void *arg)
{
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
SignalChecker_tp_clear(SignalChecker *self)
{
    return HandleType.tp_clear((PyObject *)self);
}


static PyMethodDef
SignalChecker_tp_methods[] = {
    { "start", (PyCFunction)SignalChecker_func_start, METH_NOARGS, "Start the SignalChecker." },
    { "stop", (PyCFunction)SignalChecker_func_stop, METH_NOARGS, "Stop the SignalChecker." },
    { NULL }
};


static PyTypeObject SignalCheckerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.util.SignalChecker",                               /*tp_name*/
    sizeof(SignalChecker),                                          /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)SignalChecker_tp_traverse,                        /*tp_traverse*/
    (inquiry)SignalChecker_tp_clear,                                /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    SignalChecker_tp_methods,                                       /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)SignalChecker_tp_init,                                /*tp_init*/
    0,                                                              /*tp_alloc*/
    SignalChecker_tp_new,                                           /*tp_new*/
};


#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_util_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv._cpyuv.util",     /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    Util_methods,           /*m_methods*/
};
#endif

PyObject *
init_util(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_util_module);
#else
    module = Py_InitModule("pyuv._cpyuv.util", Util_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    /* initialize PyStructSequence types */
    if (CPUInfoResultType.tp_name == 0)
        PyStructSequence_InitType(&CPUInfoResultType, &cpu_info_result_desc);
    if (CPUInfoTimesResultType.tp_name == 0)
        PyStructSequence_InitType(&CPUInfoTimesResultType, &cpu_info_times_result_desc);
    if (InterfaceAddressesResultType.tp_name == 0)
        PyStructSequence_InitType(&InterfaceAddressesResultType, &interface_addresses_result_desc);
    if (RusageResultType.tp_name == 0)
        PyStructSequence_InitType(&RusageResultType, &rusage_result_desc);

    SignalCheckerType.tp_base = &HandleType;
    PyUVModule_AddType(module, "SignalChecker", &SignalCheckerType);

    return module;
}

