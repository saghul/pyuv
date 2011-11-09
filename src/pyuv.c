
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
#include "pipe.c"
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
    PyUVModule_AddObject(pyuv, "errno", errno_module);

    /* Error module */
    PyObject *error = Py_InitModule("pyuv.error", NULL);
    PyUVModule_AddObject(pyuv, "error", error);

    PyExc_UVError = PyErr_NewException("pyuv.error.UVError", NULL, NULL);
    PyExc_AsyncError = PyErr_NewException("pyuv.error.AsyncError", PyExc_UVError, NULL);
    PyExc_TimerError = PyErr_NewException("pyuv.error.TimerError", PyExc_UVError, NULL);
    PyExc_PrepareError = PyErr_NewException("pyuv.error.PrepareError", PyExc_UVError, NULL);
    PyExc_IdleError = PyErr_NewException("pyuv.error.IdleError", PyExc_UVError, NULL);
    PyExc_CheckError = PyErr_NewException("pyuv.error.CheckError", PyExc_UVError, NULL);
    PyExc_SignalError = PyErr_NewException("pyuv.error.SignalError", PyExc_UVError, NULL);
    PyExc_IOStreamError = PyErr_NewException("pyuv.error.IOStreamError", PyExc_UVError, NULL);
    PyExc_TCPError = PyErr_NewException("pyuv.error.TCPError", PyExc_IOStreamError, NULL);
    PyExc_PipeError = PyErr_NewException("pyuv.error.PipeError", PyExc_IOStreamError, NULL);
    PyExc_UDPConnectionError = PyErr_NewException("pyuv.error.UDPConnectionError", PyExc_UVError, NULL);
    PyExc_DNSError = PyErr_NewException("pyuv.error.DNSError", NULL, NULL);
    PyExc_ThreadPoolError = PyErr_NewException("pyuv.error.ThreadPoolError", PyExc_UVError, NULL);
    PyExc_FSError = PyErr_NewException("pyuv.error.FSError", PyExc_UVError, NULL);

    PyUVModule_AddType(error, "UVError", (PyTypeObject *)PyExc_UVError);
    PyUVModule_AddType(error, "AsyncError", (PyTypeObject *)PyExc_AsyncError);
    PyUVModule_AddType(error, "TimerError", (PyTypeObject *)PyExc_TimerError);
    PyUVModule_AddType(error, "PrepareError", (PyTypeObject *)PyExc_PrepareError);
    PyUVModule_AddType(error, "IdleError", (PyTypeObject *)PyExc_IdleError);
    PyUVModule_AddType(error, "CheckError", (PyTypeObject *)PyExc_CheckError);
    PyUVModule_AddType(error, "SignalError", (PyTypeObject *)PyExc_SignalError);
    PyUVModule_AddType(error, "IOStreamError", (PyTypeObject *)PyExc_IOStreamError);
    PyUVModule_AddType(error, "TCPError", (PyTypeObject *)PyExc_TCPError);
    PyUVModule_AddType(error, "PipeError", (PyTypeObject *)PyExc_PipeError);
    PyUVModule_AddType(error, "UDPConnectionError", (PyTypeObject *)PyExc_UDPConnectionError);
    PyUVModule_AddType(error, "DNSError", (PyTypeObject *)PyExc_DNSError);
    PyUVModule_AddType(error, "ThreadPoolError", (PyTypeObject *)PyExc_ThreadPoolError);
    PyUVModule_AddType(error, "FSError", (PyTypeObject *)PyExc_FSError);

    /* DNS module */
    PyObject *dns = init_dns();
    if (dns == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "dns", dns);
    PyUVModule_AddType(dns, "DNSResolver", &DNSResolverType);

    /* FS module */
    PyObject *fs = init_fs();
    if (fs == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "fs", fs);

    /* Types */
    TCPType.tp_base = &IOStreamType;
    PipeType.tp_base = &IOStreamType;

    PyUVModule_AddType(pyuv, "Loop", &LoopType);
    PyUVModule_AddType(pyuv, "Async", &AsyncType);
    PyUVModule_AddType(pyuv, "Timer", &TimerType);
    PyUVModule_AddType(pyuv, "Prepare", &PrepareType);
    PyUVModule_AddType(pyuv, "Idle", &IdleType);
    PyUVModule_AddType(pyuv, "Check", &CheckType);
    PyUVModule_AddType(pyuv, "Signal", &SignalType);
    PyUVModule_AddType(pyuv, "TCP", &TCPType);
    PyUVModule_AddType(pyuv, "Pipe", &PipeType);
    PyUVModule_AddType(pyuv, "UDPConnection", &UDPConnectionType);
    PyUVModule_AddType(pyuv, "ThreadPool", &ThreadPoolType);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", __MSTR(MODULE_VERSION));

    return pyuv;
}

void
initpyuv(void)
{
    initialize_module();
}


