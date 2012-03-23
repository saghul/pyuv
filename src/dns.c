
#include "nameser.h"


static PyObject* PyExc_DNSError;


typedef struct {
    DNSResolver *resolver;
    PyObject *cb;
} ares_cb_data_t;


static void
host_cb(void *arg, int status, int timeouts, struct hostent *hostent)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    char ip[INET6_ADDRSTRLEN];
    char **ptr;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *callback, *dns_name, *errorno, *dns_aliases, *dns_addrlist, *dns_result, *tmp, *result;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_aliases = PyList_New(0);
    dns_addrlist = PyList_New(0);
    dns_result = PyTuple_New(3);

    if (!(dns_aliases && dns_addrlist && dns_result)) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        Py_XDECREF(dns_aliases);
        Py_XDECREF(dns_addrlist);
        Py_XDECREF(dns_result);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = hostent->h_aliases; *ptr != NULL; ptr++) {
        if (*ptr != hostent->h_name && strcmp(*ptr, hostent->h_name)) {
            tmp = PYUVString_FromString(*ptr);
            if (tmp == NULL) {
                break;
            }
            PyList_Append(dns_aliases, tmp);
            Py_DECREF(tmp);
        }
    }
    for (ptr = hostent->h_addr_list; *ptr != NULL; ptr++) {
        if (hostent->h_addrtype == AF_INET) {
            uv_inet_ntop(AF_INET, *ptr, ip, INET_ADDRSTRLEN);
            tmp = PYUVString_FromString(ip);
        } else if (hostent->h_addrtype == AF_INET6) {
            uv_inet_ntop(AF_INET6, *ptr, ip, INET6_ADDRSTRLEN);
            tmp = PYUVString_FromString(ip);
        } else {
            continue;
        }
        if (tmp == NULL) {
            break;
        }
        PyList_Append(dns_addrlist, tmp);
        Py_DECREF(tmp);
    }
    dns_name = PYUVString_FromString(hostent->h_name);

    PyTuple_SET_ITEM(dns_result, 0, dns_name);
    PyTuple_SET_ITEM(dns_result, 1, dns_aliases);
    PyTuple_SET_ITEM(dns_result, 2, dns_addrlist);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
