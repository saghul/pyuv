
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

#ifdef PY3
/* pyuv_module */
static PyModuleDef pyuv_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv",                                   /*m_name*/
    NULL,                                     /*m_doc*/
    -1,                                       /*m_size*/
    NULL,                                     /*m_methods*/
};
#endif

/* Module */
static PyObject*
initialize_module(void)
{
    /* Initialize GIL */
    PyEval_InitThreads();

    /* Main module */
    PyObject *pyuv;
#ifdef PY3
    pyuv = PyModule_Create(&pyuv_module);
#else
    pyuv = Py_InitModule("pyuv", NULL);
#endif
    /* Errno module */
    PyObject *errno_module;
#ifdef PY3
    errno_module = PyInit_pyuverrno();
#else
    errno_module = init_errno();
#endif
    if (errno_module == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "errno", errno_module);

    /* Error module */
    PyObject *error;
#ifdef PY3
    error = PyInit_error(); 
#else
    error = init_error();
#endif
    if (error == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "error", error);

    /* DNS module */
    PyObject *dns;
#ifdef PY3
    dns = PyInit_dns();
#else
    dns = init_dns();
#endif
    if (dns == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "dns", dns);
    PyUVModule_AddType(dns, "DNSResolver", &DNSResolverType);

    /* FS module */
    PyObject *fs;
#ifdef PY3
    fs = PyInit_fs();
#else
    fs = init_fs();
#endif
    if (fs == NULL) {
        return NULL;
    }
    PyUVModule_AddObject(pyuv, "fs", fs);

    /* Util module */
    PyObject *util;
#ifdef PY3
    util = PyInit_util();
#else
    util = init_util();
#endif
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

    PyModule_AddIntMacro(pyuv, UV_UNKNOWN_HANDLE);
    PyModule_AddIntMacro(pyuv, UV_TCP);
    PyModule_AddIntMacro(pyuv, UV_UDP);
    PyModule_AddIntMacro(pyuv, UV_NAMED_PIPE);
    PyModule_AddIntMacro(pyuv, UV_TTY);
    PyModule_AddIntMacro(pyuv, UV_FILE);
    PyModule_AddIntMacro(pyuv, UV_TIMER);
    PyModule_AddIntMacro(pyuv, UV_PREPARE);
    PyModule_AddIntMacro(pyuv, UV_CHECK);
    PyModule_AddIntMacro(pyuv, UV_IDLE);
    PyModule_AddIntMacro(pyuv, UV_ASYNC);
    PyModule_AddIntMacro(pyuv, UV_ARES_TASK);
    PyModule_AddIntMacro(pyuv, UV_ARES_EVENT);
    PyModule_AddIntMacro(pyuv, UV_PROCESS);
    PyModule_AddIntMacro(pyuv, UV_FS_EVENT);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", __MSTR(MODULE_VERSION));

    /* libuv version */
    PyModule_AddStringConstant(pyuv, "LIBUV_VERSION", __MSTR(LIBUV_VERSION));

    return pyuv;
}

#ifdef PY3
#define INITERROR return NULL

PyObject *
PyInit_pyuv(void)
#else
#define INITERROR return

void
initpyuv(void)
#endif
{
    PyObject *m;
    m = initialize_module();

    if(m == NULL){
        INITERROR;
    }

#ifdef PY3
    return m;
#endif
}


