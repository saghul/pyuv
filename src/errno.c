
/* Borrowed code from Python (Modules/errnomodule.c) */

static void
inscode(PyObject *module_dict, PyObject *other_dict, const char *name, int code)
{
    PyObject *error_name = PyString_FromString(name);
    PyObject *error_code = PyInt_FromLong((long) code);

    /* Don't bother checking for errors; they'll be caught at the end
     * of the module initialization function by the caller of
     * init_errno().
     */
    if (error_name && error_code) {
        PyDict_SetItem(module_dict, error_name, error_code);
        PyDict_SetItem(other_dict, error_code, error_name);
    }
    Py_XDECREF(error_name);
    Py_XDECREF(error_code);
}


PyObject *
init_errno(void)
{
    PyObject *module;
    PyObject *module_dict;
    PyObject *errorcode_dict;
    PyObject *ares_errorcode_dict;

    module = Py_InitModule("pyuv.errno", NULL);
    if (module == NULL) {
        return NULL;
    }

    module_dict = PyModule_GetDict(module);
    errorcode_dict = PyDict_New();
    ares_errorcode_dict = PyDict_New();
    if (!module_dict || !errorcode_dict || 
        PyDict_SetItemString(module_dict, "errorcode", errorcode_dict) < 0 || PyDict_SetItemString(module_dict, "ares_errorcode", ares_errorcode_dict) < 0) {
        return NULL;
    }

    inscode(module_dict, errorcode_dict, "UV_UNKNOWN", UV_UNKNOWN);
    inscode(module_dict, errorcode_dict, "UV_OK", UV_OK);
    inscode(module_dict, errorcode_dict, "UV_EOF", UV_EOF);
    inscode(module_dict, errorcode_dict, "UV_EADDRINFO", UV_EADDRINFO);
    inscode(module_dict, errorcode_dict, "UV_EACCES", UV_EACCES);
    inscode(module_dict, errorcode_dict, "UV_EAGAIN", UV_EAGAIN);
    inscode(module_dict, errorcode_dict, "UV_EADDRINUSE", UV_EADDRINUSE);
    inscode(module_dict, errorcode_dict, "UV_EADDRNOTAVAIL", UV_EADDRNOTAVAIL);
    inscode(module_dict, errorcode_dict, "UV_EAFNOSUPPORT", UV_EAFNOSUPPORT);
    inscode(module_dict, errorcode_dict, "UV_EALREADY", UV_EALREADY);
    inscode(module_dict, errorcode_dict, "UV_EBADF", UV_EBADF);
    inscode(module_dict, errorcode_dict, "UV_EBUSY", UV_EBUSY);
    inscode(module_dict, errorcode_dict, "UV_ECONNABORTED", UV_ECONNABORTED);
    inscode(module_dict, errorcode_dict, "UV_ECONNREFUSED", UV_ECONNREFUSED);
    inscode(module_dict, errorcode_dict, "UV_ECONNRESET", UV_ECONNRESET);
    inscode(module_dict, errorcode_dict, "UV_EDESTADDRREQ", UV_EDESTADDRREQ);
    inscode(module_dict, errorcode_dict, "UV_EFAULT", UV_EFAULT);
    inscode(module_dict, errorcode_dict, "UV_EHOSTUNREACH", UV_EHOSTUNREACH);
    inscode(module_dict, errorcode_dict, "UV_EINTR", UV_EINTR);
    inscode(module_dict, errorcode_dict, "UV_EINVAL", UV_EINVAL);
    inscode(module_dict, errorcode_dict, "UV_EISCONN", UV_EISCONN);
    inscode(module_dict, errorcode_dict, "UV_EMFILE", UV_EMFILE);
    inscode(module_dict, errorcode_dict, "UV_EMSGSIZE", UV_EMSGSIZE);
    inscode(module_dict, errorcode_dict, "UV_ENETDOWN", UV_ENETDOWN);
    inscode(module_dict, errorcode_dict, "UV_ENETUNREACH", UV_ENETUNREACH);
    inscode(module_dict, errorcode_dict, "UV_ENFILE", UV_ENFILE);
    inscode(module_dict, errorcode_dict, "UV_ENOBUFS", UV_ENOBUFS);
    inscode(module_dict, errorcode_dict, "UV_ENOMEM", UV_ENOMEM);
    inscode(module_dict, errorcode_dict, "UV_ENOTDIR", UV_ENOTDIR);
    inscode(module_dict, errorcode_dict, "UV_EISDIR", UV_EISDIR);
    inscode(module_dict, errorcode_dict, "UV_ENONET", UV_ENONET);
    inscode(module_dict, errorcode_dict, "UV_ENOTCONN", UV_ENOTCONN);
    inscode(module_dict, errorcode_dict, "UV_ENOTSOCK", UV_ENOTSOCK);
    inscode(module_dict, errorcode_dict, "UV_ENOTSUP", UV_ENOTSUP);
    inscode(module_dict, errorcode_dict, "UV_ENOENT", UV_ENOENT);
    inscode(module_dict, errorcode_dict, "UV_ENOSYS", UV_ENOSYS);
    inscode(module_dict, errorcode_dict, "UV_EPIPE", UV_EPIPE);
    inscode(module_dict, errorcode_dict, "UV_EPROTO", UV_EPROTO);
    inscode(module_dict, errorcode_dict, "UV_EPROTONOSUPPORT", UV_EPROTONOSUPPORT);
    inscode(module_dict, errorcode_dict, "UV_EPROTOTYPE", UV_EPROTOTYPE);
    inscode(module_dict, errorcode_dict, "UV_ETIMEDOUT", UV_ETIMEDOUT);
    inscode(module_dict, errorcode_dict, "UV_ECHARSET", UV_ECHARSET);
    inscode(module_dict, errorcode_dict, "UV_EAIFAMNOSUPPORT", UV_EAIFAMNOSUPPORT);
    inscode(module_dict, errorcode_dict, "UV_EAISERVICE", UV_EAISERVICE);
    inscode(module_dict, errorcode_dict, "UV_EAISOCKTYPE", UV_EAISOCKTYPE);
    inscode(module_dict, errorcode_dict, "UV_ESHUTDOWN", UV_ESHUTDOWN);
    inscode(module_dict, errorcode_dict, "UV_EEXIST", UV_EEXIST);
    inscode(module_dict, errorcode_dict, "UV_ESRCH", UV_ESRCH);

    inscode(module_dict, ares_errorcode_dict, "ARES_SUCCESS", ARES_SUCCESS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENODATA", ARES_ENODATA);
    inscode(module_dict, ares_errorcode_dict, "ARES_EFORMERR", ARES_EFORMERR);
    inscode(module_dict, ares_errorcode_dict, "ARES_ESERVFAIL", ARES_ESERVFAIL);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTFOUND", ARES_ENOTFOUND);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTIMP", ARES_ENOTIMP);
    inscode(module_dict, ares_errorcode_dict, "ARES_EREFUSED", ARES_EREFUSED);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADQUERY", ARES_EBADQUERY);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADNAME", ARES_EBADNAME);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADFAMILY", ARES_EBADFAMILY);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADRESP", ARES_EBADRESP);
    inscode(module_dict, ares_errorcode_dict, "ARES_ECONNREFUSED", ARES_ECONNREFUSED);
    inscode(module_dict, ares_errorcode_dict, "ARES_ETIMEOUT", ARES_ETIMEOUT);
    inscode(module_dict, ares_errorcode_dict, "ARES_EOF", ARES_EOF);
    inscode(module_dict, ares_errorcode_dict, "ARES_EFILE", ARES_EFILE);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOMEM", ARES_ENOMEM);
    inscode(module_dict, ares_errorcode_dict, "ARES_EDESTRUCTION", ARES_EDESTRUCTION);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADSTR", ARES_EBADSTR);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADFLAGS", ARES_EBADFLAGS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENONAME", ARES_ENONAME);
    inscode(module_dict, ares_errorcode_dict, "ARES_EBADHINTS", ARES_EBADHINTS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ENOTINITIALIZED", ARES_ENOTINITIALIZED);
    inscode(module_dict, ares_errorcode_dict, "ARES_ELOADIPHLPAPI", ARES_ELOADIPHLPAPI);
    inscode(module_dict, ares_errorcode_dict, "ARES_EADDRGETNETWORKPARAMS", ARES_EADDRGETNETWORKPARAMS);
    inscode(module_dict, ares_errorcode_dict, "ARES_ECANCELLED", ARES_ECANCELLED);

    Py_DECREF(errorcode_dict);
    Py_DECREF(ares_errorcode_dict);

    return module;
}

