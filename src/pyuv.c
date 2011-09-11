
#include "pyuv.h"

#include "utils.c"

#include "loop.c"
#include "async.c"
#include "timer.c"
#include "tcp.c"
#include "udp.c"



static PyObject* PyExc_UVError;

/* Module */
PyMODINIT_FUNC
initpyuv(void)
{
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

    PyExc_UVError = PyErr_NewException("pyuv.UVError", NULL, NULL);
    __PyModule_AddType(error, "UVError", (PyTypeObject *)PyExc_UVError);
    PyExc_AsyncError = PyErr_NewException("pyuv.AsyncError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "AsyncError", (PyTypeObject *)PyExc_AsyncError);
    PyExc_TimerError = PyErr_NewException("pyuv.TimerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TimerError", (PyTypeObject *)PyExc_TimerError);
    PyExc_TCPConnectionError = PyErr_NewException("pyuv.TCPConnectionError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TCPConnectionError", (PyTypeObject *)PyExc_TCPConnectionError);
    PyExc_TCPServerError = PyErr_NewException("pyuv.TCPServerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "TCPServerError", (PyTypeObject *)PyExc_TCPServerError);
    PyExc_UDPServerError = PyErr_NewException("pyuv.UDPServerError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "UDPServerError", (PyTypeObject *)PyExc_UDPServerError);

    __PyModule_AddObject(pyuv, "error", error);

    /* Types */
    __PyModule_AddType(pyuv, "Loop", &LoopType);
    __PyModule_AddType(pyuv, "Async", &AsyncType);
    __PyModule_AddType(pyuv, "Timer", &TimerType);
    __PyModule_AddType(pyuv, "TCPConnection", &TCPConnectionType);
    __PyModule_AddType(pyuv, "TCPServer", &TCPServerType);
    __PyModule_AddType(pyuv, "UDPServer", &UDPServerType);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", macro_str(MODULE_VERSION));
}


