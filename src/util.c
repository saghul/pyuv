
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
    uv_err_t err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_uptime(&uptime);
    if (err.code == UV_OK) {
        return PyFloat_FromDouble(uptime);
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
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
    uv_err_t err;
    PyObject *exc_data;

    UNUSED_ARG(obj);

    err = uv_resident_set_memory(&rss);
    if (err.code == UV_OK) {
        return PyInt_FromSsize_t(rss);
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
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
    uv_err_t err;
    PyObject *result, *item, *exc_data;

    UNUSED_ARG(obj);

    err = uv_interface_addresses(&interfaces, &count);
    if (err.code == UV_OK) {
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
            PyList_SET_ITEM(result, i, item);
        }
        uv_free_interface_addresses(interfaces, count);
        return result;
    } else {
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static PyObject *
Util_func_cpu_info(PyObject *obj)
{
    int i, count;
    uv_cpu_info_t* cpus;
    uv_err_t err;
    PyObject *result, *item, *times, *exc_data;

    UNUSED_ARG(obj);

    err = uv_cpu_info(&cpus, &count);
    if (err.code == UV_OK) {
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
        exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


static void
getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    struct addrinfo *ptr;
    Loop *loop;
    GAIRequest *gai_req;
    PyObject *addr, *item, *errorno, *dns_result, *result;

    ASSERT(req);

    gai_req = PYUV_CONTAINER_OF(req, GAIRequest, req);
    loop = REQUEST(gai_req)->loop;

    if (status != 0) {
        uv_err_t err = uv_last_error(req->loop);
        errorno = PyInt_FromLong((long)err.code);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        errorno = PyInt_FromLong((long)UV_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = res; ptr; ptr = ptr->ai_next) {
        addr = makesockaddr(ptr->ai_addr, ptr->ai_addrlen);
        if (!addr) {
            PyErr_Clear();
            break;
        }

        item = PyStructSequence_New(&AddrinfoResultType);
        if (!item) {
            PyErr_Clear();
            break;
        }
        PyStructSequence_SET_ITEM(item, 0, PyInt_FromLong((long)ptr->ai_family));
        PyStructSequence_SET_ITEM(item, 1, PyInt_FromLong((long)ptr->ai_socktype));
        PyStructSequence_SET_ITEM(item, 2, PyInt_FromLong((long)ptr->ai_protocol));
        PyStructSequence_SET_ITEM(item, 3, Py_BuildValue("s", ptr->ai_canonname ? ptr->ai_canonname : ""));
        PyStructSequence_SET_ITEM(item, 4, addr);

        PyList_Append(dns_result, item);
        Py_DECREF(item);
    }
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(gai_req->callback, dns_result, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(loop);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    uv_freeaddrinfo(res);
    UV_REQUEST(gai_req) = NULL;
    Py_DECREF(gai_req);

    PyGILState_Release(gstate);
}

static PyObject *
Util_func_getaddrinfo(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    char *host_str;
    char port_str[6];
    int port, family, socktype, protocol, flags, r;
    struct addrinfo hints;
    Loop *loop;
    GAIRequest *gai_req;
    PyObject *callback, *host, *idna;

    static char *kwlist[] = {"loop", "callback", "host", "port", "family", "socktype", "protocol", "flags", NULL};

    UNUSED_ARG(obj);
    gai_req = NULL;
    idna = NULL;
    port = socktype = protocol = flags = 0;
    family = AF_UNSPEC;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!OO|iiiii:getaddrinfo", kwlist, &LoopType, &loop, &callback, &host, &port, &family, &socktype, &protocol, &flags)) {
        return NULL;
    }

    if (host == Py_None) {
        host_str = NULL;
    } else if (PyUnicode_Check(host)) {
        idna = PyObject_CallMethod(host, "encode", "s", "idna");
        if (!idna)
            return NULL;
        host_str = PyBytes_AS_STRING(idna);
    } else if (PyBytes_Check(host)) {
        host_str = PyBytes_AsString(host);
    } else {
        PyErr_SetString(PyExc_TypeError, "getaddrinfo() argument 3 must be string or None");
        goto error;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        goto error;
    }

    if (port < 0 || port > 0xffff) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        goto error;
    }
    snprintf(port_str, sizeof(port_str), "%d", port);

    gai_req = (GAIRequest *)PyObject_CallFunctionObjArgs((PyObject *)&GAIRequestType, loop, callback, NULL);
    if (!gai_req) {
        PyErr_NoMemory();
        goto error;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;

    r = uv_getaddrinfo(loop->uv_loop, &gai_req->req, &getaddrinfo_cb, host_str, port_str, &hints);
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_UVError);
        goto error;
    }

    Py_XDECREF(idna);

    Py_INCREF(gai_req);
    return (PyObject *)gai_req;

error:
    Py_XDECREF(idna);
    Py_XDECREF(gai_req);
    return NULL;
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
    { "getaddrinfo", (PyCFunction)Util_func_getaddrinfo, METH_VARARGS|METH_KEYWORDS, "Getaddrinfo" },
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
drain_poll_fd(uv_os_sock_t fd)
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
check_signals(uv_poll_t *handle, int status, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    SignalChecker *self;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, SignalChecker, poll_h);

    if (status == 0) {
        ASSERT(events == UV_READABLE);
    }

    /* Drain the fd */
    if (drain_poll_fd(self->fd) != 0) {
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
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_poll_start(&self->poll_h, UV_READABLE, check_signals);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UVError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
SignalChecker_func_stop(SignalChecker *self)
{
    int r;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    r = uv_poll_stop(&self->poll_h);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_UVError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
SignalChecker_tp_init(SignalChecker *self, PyObject *args, PyObject *kwargs)
{
    int r;
    long fd;
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!l:__init__", &LoopType, &loop, &fd)) {
        return -1;
    }

    r = uv_poll_init_socket(loop->uv_loop, &self->poll_h, (uv_os_sock_t)fd);
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_UVError);
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
    "pyuv.util.SignalChecker",                                      /*tp_name*/
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
    "pyuv.util",            /*m_name*/
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
    module = Py_InitModule("pyuv.util", Util_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    /* initialize PyStructSequence types */
    if (AddrinfoResultType.tp_name == 0)
        PyStructSequence_InitType(&AddrinfoResultType, &addrinfo_result_desc);
    if (CPUInfoResultType.tp_name == 0)
        PyStructSequence_InitType(&CPUInfoResultType, &cpu_info_result_desc);
    if (CPUInfoTimesResultType.tp_name == 0)
        PyStructSequence_InitType(&CPUInfoTimesResultType, &cpu_info_times_result_desc);
    if (InterfaceAddressesResultType.tp_name == 0)
        PyStructSequence_InitType(&InterfaceAddressesResultType, &interface_addresses_result_desc);

    SignalCheckerType.tp_base = &HandleType;
    PyUVModule_AddType(module, "SignalChecker", &SignalCheckerType);

    return module;
}

