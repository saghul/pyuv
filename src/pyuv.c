
#include "pyuv.h"

#include "errno.c"
#include "helper.c"

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
#include "tty.c"
#include "udp.c"
#include "dns.c"
#include "fs.c"
#include "threadpool.c"
#include "process.c"
#include "util.c"

#include "error.c"

#define LIBUV_VERSION UV_VERSION_MAJOR.UV_VERSION_MINOR-LIBUV_REVISION


/* Module */
static PyObject*
initialize_module(void)
{
    PyObject *pyuv;
    PyObject *errno_module;
    PyObject *error;
    PyObject *dns;
    PyObject *fs;
    PyObject *util;

    /* Initialize GIL */
    PyEval_InitThreads();

    /* Main module */
    pyuv = Py_InitModule("pyuv", NULL);

    /* Errno module */
    errno_module = init_errno();
    if (errno_module == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "errno", errno_module);

    /* Error module */
    error = init_error();
    if (error == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "error", error);

    /* DNS module */
    dns = init_dns();
    if (dns == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "dns", dns);
    PyUVModule_AddType(dns, "DNSResolver", &DNSResolverType);

    /* FS module */
    fs = init_fs();
    if (fs == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "fs", fs);

    /* Util module */
    util = init_util();
    if (util == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "util", util);

    /* Types */
    TCPType.tp_base = &IOStreamType;
    PipeType.tp_base = &IOStreamType;
    TTYType.tp_base = &IOStreamType;

    PyUVModule_AddType(pyuv, "Loop", &LoopType);
    PyUVModule_AddType(pyuv, "Async", &AsyncType);
    PyUVModule_AddType(pyuv, "Timer", &TimerType);
    PyUVModule_AddType(pyuv, "Prepare", &PrepareType);
    PyUVModule_AddType(pyuv, "Idle", &IdleType);
    PyUVModule_AddType(pyuv, "Check", &CheckType);
    PyUVModule_AddType(pyuv, "Signal", &SignalType);
    PyUVModule_AddType(pyuv, "TCP", &TCPType);
    PyUVModule_AddType(pyuv, "Pipe", &PipeType);
    PyUVModule_AddType(pyuv, "TTY", &TTYType);
    PyUVModule_AddType(pyuv, "UDP", &UDPType);
    PyUVModule_AddType(pyuv, "ThreadPool", &ThreadPoolType);
    PyUVModule_AddType(pyuv, "Process", &ProcessType);

    /* Constants */
    PyModule_AddIntMacro(pyuv, UV_JOIN_GROUP);
    PyModule_AddIntMacro(pyuv, UV_LEAVE_GROUP);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", __MSTR(MODULE_VERSION));

    /* libuv version */
    PyModule_AddStringConstant(pyuv, "LIBUV_VERSION", __MSTR(LIBUV_VERSION));

    return pyuv;
}

void
initpyuv(void)
{
    initialize_module();
}