nameinfo_cb(void *arg, int status, int timeouts, char *node, char *service)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *callback, *errorno, *dns_node, *dns_service, *dns_result, *result;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyTuple_New(2);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_node = PYUVString_FromString(node);
    if (service) {
        dns_service = PYUVString_FromString(service);
    } else {
        dns_service = Py_None;
        Py_INCREF(Py_None);
    }

    PyTuple_SET_ITEM(dns_result, 0, dns_node);
    PyTuple_SET_ITEM(dns_result, 1, dns_service);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
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
getaddrinfo_cb(uv_getaddrinfo_t* handle, int status, struct addrinfo* res)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    struct addrinfo *ptr;
    ares_cb_data_t *data;
    uv_err_t err;
    DNSResolver *self;
    PyObject *callback, *addr, *item, *errorno, *dns_result, *result;

    ASSERT(handle);

    data = (ares_cb_data_t*)(handle->data);
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != UV_OK) {
        err = uv_last_error(UV_LOOP(self));
        errorno = PyInt_FromLong((long)err.code);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)UV_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = res; ptr; ptr = ptr->ai_next) {
        addr = makesockaddr(ptr->ai_addr, ptr->ai_addrlen);
        if (!addr) {
            PyErr_NoMemory();
            PyErr_WriteUnraisable(callback);
            break;
        }

        item = Py_BuildValue("iiisO", ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol, ptr->ai_canonname ? ptr->ai_canonname : "", addr);
        Py_DECREF(addr);
        if (!item) {
            PyErr_NoMemory();
            PyErr_WriteUnraisable(callback);
            break;
        }

        PyList_Append(dns_result, item);
        Py_DECREF(item);
    }
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    uv_freeaddrinfo(res);
    PyMem_Free(handle);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_a_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    char ip[INET6_ADDRSTRLEN];
    char **ptr;
    struct hostent *hostent;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_a_reply(answer_buf, answer_len, &hostent, NULL, NULL);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = hostent->h_addr_list; *ptr != NULL; ptr++) {
        uv_inet_ntop(hostent->h_addrtype, *ptr, ip, sizeof(ip));
        tmp = PYUVString_FromString(ip);
        if (tmp == NULL) {
            break;
        }
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_hostent(hostent);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_aaaa_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    char ip[INET6_ADDRSTRLEN];
    char **ptr;
    struct hostent *hostent;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_aaaa_reply(answer_buf, answer_len, &hostent, NULL, NULL);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = hostent->h_addr_list; *ptr != NULL; ptr++) {
        uv_inet_ntop(hostent->h_addrtype, *ptr, ip, sizeof(ip));
        tmp = PYUVString_FromString(ip);
        if (tmp == NULL) {
            break;
        }
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_hostent(hostent);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_cname_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    struct hostent *hostent;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_a_reply(answer_buf, answer_len, &hostent, NULL, NULL);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    tmp = PYUVString_FromString(hostent->h_name);
    PyList_Append(dns_result, tmp);
    Py_DECREF(tmp);
    ares_free_hostent(hostent);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_mx_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    struct ares_mx_reply *mx_reply, *mx_ptr;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_mx_reply(answer_buf, answer_len, &mx_reply);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (mx_ptr = mx_reply; mx_ptr != NULL; mx_ptr = mx_ptr->next) {
        tmp = PyTuple_New(2);
        if (tmp == NULL) {
            break;
        }
        PyTuple_SET_ITEM(tmp, 0, PYUVString_FromString(mx_ptr->host));
        PyTuple_SET_ITEM(tmp, 1, PyInt_FromLong((long)mx_ptr->priority));
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_data(mx_reply);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_ns_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    char **ptr;
    struct hostent *hostent;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_ns_reply(answer_buf, answer_len, &hostent);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (ptr = hostent->h_aliases; *ptr != NULL; ptr++) {
        tmp = PYUVString_FromString(*ptr);
        if (tmp == NULL) {
            break;
        }
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_hostent(hostent);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_txt_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    struct ares_txt_reply *txt_reply, *txt_ptr;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_txt_reply(answer_buf, answer_len, &txt_reply);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (txt_ptr = txt_reply; txt_ptr != NULL; txt_ptr = txt_ptr->next) {
        tmp = PYUVString_FromString((const char *)txt_ptr->txt);
        if (tmp == NULL) {
            break;
        }
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_data(txt_reply);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_srv_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    struct ares_srv_reply *srv_reply, *srv_ptr;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_srv_reply(answer_buf, answer_len, &srv_reply);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (srv_ptr = srv_reply; srv_ptr != NULL; srv_ptr = srv_ptr->next) {
        tmp = PyTuple_New(4);
        if (tmp == NULL) {
            break;
        }
        PyTuple_SET_ITEM(tmp, 0, PYUVString_FromString(srv_ptr->host));
        PyTuple_SET_ITEM(tmp, 1, PyInt_FromLong((long)srv_ptr->port));
        PyTuple_SET_ITEM(tmp, 2, PyInt_FromLong((long)srv_ptr->priority));
        PyTuple_SET_ITEM(tmp, 3, PyInt_FromLong((long)srv_ptr->weight));
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_data(srv_reply);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static void
query_naptr_cb(void *arg, int status,int timeouts, unsigned char *answer_buf, int answer_len)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    int parse_status;
    struct ares_naptr_reply *naptr_reply, *naptr_ptr;
    ares_cb_data_t *data;
    DNSResolver *self;
    PyObject *dns_result, *errorno, *tmp, *result, *callback;

    ASSERT(arg);

    data = (ares_cb_data_t*)arg;
    self = data->resolver;
    callback = data->cb;

    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    parse_status = ares_parse_naptr_reply(answer_buf, answer_len, &naptr_reply);
    if (parse_status != ARES_SUCCESS) {
        errorno = PyInt_FromLong((long)parse_status);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    dns_result = PyList_New(0);
    if (!dns_result) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        errorno = PyInt_FromLong((long)ARES_ENOMEM);
        dns_result = Py_None;
        Py_INCREF(Py_None);
        goto callback;
    }

    for (naptr_ptr = naptr_reply; naptr_ptr != NULL; naptr_ptr = naptr_ptr->next) {
        tmp = PyTuple_New(6);
        if (tmp == NULL) {
            break;
        }
        PyTuple_SET_ITEM(tmp, 0, PyInt_FromLong((long)naptr_ptr->order));
        PyTuple_SET_ITEM(tmp, 1, PyInt_FromLong((long)naptr_ptr->preference));
        PyTuple_SET_ITEM(tmp, 2, PYUVString_FromString((char *)naptr_ptr->flags));
        PyTuple_SET_ITEM(tmp, 3, PYUVString_FromString((char *)naptr_ptr->service));
        PyTuple_SET_ITEM(tmp, 4, PYUVString_FromString((char *)naptr_ptr->regexp));
        PyTuple_SET_ITEM(tmp, 5, PYUVString_FromString(naptr_ptr->replacement));
        PyList_Append(dns_result, tmp);
        Py_DECREF(tmp);
    }
    ares_free_data(naptr_reply);
    errorno = Py_None;
    Py_INCREF(Py_None);

callback:
    result = PyObject_CallFunctionObjArgs(callback, self, dns_result, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(callback);
    }
    Py_XDECREF(result);
    Py_DECREF(dns_result);
    Py_DECREF(errorno);

    Py_DECREF(callback);
    PyMem_Free(data);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
DNSResolver_func_gethostbyname(DNSResolver *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:gethostbyname", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_gethostbyname(self->channel, name, AF_INET, &host_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_gethostbyaddr(DNSResolver *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    int family, length;
    void *address;
    struct in_addr addr4;
    struct in6_addr addr6;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:gethostbyaddr", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (uv_inet_pton(AF_INET, name, &addr4) == 1) {
        family = AF_INET;
        length = sizeof(struct in_addr);
        address = (void *)&addr4;
    } else if (uv_inet_pton(AF_INET6, name, &addr6) == 1) {
        family = AF_INET6;
        length = sizeof(struct in6_addr);
        address = (void *)&addr6;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_gethostbyaddr(self->channel, address, length, family, &host_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_getnameinfo(DNSResolver *self, PyObject *args, PyObject *kwargs)
{
    char *addr;
    int port, flags, length;
    struct in_addr addr4;
    struct in6_addr addr6;
    struct sockaddr *sa;
    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "(si)iO:getnameinfo", &addr, &port, &flags, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (port < 0 || port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return NULL;
    }

    if (uv_inet_pton(AF_INET, addr, &addr4) == 1) {
        sa4 = uv_ip4_addr(addr, port);
        sa = (struct sockaddr *)&sa4;
        length = sizeof(struct sockaddr_in);
    } else if (uv_inet_pton(AF_INET6, addr, &addr6) == 1) {
        sa6 = uv_ip6_addr(addr, port);
        sa = (struct sockaddr *)&sa6;
        length = sizeof(struct sockaddr_in6);
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_getnameinfo(self->channel, sa, length, flags, &nameinfo_cb, (void *)cb_data);
    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_getaddrinfo(DNSResolver *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    char port_str[6];
    int port, family, socktype, protocol, flags, r;
    struct addrinfo hints;
    ares_cb_data_t *cb_data = NULL;
    uv_getaddrinfo_t* handle = NULL;
    PyObject *callback;

    static char *kwlist[] = {"callback", "name", "port", "family", "socktype", "protocol", "flags", NULL};

    port = socktype = protocol = flags = 0;
    family = AF_UNSPEC;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|iiiii:getaddrinfo", kwlist, &name, &callback, &port, &family, &socktype, &protocol, &flags)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (port < 0 || port > 65536) {
        PyErr_SetString(PyExc_ValueError, "port must be between 0 and 65536");
        return NULL;
    }
    snprintf(port_str, sizeof(port_str), "%d", port);

    handle = PyMem_Malloc(sizeof(uv_getaddrinfo_t));
    if (!handle) {
        PyErr_NoMemory();
        goto error;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;
    handle->data = (void *)cb_data;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;

    r = uv_getaddrinfo(UV_LOOP(self), handle, &getaddrinfo_cb, name, port_str, &hints);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_DNSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (handle) {
        PyMem_Free(handle);
    }
    if (cb_data) {
        Py_DECREF(callback);
        PyMem_Free(cb_data);
    }
    return NULL;
}


static PyObject *
DNSResolver_func_query_a(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_a", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_A, &query_a_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_aaaa(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_aaaa", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_AAAA, &query_aaaa_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_cname(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_cname", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_CNAME, &query_cname_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_mx(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_mx", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_MX, &query_mx_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_ns(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_ns", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_NS, &query_ns_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_txt(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_txt", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_TXT, &query_txt_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_srv(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_srv", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_SRV, &query_srv_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_query_naptr(DNSResolver *self, PyObject *args)
{
    char *name;
    ares_cb_data_t *cb_data;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO:query_naptr", &name, &callback)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    cb_data = (ares_cb_data_t*) PyMem_Malloc(sizeof *cb_data);
    if (!cb_data) {
        return PyErr_NoMemory();
    }

    Py_INCREF(callback);
    cb_data->resolver = self;
    cb_data->cb = callback;

    ares_query(self->channel, name, C_IN, T_NAPTR, &query_naptr_cb, (void *)cb_data);

    Py_RETURN_NONE;
}


static PyObject *
DNSResolver_func_cancel(DNSResolver *self)
{
    ares_cancel(self->channel);
    Py_RETURN_NONE;
}


static int
set_dns_servers(DNSResolver *self, PyObject *value)
{
    char *server;
    int i, r, length, ret;
    struct ares_addr_node *servers;
    PyObject *server_list = value;
    PyObject *item;

    ret = 0;

    if (!PyList_Check(server_list)) {
        PyErr_SetString(PyExc_TypeError, "servers argument must be a list");
	return -1;
    }

    length = PyList_Size(server_list);
    servers = PyMem_Malloc(sizeof(struct ares_addr_node) * length);

    for (i = 0; i < length; i++) {
        item = PyList_GetItem(server_list, i);
        if (!item) {
            ret = -1;
            goto servers_set_end;
        }

        server = PyString_AsString(item);
        if (!server) {
            ret = -1;
            goto servers_set_end;
        }

        if (uv_inet_pton(AF_INET, server, &servers[i].addr.addr4) == 1) {
            servers[i].family = AF_INET;
        } else if (uv_inet_pton(AF_INET6, server, &servers[i].addr.addr6) == 1) {
            servers[i].family = AF_INET6;
        } else {
            PyErr_SetString(PyExc_ValueError, "invalid IP address");
            ret = -1;
            goto servers_set_end;
        }

        if (i > 0) {
            servers[i-1].next = &servers[i];
        }
    }

    if (length > 0) {
        servers[length-1].next = NULL;
    } else {
        servers = NULL;
    }

    r = ares_set_servers(self->channel, servers);
    if (r != 0) {
        PyErr_SetString(PyExc_DNSError, "error c-ares library options");
        ret = -1;
    }

servers_set_end:

    PyMem_Free(servers);
    return ret;
}


static PyObject *
DNSResolver_servers_get(DNSResolver *self, void *closure)
{
    int r;
    char ip[INET6_ADDRSTRLEN];
    struct ares_addr_node *server, *servers;
    PyObject *server_list;
    PyObject *tmp;

    UNUSED_ARG(closure);

    server_list = PyList_New(0);
    if (!server_list) {
        PyErr_NoMemory();
        return NULL;
    }

    r = ares_get_servers(self->channel, &servers);
    if (r != 0) {
        PyErr_SetString(PyExc_DNSError, "error getting c-ares nameservers");
        return NULL;
    }

    for (server = servers; server != NULL; server = server->next) {
        if (server->family == AF_INET) {
            uv_inet_ntop(AF_INET, &(server->addr.addr4), ip, INET_ADDRSTRLEN);
            tmp = PYUVString_FromString(ip);
        } else {
            uv_inet_ntop(AF_INET6, &(server->addr.addr6), ip, INET6_ADDRSTRLEN);
            tmp = PYUVString_FromString(ip);
        }
        if (tmp == NULL) {
            break;
        }
        r = PyList_Append(server_list, tmp);
        Py_DECREF(tmp);
        if (r != 0) {
            break;
        }
    }

    return server_list;
}


static int
DNSResolver_servers_set(DNSResolver *self, PyObject *value, void *closure)
{
    UNUSED_ARG(closure);
    return set_dns_servers(self, value);
}


static int
DNSResolver_tp_init(DNSResolver *self, PyObject *args, PyObject *kwargs)
{
    int r, optmask;
    struct ares_options options;
    Loop *loop;
    PyObject *servers = NULL;

    static char *kwlist[] = {"loop", "servers", NULL};

    if (self->channel) {
        PyErr_SetString(PyExc_DNSError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O:__init__", kwlist, &LoopType, &loop, &servers)) {
        return -1;
    }

    Py_INCREF(loop);
    self->loop = loop;

    r = ares_library_init(ARES_LIB_INIT_ALL);
    if (r != 0) {
        PyErr_SetString(PyExc_DNSError, "error initializing c-ares library");
        return -1;
    }

    optmask = ARES_OPT_FLAGS;
    options.flags = ARES_FLAG_USEVC;

    r = uv_ares_init_options(UV_LOOP(self), &self->channel, &options, optmask);
    if (r) {
        PyErr_SetString(PyExc_DNSError, "error c-ares library options");
        return -1;
    }

    if (servers) {
        return set_dns_servers(self, servers);
    }

    return 0;
}


static PyObject *
DNSResolver_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    DNSResolver *self = (DNSResolver *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->channel = NULL;
    return (PyObject *)self;
}


static int
DNSResolver_tp_traverse(DNSResolver *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->loop);
    return 0;
}


static int
DNSResolver_tp_clear(DNSResolver *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->loop);
    return 0;
}


static void
DNSResolver_tp_dealloc(DNSResolver *self)
{
    if (self->channel) {
        uv_ares_destroy(UV_LOOP(self), self->channel);
        self->channel = NULL;
    }
    DNSResolver_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
DNSResolver_tp_methods[] = {
    { "cancel", (PyCFunction)DNSResolver_func_cancel, METH_NOARGS, "Cancel all pending queries on this resolver" },
    { "gethostbyname", (PyCFunction)DNSResolver_func_gethostbyname, METH_VARARGS|METH_KEYWORDS, "Gethostbyname" },
    { "gethostbyaddr", (PyCFunction)DNSResolver_func_gethostbyaddr, METH_VARARGS|METH_KEYWORDS, "Gethostbyaddr" },
    { "getnameinfo", (PyCFunction)DNSResolver_func_getnameinfo, METH_VARARGS|METH_KEYWORDS, "Getnameinfo" },
    { "getaddrinfo", (PyCFunction)DNSResolver_func_getaddrinfo, METH_VARARGS|METH_KEYWORDS, "Getaddrinfo" },
    { "query_a", (PyCFunction)DNSResolver_func_query_a, METH_VARARGS, "Run a DNS query of type A" },
    { "query_aaaa", (PyCFunction)DNSResolver_func_query_aaaa, METH_VARARGS, "Run a DNS query of type AAAA" },
    { "query_cname", (PyCFunction)DNSResolver_func_query_cname, METH_VARARGS, "Run a DNS query of type CNAME" },
    { "query_mx", (PyCFunction)DNSResolver_func_query_mx, METH_VARARGS, "Run a DNS query of type MX" },
    { "query_ns", (PyCFunction)DNSResolver_func_query_ns, METH_VARARGS, "Run a DNS query of type NS" },
    { "query_txt", (PyCFunction)DNSResolver_func_query_txt, METH_VARARGS, "Run a DNS query of type TXT" },
    { "query_srv", (PyCFunction)DNSResolver_func_query_srv, METH_VARARGS, "Run a DNS query of type SRV" },
    { "query_naptr", (PyCFunction)DNSResolver_func_query_naptr, METH_VARARGS, "Run a DNS query of type NAPTR" },
    { NULL }
};


static PyMemberDef DNSResolver_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(DNSResolver, loop), READONLY, "Loop where this DNSResolver is running on."},
    {"data", T_OBJECT, offsetof(DNSResolver, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef DNSResolver_tp_getsets[] = {
    {"servers", (getter)DNSResolver_servers_get, (setter)DNSResolver_servers_set, "DNS server list", NULL},
    {NULL}
};


static PyTypeObject DNSResolverType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.dns.DNSResolver",                                         /*tp_name*/
    sizeof(DNSResolver),                                            /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)DNSResolver_tp_dealloc,                             /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)DNSResolver_tp_traverse,                          /*tp_traverse*/
    (inquiry)DNSResolver_tp_clear,                                  /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    DNSResolver_tp_methods,                                         /*tp_methods*/
    DNSResolver_tp_members,                                         /*tp_members*/
    DNSResolver_tp_getsets,                                         /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)DNSResolver_tp_init,                                  /*tp_init*/
    0,                                                              /*tp_alloc*/
    DNSResolver_tp_new,                                             /*tp_new*/
};

#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_dns_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv.dns",             /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    NULL,                   /*m_methods*/
};
#endif

PyObject *
init_dns(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_dns_module);
#else
    module = Py_InitModule("pyuv.dns", NULL);
#endif

    if (module == NULL) {
        return NULL;
    }

    PyModule_AddIntMacro(module, ARES_NI_NOFQDN);
    PyModule_AddIntMacro(module, ARES_NI_NUMERICHOST);
    PyModule_AddIntMacro(module, ARES_NI_NAMEREQD);
    PyModule_AddIntMacro(module, ARES_NI_NUMERICSERV);
    PyModule_AddIntMacro(module, ARES_NI_DGRAM);
    PyModule_AddIntMacro(module, ARES_NI_TCP);
    PyModule_AddIntMacro(module, ARES_NI_UDP);
    PyModule_AddIntMacro(module, ARES_NI_SCTP);
    PyModule_AddIntMacro(module, ARES_NI_DCCP);
    PyModule_AddIntMacro(module, ARES_NI_NUMERICSCOPE);
    PyModule_AddIntMacro(module, ARES_NI_LOOKUPHOST);
    PyModule_AddIntMacro(module, ARES_NI_LOOKUPSERVICE);
    PyModule_AddIntMacro(module, ARES_NI_IDN);
    PyModule_AddIntMacro(module, ARES_NI_IDN_ALLOW_UNASSIGNED);
    PyModule_AddIntMacro(module, ARES_NI_IDN_USE_STD3_ASCII_RULES);

    PyUVModule_AddType(module, "DNSResolver", &DNSResolverType);

    return module;
}


