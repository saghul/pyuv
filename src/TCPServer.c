
#include <arpa/inet.h>
#include <sys/socket.h>


static PyObject* PyExc_TCPServerError;


static void
on_tcp_connection(uv_stream_t* server, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(server);

    TCPServer *self = (TCPServer *)(server->data);
    assert(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != 0) {
        uv_err_t err = uv_last_error(SELF_LOOP);
        PyErr_SetString(PyExc_TCPServerError, uv_strerror(err));
        PyErr_WriteUnraisable(PyExc_TCPServerError);
    } else {
        PyObject *result;
        result = PyObject_CallFunctionObjArgs(self->on_new_connection_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_new_connection_cb);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
on_tcp_server_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    assert(handle);
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static PyObject *
TCPServer_func_listen(TCPServer *self, PyObject *args)
{
    int r;
    int backlog = 1;

    if (!PyArg_ParseTuple(args, "i", &backlog)) {
        return NULL;
    }

    if (backlog < 0) {
        PyErr_SetString(PyExc_ValueError, "backlog must be bigger than 0");
        return NULL;
    }

    if (self->address_type == AF_INET) {
        r = uv_tcp_bind(self->uv_tcp_server, uv_ip4_addr(self->listen_ip, self->listen_port));
    } else {
        r = uv_tcp_bind6(self->uv_tcp_server, uv_ip6_addr(self->listen_ip, self->listen_port));
    }
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }

    r = uv_listen((uv_stream_t *)self->uv_tcp_server, backlog, on_tcp_connection);
    if (r) { 
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }

    Py_RETURN_NONE;
}


static PyObject *
TCPServer_func_stop_listening(TCPServer *self)
{
    if (self->uv_tcp_server) {
        self->uv_tcp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_tcp_server, on_tcp_server_close);
        self->uv_tcp_server = NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
TCPServer_func_accept(TCPServer *self, PyObject *args, PyObject *kwargs)
{
    PyObject *read_callback;
    PyObject *write_callback;
    PyObject *close_callback;

    static char *kwlist[] = {"read_cb", "write_cb", "close_cb", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO:accept", kwlist, &read_callback, &write_callback, &close_callback)) {
        return NULL;
    }

    TCPConnection *tcp_connection;
    tcp_connection = (TCPConnection *)PyObject_CallFunction((PyObject *)&TCPConnectionType, "OOOO", self, read_callback, write_callback, close_callback);
    int r = uv_accept((uv_stream_t *)self->uv_tcp_server, tcp_connection->uv_stream);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, NULL);
    }
    TCPConnection_start_read(tcp_connection);

    return (PyObject *)tcp_connection;
}


static int
TCPServer_tp_init(TCPServer *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;
    PyObject *callback;
    PyObject *listen_address = NULL;
    char *listen_ip;
    int listen_port;
    struct in_addr addr4;
    struct in6_addr addr6;
    uv_tcp_t *uv_tcp_server;

    if (!PyArg_ParseTuple(args, "O!OO:__init__", &LoopType, &loop, &listen_address, &callback)) {
        return -1;
    }

    if (!PyArg_ParseTuple(listen_address, "si", &listen_ip, &listen_port)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return -1;
    } else {
        tmp = self->on_new_connection_cb;
        Py_INCREF(callback);
        self->on_new_connection_cb = callback;
        Py_XDECREF(tmp);
    }

    if (listen_port < 0 || listen_port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return -1;
    }

    if (inet_pton(AF_INET, listen_ip, &addr4) == 1) {
        self->address_type = AF_INET;
    } else if (inet_pton(AF_INET6, listen_ip, &addr6) == 1) {
        self->address_type = AF_INET6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return -1;
    }

    tmp = self->listen_address;
    Py_INCREF(listen_address);
    self->listen_address = listen_address;
    Py_XDECREF(tmp);
    self->listen_ip = listen_ip;
    self->listen_port = listen_port;

    uv_tcp_server = PyMem_Malloc(sizeof(uv_tcp_t));
    if (!uv_tcp_server) {
        PyErr_NoMemory();
        return -1;
    }
    int r = uv_tcp_init(SELF_LOOP, uv_tcp_server);
    if (r) {
        RAISE_ERROR(SELF_LOOP, PyExc_TCPServerError, -1);
    }
    uv_tcp_server->data = (void *)self;
    self->uv_tcp_server = uv_tcp_server;

    return 0;
}


static PyObject *
TCPServer_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    TCPServer *self = (TCPServer *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
TCPServer_tp_traverse(TCPServer *self, visitproc visit, void *arg)
{
    Py_VISIT(self->listen_address);
    Py_VISIT(self->on_new_connection_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
TCPServer_tp_clear(TCPServer *self)
{
    Py_CLEAR(self->listen_address);
    Py_CLEAR(self->on_new_connection_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
TCPServer_tp_dealloc(TCPServer *self)
{
    if (self->uv_tcp_server) {
        self->uv_tcp_server->data = NULL;
        uv_close((uv_handle_t *)self->uv_tcp_server, on_tcp_server_close);
        self->uv_tcp_server = NULL;
    }
    TCPServer_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
TCPServer_tp_methods[] = {
    { "listen", (PyCFunction)TCPServer_func_listen, METH_VARARGS, "Start listening for TCP connections." },
    { "stop_listening", (PyCFunction)TCPServer_func_stop_listening, METH_NOARGS, "Stop listening for TCP connections." },
    { "accept", (PyCFunction)TCPServer_func_accept, METH_VARARGS|METH_KEYWORDS, "Accept incoming TCP connection." },
    { NULL }
};


static PyMemberDef TCPServer_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(TCPServer, loop), READONLY, "Loop where this TCPServer is running on."},
    {"listen_address", T_OBJECT_EX, offsetof(TCPServer, listen_address), 0, "Address tuple where this server listens."},
    {NULL}
};


static PyTypeObject TCPServerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.TCPServer",                                               /*tp_name*/
    sizeof(TCPServer),                                              /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)TCPServer_tp_dealloc,                               /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,      /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)TCPServer_tp_traverse,                            /*tp_traverse*/
    (inquiry)TCPServer_tp_clear,                                    /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    TCPServer_tp_methods,                                           /*tp_methods*/
    TCPServer_tp_members,                                           /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)TCPServer_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    TCPServer_tp_new,                                               /*tp_new*/
};


