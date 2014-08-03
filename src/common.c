
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


/* Encode a Python unicode object into bytes using the default filesystem encoding.
 * Falls back to utf-8. On Windows, this function always uses utf-8, because libuv
 * expects to get utf-8. */
static PyObject *
pyuv_PyUnicode_EncodeFSDefault(PyObject *unicode)
{
#ifndef PYUV_WINDOWS
    if (Py_FileSystemDefaultEncoding)
        return PyUnicode_AsEncodedString(unicode, Py_FileSystemDefaultEncoding, "surrogateescape");
    else
#endif
        return PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(unicode), PyUnicode_GET_SIZE(unicode), "surrogateescape");
}


/* Converter: encode unicode (or str in Python 3) objects into bytes using the default encoding. */
static int
pyuv_PyUnicode_FSConverter(PyObject *arg, void* addr)
{
    PyObject *output;
    Py_ssize_t size;
    void *data;

    if (arg == NULL)
        return 0;

    if (PyBytes_Check(arg)) {
        output = arg;
        Py_INCREF(output);
    } else {
        arg = PyUnicode_FromObject(arg);
        if (!arg)
            return 0;
        output = pyuv_PyUnicode_EncodeFSDefault(arg);
        Py_DECREF(arg);
        if (!output)
            return 0;
        if (!PyBytes_Check(output)) {
            Py_DECREF(output);
            PyErr_SetString(PyExc_TypeError, "encoder failed to return bytes");
            return 0;
        }
    }
    size = PyBytes_GET_SIZE(output);
    data = PyBytes_AS_STRING(output);
    if (size != strlen(data)) {
        PyErr_SetString(PyExc_TypeError, "embedded NUL character");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return 1;
}


/* Extract the content of a unicode or bytes object and duplicate it. */
static char*
pyuv_dup_strobj(PyObject *obj)
{
    PyObject *bytes;
    char *data, *out;
    Py_ssize_t size;

    if (pyuv_PyUnicode_FSConverter(obj, &bytes) > 0) {
        data = PyBytes_AS_STRING(bytes);
        size = PyBytes_GET_SIZE(bytes) + 1;
        out = PyMem_Malloc(size);
        if (!out) {
            PyErr_NoMemory();
            Py_DECREF(bytes);
            return NULL;
        }
        memcpy(out, data, size);
        Py_DECREF(bytes);
        return out;
    }

    return NULL;
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

    if (host[0] == '\0') {
        /* special case, interpret ('', 1234) as 0.0.0.0:1234 */
        uv_inet_pton(AF_INET, "0.0.0.0", &addr4);
        sa4 = (struct sockaddr_in *)ss;
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons((short)port);
        sa4->sin_addr = addr4;
        return 0;
    } else if (uv_inet_pton(AF_INET, host, &addr4) == 0) {
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
makesockaddr(struct sockaddr *addr)
{
    static char buf[INET6_ADDRSTRLEN+1];
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

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
        /* If we don't know the address family, don't raise an exception -- return None. */
        Py_RETURN_NONE;
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


static void
pyuv__alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf)
{
    Loop *loop;
    loop = handle->loop->data;
    ASSERT(loop);

    if (loop->buffer.in_use) {
        buf->base = NULL;
        buf->len = 0;
    } else {
        buf->base = loop->buffer.slab;
        buf->len = sizeof(loop->buffer.slab);
        loop->buffer.in_use = True;
    }
}

