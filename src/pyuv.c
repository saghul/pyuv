
#include "pyuv.h"

#include "util.c"

#include "loop.c"
#include "async.c"
#include "timer.c"
#include "tcp.c"
#include "udp.c"
#include "dns.c"
#include "threadpool.c"


static PyObject* PyExc_UVError;

/* Module */
static PyObject*
initialize_module(void)
{
    /* Initialize GIL */
    PyEval_InitThreads();

    /* Main module */
    PyObject *pyuv = Py_InitModule("pyuv", NULL);

    /* Error submodule */
    PyObject *error = Py_InitModule("pyuv.error", NULL);

    PyModule_AddIntMacro(error, UV_UNKNOWN);
    PyModule_AddIntMacro(error, UV_OK);
    PyModule_AddIntMacro(error, UV_EOF);
    PyModule_AddIntMacro(error, UV_EACCESS);
    PyModule_AddIntMacro(error, UV_EAGAIN);
    PyModule_AddIntMacro(error, UV_EADDRINUSE);
    PyModule_AddIntMacro(error, UV_EADDRNOTAVAIL);
    PyModule_AddIntMacro(error, UV_EAFNOSUPPORT);
    PyModule_AddIntMacro(error, UV_EALREADY);
    PyModule_AddIntMacro(error, UV_EBADF);
    PyModule_AddIntMacro(error, UV_EBUSY);
    PyModule_AddIntMacro(error, UV_ECONNABORTED);
    PyModule_AddIntMacro(error, UV_ECONNREFUSED);
    PyModule_AddIntMacro(error, UV_ECONNRESET);
    PyModule_AddIntMacro(error, UV_EDESTADDRREQ);
    PyModule_AddIntMacro(error, UV_EFAULT);
    PyModule_AddIntMacro(error, UV_EHOSTUNREACH);
    PyModule_AddIntMacro(error, UV_EINTR);
    PyModule_AddIntMacro(error, UV_EINVAL);
    PyModule_AddIntMacro(error, UV_EISCONN);
    PyModule_AddIntMacro(error, UV_EMFILE);
    PyModule_AddIntMacro(error, UV_EMSGSIZE);
    PyModule_AddIntMacro(error, UV_ENETDOWN);
    PyModule_AddIntMacro(error, UV_ENETUNREACH);
    PyModule_AddIntMacro(error, UV_ENFILE);
    PyModule_AddIntMacro(error, UV_ENOBUFS);
    PyModule_AddIntMacro(error, UV_ENOMEM);
    PyModule_AddIntMacro(error, UV_ENONET);
    PyModule_AddIntMacro(error, UV_ENOPROTOOPT);
    PyModule_AddIntMacro(error, UV_ENOTCONN);
    PyModule_AddIntMacro(error, UV_ENOTSOCK);
    PyModule_AddIntMacro(error, UV_ENOTSUP);
    PyModule_AddIntMacro(error, UV_EPIPE);
    PyModule_AddIntMacro(error, UV_EPROTO);
    PyModule_AddIntMacro(error, UV_EPROTONOSUPPORT);
    PyModule_AddIntMacro(error, UV_EPROTOTYPE);
    PyModule_AddIntMacro(error, UV_ETIMEDOUT);
    PyModule_AddIntMacro(error, UV_ECHARSET);
    PyModule_AddIntMacro(error, UV_EAIFAMNOSUPPORT);
    PyModule_AddIntMacro(error, UV_EAINONAME);
    PyModule_AddIntMacro(error, UV_EAISERVICE);
    PyModule_AddIntMacro(error, UV_EAISOCKTYPE);
    PyModule_AddIntMacro(error, UV_ESHUTDOWN);

    PyModule_AddIntMacro(error, ARES_SUCCESS);
    PyModule_AddIntMacro(error, ARES_ENODATA);
    PyModule_AddIntMacro(error, ARES_EFORMERR);
    PyModule_AddIntMacro(error, ARES_ESERVFAIL);
    PyModule_AddIntMacro(error, ARES_ENOTFOUND);
    PyModule_AddIntMacro(error, ARES_ENOTIMP);
    PyModule_AddIntMacro(error, ARES_EREFUSED);
    PyModule_AddIntMacro(error, ARES_EBADQUERY);
    PyModule_AddIntMacro(error, ARES_EBADNAME);
    PyModule_AddIntMacro(error, ARES_EBADFAMILY);
    PyModule_AddIntMacro(error, ARES_EBADRESP);
    PyModule_AddIntMacro(error, ARES_ECONNREFUSED);
    PyModule_AddIntMacro(error, ARES_ETIMEOUT);
    PyModule_AddIntMacro(error, ARES_EOF);
    PyModule_AddIntMacro(error, ARES_EFILE);
    PyModule_AddIntMacro(error, ARES_ENOMEM);
    PyModule_AddIntMacro(error, ARES_EDESTRUCTION);
    PyModule_AddIntMacro(error, ARES_EBADSTR);
    PyModule_AddIntMacro(error, ARES_EBADFLAGS);
    PyModule_AddIntMacro(error, ARES_ENONAME);
    PyModule_AddIntMacro(error, ARES_EBADHINTS);
    PyModule_AddIntMacro(error, ARES_ENOTINITIALIZED);
    PyModule_AddIntMacro(error, ARES_ELOADIPHLPAPI);
    PyModule_AddIntMacro(error, ARES_EADDRGETNETWORKPARAMS);
    PyModule_AddIntMacro(error, ARES_ECANCELLED);

    PyExc_UVError = PyErr_NewException("pyuv.error.UVError", NULL, NULL);
    __PyModule_AddType(error, "UVError", (PyTypeObject *)PyExc_UVError);
    PyExc_AsyncError = PyErr_NewException("pyuv.error.AsyncError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "AsyncError", (PyTypeObject *)PyExc_AsyncError);
    PyExc_TimerError = PyErr_NewException("pyuv.error.TimerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TimerError", (PyTypeObject *)PyExc_TimerError);
    PyExc_TCPConnectionError = PyErr_NewException("pyuv.error.TCPConnectionError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TCPConnectionError", (PyTypeObject *)PyExc_TCPConnectionError);
    PyExc_TCPServerError = PyErr_NewException("pyuv.error.TCPServerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TCPServerError", (PyTypeObject *)PyExc_TCPServerError);
    PyExc_UDPServerError = PyErr_NewException("pyuv.error.UDPServerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "UDPServerError", (PyTypeObject *)PyExc_UDPServerError);
    PyExc_DNSError = PyErr_NewException("pyuv.error.DNSError", NULL, NULL);
    __PyModule_AddType(error, "DNSError", (PyTypeObject *)PyExc_DNSError);
    PyExc_ThreadPoolError = PyErr_NewException("pyuv.error.ThreadPoolError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "ThreadPoolError", (PyTypeObject *)PyExc_ThreadPoolError);

    __PyModule_AddObject(pyuv, "error", error);

    /* Macros */
    PyModule_AddIntMacro(pyuv, ARES_NI_NOFQDN);
    PyModule_AddIntMacro(pyuv, ARES_NI_NUMERICHOST);
    PyModule_AddIntMacro(pyuv, ARES_NI_NAMEREQD);
    PyModule_AddIntMacro(pyuv, ARES_NI_NUMERICSERV);
    PyModule_AddIntMacro(pyuv, ARES_NI_DGRAM);
    PyModule_AddIntMacro(pyuv, ARES_NI_TCP);
    PyModule_AddIntMacro(pyuv, ARES_NI_UDP);
    PyModule_AddIntMacro(pyuv, ARES_NI_SCTP);
    PyModule_AddIntMacro(pyuv, ARES_NI_DCCP);
    PyModule_AddIntMacro(pyuv, ARES_NI_NUMERICSCOPE);
    PyModule_AddIntMacro(pyuv, ARES_NI_LOOKUPHOST);
    PyModule_AddIntMacro(pyuv, ARES_NI_LOOKUPSERVICE);
    PyModule_AddIntMacro(pyuv, ARES_NI_IDN);
    PyModule_AddIntMacro(pyuv, ARES_NI_IDN_ALLOW_UNASSIGNED);
    PyModule_AddIntMacro(pyuv, ARES_NI_IDN_USE_STD3_ASCII_RULES);

    /* Types */
    __PyModule_AddType(pyuv, "Loop", &LoopType);
    __PyModule_AddType(pyuv, "Async", &AsyncType);
    __PyModule_AddType(pyuv, "Timer", &TimerType);
    __PyModule_AddType(pyuv, "TCPConnection", &TCPConnectionType);
    __PyModule_AddType(pyuv, "TCPServer", &TCPServerType);
    __PyModule_AddType(pyuv, "UDPServer", &UDPServerType);
    __PyModule_AddType(pyuv, "DNSResolver", &DNSResolverType);
    __PyModule_AddType(pyuv, "ThreadPool", &ThreadPoolType);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", __MSTR(MODULE_VERSION));

    return pyuv;
}

void
initpyuv(void)
{
    initialize_module();
}


