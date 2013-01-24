
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
    int i, count;
    char ip[INET6_ADDRSTRLEN];
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
                uv_ip4_name(&interfaces[i].address.address4, ip, INET_ADDRSTRLEN);
            } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
                uv_ip6_name(&interfaces[i].address.address6, ip, INET6_ADDRSTRLEN);
            }
            PyStructSequence_SET_ITEM(item, 2, Py_BuildValue("s", ip));
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


static Bool setup_args_called = False;

static void
setup_args(void)
{
    int r, argc;
    char **argv;
    void (*Py_GetArgcArgv)(int *argc, char ***argv);
    uv_lib_t dlmain;

    r = uv_dlopen(NULL, &dlmain);
    if (r != 0) {
        return;
    }

    r = uv_dlsym(&dlmain, "Py_GetArgcArgv", (void **)&Py_GetArgcArgv);
    if (r != 0) {
        uv_dlclose(&dlmain);
        return;
    }

    Py_GetArgcArgv(&argc, &argv);
    uv_dlclose(&dlmain);

    uv_setup_args(argc, argv);
    setup_args_called = True;
}


static PyObject *
Util_func_set_process_title(PyObject *obj, PyObject *args)
{
    char *title;
    uv_err_t err;

    UNUSED_ARG(obj);

    if (!PyArg_ParseTuple(args, "s:set_process_title", &title)) {
        return NULL;
    }

    if (!setup_args_called) {
        setup_args();
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
Util_func_get_process_title(PyObject *obj)
{
    char buffer[512];
    uv_err_t err;

    UNUSED_ARG(obj);

    if (!setup_args_called) {
        setup_args();
    }

    err = uv_get_process_title(buffer, sizeof(buffer));
    if (err.code == UV_OK) {
        return PyBytes_FromString(buffer);
    } else {
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));
        if (exc_data != NULL) {
            PyErr_SetObject(PyExc_UVError, exc_data);
            Py_DECREF(exc_data);
        }
        return NULL;
    }
}


/* Modified from Python Modules/socketmodule.c */
static PyObject *
makesockaddr(struct sockaddr *addr, int addrlen)
{
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    char ip[INET6_ADDRSTRLEN];

    if (addrlen == 0) {
        /* No address */
        Py_RETURN_NONE;
    }

    switch (addr->sa_family) {
    case AF_INET:
    {
        addr4 = (struct sockaddr_in*)addr;
        uv_ip4_name(addr4, ip, INET_ADDRSTRLEN);
        return Py_BuildValue("si", ip, ntohs(addr4->sin_port));
    }

    case AF_INET6:
    {
        addr6 = (struct sockaddr_in6*)addr;
        uv_ip6_name(addr6, ip, INET6_ADDRSTRLEN);
        return Py_BuildValue("siii", ip, ntohs(addr6->sin6_port), addr6->sin6_flowinfo, addr6->sin6_scope_id);
    }

    default:
        /* If we don't know the address family, don't raise an exception -- return it as a tuple. */
        return Py_BuildValue("is#", addr->sa_family, addr->sa_data, sizeof(addr->sa_data));
    }
}

static void
getaddrinfo_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    struct addrinfo *ptr;
    uv_err_t err;
    Loop *loop;
    GAIRequest *pyreq;
    PyObject *addr, *item, *errorno, *dns_result, *result;

    ASSERT(req);
    pyreq = (GAIRequest *)req->data;
    loop = (Loop *)req->loop->data;

    if (status != 0) {
        err = uv_last_error(loop->uv_loop);
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
    result = PyObject_CallFunctionObjArgs(pyreq->callback, dns_result, errorno, NULL);
    if (result == NULL) {
        handle_uncaught_exception(loop);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(loop);
    Py_DECREF(pyreq->callback);
    pyreq->callback = NULL;
    ((Request *)pyreq)->req = NULL;
    Py_DECREF(pyreq);

    uv_freeaddrinfo(res);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}

static PyObject *
Util_func_getaddrinfo(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    char *host_str;
    char port_str[6];
    int port, family, socktype, protocol, flags, r;
    struct addrinfo hints;
    uv_getaddrinfo_t* req;
    Loop *loop;
    GAIRequest *pyreq;
    PyObject *callback, *host, *idna;

    static char *kwlist[] = {"loop", "callback", "host", "port", "family", "socktype", "protocol", "flags", NULL};

    UNUSED_ARG(obj);
    req = NULL;
    pyreq = NULL;
    port = socktype = protocol = flags = 0;
    family = AF_UNSPEC;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!OO|iiiii:getaddrinfo", kwlist, &LoopType, &loop, &host, &callback, &port, &family, &socktype, &protocol, &flags)) {
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
        PyErr_SetString(PyExc_TypeError, "getaddrinfo() argument 2 must be string or None");
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (port < 0 || port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65535");
        return NULL;
    }
    snprintf(port_str, sizeof(port_str), "%d", port);

    req = PyMem_Malloc(sizeof *req);
    if (!req) {
        PyErr_NoMemory();
        goto error;
    }

    pyreq = (GAIRequest *)PyObject_CallObject((PyObject *)&GAIRequestType, NULL);
    if (!pyreq) {
        PyErr_NoMemory();
        goto error;
    }
    ((Request *)pyreq)->req = (uv_req_t *)req;
    pyreq->callback = callback;

    Py_INCREF(loop);
    Py_INCREF(callback);
    req->data = (void *)pyreq;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;

    r = uv_getaddrinfo(loop->uv_loop, req, &getaddrinfo_cb, host_str, port_str, &hints);
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_UVError);
        goto error;
    }

    Py_INCREF(pyreq);
    return (PyObject *)pyreq;

error:
    Py_XDECREF(pyreq);
    PyMem_Free(req);
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
    { "set_process_title", (PyCFunction)Util_func_set_process_title, METH_VARARGS, "Sets current process title." },
    { "get_process_title", (PyCFunction)Util_func_get_process_title, METH_NOARGS, "Gets current process title." },
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
    self = (SignalChecker *)handle->data;

    Py_INCREF(self);

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

    r = uv_poll_start((uv_poll_t *)UV_HANDLE(self), UV_READABLE, check_signals);
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

    r = uv_poll_stop((uv_poll_t *)UV_HANDLE(self));
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

    r = uv_poll_init_socket(loop->uv_loop, (uv_poll_t *)UV_HANDLE(self), (uv_os_sock_t)fd);
    if (r != 0) {
        RAISE_UV_EXCEPTION(loop->uv_loop, PyExc_UVError);
        return -1;
    }

    self->fd = (uv_os_sock_t)fd;

    initialize_handle(HANDLE(self), loop);

    return 0;
}


static PyObject *
SignalChecker_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    uv_poll_t *uv_poll;

    uv_poll = PyMem_Malloc(sizeof *uv_poll);
    if (!uv_poll) {
        PyErr_NoMemory();
        return NULL;
    }

    SignalChecker *self = (SignalChecker *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        PyMem_Free(uv_poll);
        return NULL;
    }

    uv_poll->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_poll;

    return (PyObject *)self;
}


static int
SignalChecker_tp_traverse(SignalChecker *self, visitproc visit, void *arg)
{
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
SignalChecker_tp_clear(SignalChecker *self)
{
    HandleType.tp_clear((PyObject *)self);
    return 0;
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


