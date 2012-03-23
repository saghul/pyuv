
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

#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv",                 /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    NULL,                   /*m_methods*/
};
#endif

/* Module */
PyObject*
init_pyuv(void)
{
    /* Modules */
    PyObject *pyuv;
    PyObject *errno_module;
    PyObject *error_module;
    PyObject *dns;
    PyObject *fs;
    PyObject *util;

    /* Initialize GIL */
    PyEval_InitThreads();

    /* Main module */
#ifdef PYUV_PYTHON3
    pyuv = PyModule_Create(&pyuv_module);
#else
    pyuv = Py_InitModule("pyuv", NULL);
#endif

    /* Errno module */
    errno_module = init_errno();
    if (errno_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "errno", errno_module);

    /* Error module */
    error_module = init_error();
    if (error_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "error", error_module);

    /* DNS module */
    dns = init_dns();
    if (dns == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "dns", dns);

    /* FS module */
    fs = init_fs();
    if (fs == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "fs", fs);

    /* Util module */
    util = init_util();
    if (util == NULL) {
        goto fail;
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

fail:
#ifdef PYUV_PYTHON3
    Py_DECREF(pyuv);
#endif
    return NULL;

}

#ifdef PYUV_PYTHON3
PyMODINIT_FUNC
PyInit_pyuv(void)
{
    return init_pyuv();
}
#else
PyMODINIT_FUNC
initpyuv(void)
{
    init_pyuv();
}
#endif


