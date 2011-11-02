
#include "pyuv.h"

#include "errno.c"
#include "util.c"

#include "loop.c"
#include "async.c"
#include "timer.c"
#include "prepare.c"
#include "idle.c"
#include "check.c"
#include "signal.c"
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

    /* Errno module */
    PyObject *errno_module = init_errno();
    if (errno_module == NULL) {
        return NULL;
    }
    __PyModule_AddObject(pyuv, "errno", errno_module);

    /* Error module */
    PyObject *error = Py_InitModule("pyuv.error", NULL);
    __PyModule_AddObject(pyuv, "error", error);

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
    PyExc_SignalError = PyErr_NewException("pyuv.error.SignalError", PyExc_UVError, NULL);
    __PyModule_AddType(error, "SignalError", (PyTypeObject *)PyExc_SignalError);
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
    PyObject *dns = init_dns();
    if (dns == NULL) {
        return NULL;
    }
    __PyModule_AddObject(pyuv, "dns", dns);
    __PyModule_AddType(dns, "DNSResolver", &DNSResolverType);

    /* FS module */
    PyObject *fs = init_fs();
    if (fs == NULL) {
        return NULL;
    }
    __PyModule_AddObject(pyuv, "fs", fs);

    /* Types */
    __PyModule_AddType(pyuv, "Loop", &LoopType);
    __PyModule_AddType(pyuv, "Async", &AsyncType);
    __PyModule_AddType(pyuv, "Timer", &TimerType);
    __PyModule_AddType(pyuv, "Prepare", &PrepareType);
    __PyModule_AddType(pyuv, "Idle", &IdleType);
    __PyModule_AddType(pyuv, "Check", &CheckType);
    __PyModule_AddType(pyuv, "Signal", &SignalType);
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


