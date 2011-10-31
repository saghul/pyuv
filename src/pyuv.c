
#include "pyuv.h"

#include "util.c"

#include "loop.c"
#include "async.c"
#include "timer.c"
#include "prepare.c"
#include "idle.c"
#include "check.c"
#include "stream.c"
#include "tcp.c"
#include "udp.c"
#include "dns.c"
#include "threadpool.c"
#include "fs.c"


static PyObject* PyExc_UVError;

/* Module */
static PyObject*
initialize_module(void)
{
    /* Initialize GIL */
    PyEval_InitThreads();

    /* Main module */
    PyObject *pyuv = Py_InitModule("pyuv", NULL);

    /* Error module */
    PyObject *error = Py_InitModule("pyuv.error", NULL);
    __PyModule_AddObject(pyuv, "error", error);

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
    PyModule_AddIntMacro(error, UV_ENOENT);
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
    PyModule_AddIntMacro(error, UV_EEXIST);

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
    PyExc_PrepareError = PyErr_NewException("pyuv.error.PrepareError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "PrepareError", (PyTypeObject *)PyExc_PrepareError);
    PyExc_IdleError = PyErr_NewException("pyuv.error.IdleError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "IdleError", (PyTypeObject *)PyExc_IdleError);
    PyExc_CheckError = PyErr_NewException("pyuv.error.CheckError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "CheckError", (PyTypeObject *)PyExc_CheckError);
    PyExc_TCPServerError = PyErr_NewException("pyuv.error.TCPServerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TCPServerError", (PyTypeObject *)PyExc_TCPServerError);
    PyExc_IOStreamError = PyErr_NewException("pyuv.error.IOStreamError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "IOStreamError", (PyTypeObject *)PyExc_IOStreamError);
    PyExc_TCPClientError = PyErr_NewException("pyuv.error.TCPClientError", PyExc_IOStreamError, NULL);
    __PyModule_AddType(error, "TCPClientError", (PyTypeObject *)PyExc_TCPClientError);
    PyExc_TCPClientConnectionError = PyErr_NewException("pyuv.error.TCPClientConnectionError", PyExc_IOStreamError, NULL);
    __PyModule_AddType(error, "TCPClientConnectionError", (PyTypeObject *)PyExc_TCPClientConnectionError);
    PyExc_UDPConnectionError = PyErr_NewException("pyuv.error.UDPConnectionError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "UDPConnectionError", (PyTypeObject *)PyExc_UDPConnectionError);
    PyExc_DNSError = PyErr_NewException("pyuv.error.DNSError", NULL, NULL);
    __PyModule_AddType(error, "DNSError", (PyTypeObject *)PyExc_DNSError);
    PyExc_ThreadPoolError = PyErr_NewException("pyuv.error.ThreadPoolError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "ThreadPoolError", (PyTypeObject *)PyExc_ThreadPoolError);
    PyExc_FSError = PyErr_NewException("pyuv.error.FSError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "FSError", (PyTypeObject *)PyExc_FSError);

    /* DNS module */
    PyObject *dns = Py_InitModule("pyuv.dns", NULL);
    __PyModule_AddObject(pyuv, "dns", dns);

    /* FS module */
    PyObject *fs = Py_InitModule("pyuv.fs", FS_methods);
    __PyModule_AddObject(pyuv, "fs", fs);

    PyModule_AddIntMacro(dns, ARES_NI_NOFQDN);
    PyModule_AddIntMacro(dns, ARES_NI_NUMERICHOST);
    PyModule_AddIntMacro(dns, ARES_NI_NAMEREQD);
    PyModule_AddIntMacro(dns, ARES_NI_NUMERICSERV);
    PyModule_AddIntMacro(dns, ARES_NI_DGRAM);
    PyModule_AddIntMacro(dns, ARES_NI_TCP);
    PyModule_AddIntMacro(dns, ARES_NI_UDP);
    PyModule_AddIntMacro(dns, ARES_NI_SCTP);
    PyModule_AddIntMacro(dns, ARES_NI_DCCP);
    PyModule_AddIntMacro(dns, ARES_NI_NUMERICSCOPE);
    PyModule_AddIntMacro(dns, ARES_NI_LOOKUPHOST);
    PyModule_AddIntMacro(dns, ARES_NI_LOOKUPSERVICE);
    PyModule_AddIntMacro(dns, ARES_NI_IDN);
    PyModule_AddIntMacro(dns, ARES_NI_IDN_ALLOW_UNASSIGNED);
    PyModule_AddIntMacro(dns, ARES_NI_IDN_USE_STD3_ASCII_RULES);

    __PyModule_AddType(dns, "DNSResolver", &DNSResolverType);

    /* Types */
    __PyModule_AddType(pyuv, "Loop", &LoopType);
    __PyModule_AddType(pyuv, "Async", &AsyncType);
    __PyModule_AddType(pyuv, "Timer", &TimerType);
    __PyModule_AddType(pyuv, "Prepare", &PrepareType);
    __PyModule_AddType(pyuv, "Idle", &IdleType);
    __PyModule_AddType(pyuv, "Check", &CheckType);
    TCPClientType.tp_base = &IOStreamType;
    __PyModule_AddType(pyuv, "TCPClient", &TCPClientType);
    TCPClientConnectionType.tp_base = &IOStreamType;
    __PyModule_AddType(pyuv, "TCPClientConnection", &TCPClientConnectionType);
    __PyModule_AddType(pyuv, "TCPServer", &TCPServerType);
    __PyModule_AddType(pyuv, "UDPConnection", &UDPConnectionType);
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


