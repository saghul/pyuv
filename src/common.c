
/* Add a type to a module */
static int
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


/* Add an object to a module */
static int
PyUVModule_AddObject(PyObject *module, const char *name, PyObject *value)
{
    Py_INCREF(value);
    if (PyModule_AddObject(module, name, value)) {
        Py_DECREF(value);
        return -1;
    }
    return 0;
}


static INLINE int
pyseq2uvbuf(PyObject *seq, Py_buffer **rviews, uv_buf_t **rbufs, int *rbuf_count)
{
    int i, j, buf_count;
    uv_buf_t *uv_bufs = NULL;
    Py_buffer *views = NULL;
    PyObject *data_fast = NULL;

    i = 0;
    *rviews = NULL;
    *rbufs = NULL;
    *rbuf_count = 0;

    if ((data_fast = PySequence_Fast(seq, "argument 1 must be an iterable")) == NULL) {
        goto error;
    }

    buf_count = PySequence_Fast_GET_SIZE(data_fast);
    if (buf_count > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "argument 1 is too long");
        goto error;
    }

    if (buf_count == 0) {
        PyErr_SetString(PyExc_ValueError, "argument 1 is empty");
        goto error;
    }

    uv_bufs = PyMem_Malloc(sizeof *uv_bufs * buf_count);
    views = PyMem_Malloc(sizeof *views * buf_count);
    if (!uv_bufs || !views) {
        PyErr_NoMemory();
        goto error;
    }

    for (i = 0; i < buf_count; i++) {
        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(data_fast, i), PYUV_BYTES"*;argument 1 must be an iterable of buffer-compatible objects", &views[i])) {
            goto error;
        }
        uv_bufs[i].base = views[i].buf;
        uv_bufs[i].len = views[i].len;
    }

    *rviews = views;
    *rbufs = uv_bufs;
    *rbuf_count = buf_count;
    Py_XDECREF(data_fast);
    return 0;

error:
    for (j = 0; j < i; j++) {
        PyBuffer_Release(&views[j]);
    }
    PyMem_Free(views);
    PyMem_Free(uv_bufs);
    Py_XDECREF(data_fast);
    return -1;
}


/* parse a Python tuple containing host, port, flowinfo, scope_id into a sockaddr struct */
static int
pyuv_parse_addr_tuple(PyObject *addr, struct sockaddr_storage *ss)
{
    char *host;
    int port;
    unsigned int scope_id, flowinfo;
    struct in_addr addr4;
    struct in6_addr addr6;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;

    flowinfo = scope_id = 0;

    if (!PyTuple_Check(addr)) {
        PyErr_Format(PyExc_TypeError, "address must be tuple, not %.500s", Py_TYPE(addr)->tp_name);
        return -1;
    }

    if (!PyArg_ParseTuple(addr, "si|II", &host, &port, &flowinfo, &scope_id)) {
        return -1;
    }

    if (port < 0 || port > 0xffff) {
        PyErr_SetString(PyExc_OverflowError, "port must be 0-65535");
        return -1;
    }

    if (flowinfo > 0xfffff) {
        PyErr_SetString(PyExc_OverflowError, "flowinfo must be 0-1048575");
        return -1;
    }

    memset(ss, 0, sizeof(struct sockaddr_storage));

    if (uv_inet_pton(AF_INET, host, &addr4) == 0) {
        /* it's an IPv4 address */
        sa4 = (struct sockaddr_in *)ss;
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons((short)port);
        sa4->sin_addr = addr4;
        return 0;
    } else if (uv_inet_pton(AF_INET6, host, &addr6) == 0) {
        /* it's an IPv4 address */
        sa6 = (struct sockaddr_in6 *)ss;
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons((short)port);
        sa6->sin6_addr = addr6;
        sa6->sin6_flowinfo = flowinfo;
        sa6->sin6_scope_id = scope_id;
        return 0;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return -1;
    }
}


/* Modified from Python Modules/socketmodule.c */
static PyObject *
makesockaddr(struct sockaddr *addr, int addrlen)
{
    static char buf[INET6_ADDRSTRLEN+1];
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    if (addrlen == 0) {
        /* No address */
        Py_RETURN_NONE;
    }

    switch (addr->sa_family) {
    case AF_INET:
    {
        addr4 = (struct sockaddr_in*)addr;
        uv_ip4_name(addr4, buf, sizeof(buf));
        return Py_BuildValue("si", buf, ntohs(addr4->sin_port));
    }

    case AF_INET6:
    {
        addr6 = (struct sockaddr_in6*)addr;
        uv_ip6_name(addr6, buf, sizeof(buf));
        return Py_BuildValue("siII", buf, ntohs(addr6->sin6_port), ntohl(addr6->sin6_flowinfo), addr6->sin6_scope_id);
    }

    default:
        /* If we don't know the address family, don't raise an exception -- return it as a tuple. */
        return Py_BuildValue("is#", addr->sa_family, addr->sa_data, sizeof(addr->sa_data));
    }
}


/* handle uncausht exception in a callback */
static void
handle_uncaught_exception(Loop *loop)
{
    PyObject *excepthook, *exc, *value, *tb, *result;
    Bool exc_in_hook = False;

    ASSERT(loop);
    ASSERT(PyErr_Occurred());
    PyErr_Fetch(&exc, &value, &tb);

    excepthook = PyObject_GetAttrString((PyObject *)loop, "excepthook");
    if (excepthook == NULL) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PySys_WriteStderr("Exception while getting excepthook\n");
            PyErr_PrintEx(0);
            exc_in_hook = True;
        }
        PyErr_Restore(exc, value, tb);
    } else if (excepthook == Py_None) {
        PyErr_Restore(exc, value, tb);
    } else {
        PyErr_NormalizeException(&exc, &value, &tb);
        if (!value)
            PYUV_SET_NONE(value);
        if (!tb)
            PYUV_SET_NONE(tb);
        result = PyObject_CallFunctionObjArgs(excepthook, exc, value, tb, NULL);
        if (result == NULL) {
            PySys_WriteStderr("Unhandled exception in excepthook\n");
            PyErr_PrintEx(0);
            exc_in_hook = True;
            PyErr_Restore(exc, value, tb);
        } else {
            Py_DECREF(exc);
            Py_DECREF(value);
            Py_DECREF(tb);
        }
        Py_XDECREF(result);
    }
    Py_XDECREF(excepthook);

    /* Exception still pending, print it to stderr */
    if (PyErr_Occurred()) {
        if (exc_in_hook)
            PySys_WriteStderr("\n");
        PySys_WriteStderr("Unhandled exception in callback\n");
        PyErr_PrintEx(0);
    }
}

