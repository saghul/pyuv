
#include "pyuv.h"

#include "common.c"
#include "errno.c"
#include "error.c"
#include "loop.c"
#include "handle.c"
#include "request.c"
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
#include "poll.c"
#include "fs.c"
#include "process.c"
#include "dns.c"
#include "util.c"
#include "thread.c"


#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv._cpyuv",          /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    NULL,                   /*m_methods*/
};
#endif

/* borrowed from pyev */
#ifdef PYUV_WINDOWS
static int
pyuv__setmaxstdio(void)
{
    if (_setmaxstdio(PYUV_MAXSTDIO) != PYUV_MAXSTDIO) {
        if (errno) {
            PyErr_SetFromErrno(PyExc_WindowsError);
        }
        else {
            PyErr_SetString(PyExc_WindowsError, "_setmaxstdio failed");
        }
        return -1;
    }
    return 0;
}
#endif


/* Module */
PyObject*
init_pyuv(void)
{
    /* Modules */
    PyObject *pyuv;
    PyObject *errno_module;
    PyObject *error_module;
    PyObject *fs_module;
    PyObject *dns_module;
    PyObject *util_module;
    PyObject *thread_module;

    /* Initialize GIL */
    PyEval_InitThreads();

#ifdef PYUV_WINDOWS
    if (pyuv__setmaxstdio()) {
        return NULL;
    }
#endif

    /* Main module */
#ifdef PYUV_PYTHON3
    pyuv = PyModule_Create(&pyuv_module);
#else
    pyuv = Py_InitModule("pyuv._cpyuv", NULL);
#endif

    /* Errno module */
    errno_module = init_errno();
    if (errno_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "errno", errno_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_errno_module.m_name, errno_module);
    Py_DECREF(errno_module);
#endif

    /* Error module */
    error_module = init_error();
    if (error_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "error", error_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_error_module.m_name, error_module);
    Py_DECREF(error_module);
#endif

    /* FS module */
    fs_module = init_fs();
    if (fs_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "fs", fs_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_fs_module.m_name, fs_module);
    Py_DECREF(fs_module);
#endif

    /* DNS module */
    dns_module = init_dns();
    if (dns_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "dns", dns_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_dns_module.m_name, dns_module);
    Py_DECREF(dns_module);
#endif

    /* Util module */
    util_module = init_util();
    if (util_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "util", util_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_util_module.m_name, util_module);
    Py_DECREF(util_module);
#endif

    /* Thread module */
    thread_module = init_thread();
    if (thread_module == NULL) {
        goto fail;
    }
    PyUVModule_AddObject(pyuv, "thread", thread_module);
#ifdef PYUV_PYTHON3
    PyDict_SetItemString(PyImport_GetModuleDict(), pyuv_thread_module.m_name, thread_module);
    Py_DECREF(thread_module);
#endif

    /* Types */
    AsyncType.tp_base = &HandleType;
    TimerType.tp_base = &HandleType;
    PrepareType.tp_base = &HandleType;
    IdleType.tp_base = &HandleType;
    CheckType.tp_base = &HandleType;
    SignalType.tp_base = &HandleType;
    UDPType.tp_base = &HandleType;
    PollType.tp_base = &HandleType;
    ProcessType.tp_base = &HandleType;

    StreamType.tp_base = &HandleType;
    TCPType.tp_base = &StreamType;
    PipeType.tp_base = &StreamType;
    TTYType.tp_base = &StreamType;

    GAIRequestType.tp_base = &RequestType;
    if (PyType_Ready(&GAIRequestType) < 0) {
        return NULL;
    }
    GNIRequestType.tp_base = &RequestType;
    if (PyType_Ready(&GNIRequestType) < 0) {
        return NULL;
    }
    WorkRequestType.tp_base = &RequestType;
    if (PyType_Ready(&WorkRequestType) < 0) {
        return NULL;
    }
    FSRequestType.tp_base = &RequestType;
    if (PyType_Ready(&FSRequestType) < 0) {
        return NULL;
    }

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
    PyUVModule_AddType(pyuv, "Poll", &PollType);
    PyUVModule_AddType(pyuv, "StdIO", &StdIOType);
    PyUVModule_AddType(pyuv, "Process", &ProcessType);

    /* Handle and Stream base classes */
    PyUVModule_AddType(pyuv, "Handle", &HandleType);
    PyUVModule_AddType(pyuv, "Stream", &StreamType);

    /* Loop.run modes */
    PyModule_AddIntMacro(pyuv, UV_RUN_DEFAULT);
    PyModule_AddIntMacro(pyuv, UV_RUN_ONCE);
    PyModule_AddIntMacro(pyuv, UV_RUN_NOWAIT);

    /* UDP constants */
    PyModule_AddIntMacro(pyuv, UV_JOIN_GROUP);
    PyModule_AddIntMacro(pyuv, UV_LEAVE_GROUP);
    PyModule_AddIntMacro(pyuv, UV_UDP_PARTIAL);
    PyModule_AddIntMacro(pyuv, UV_UDP_IPV6ONLY);
    PyModule_AddIntMacro(pyuv, UV_UDP_REUSEADDR);

    /* TCP constants */
    PyModule_AddIntMacro(pyuv, UV_TCP_IPV6ONLY);

    /* Process constants */
    PyModule_AddIntMacro(pyuv, UV_PROCESS_SETUID);
    PyModule_AddIntMacro(pyuv, UV_PROCESS_SETGID);
    PyModule_AddIntMacro(pyuv, UV_PROCESS_DETACHED);
    PyModule_AddIntMacro(pyuv, UV_PROCESS_WINDOWS_HIDE);
    PyModule_AddIntMacro(pyuv, UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS);
    PyModule_AddIntMacro(pyuv, UV_IGNORE);
    PyModule_AddIntMacro(pyuv, UV_CREATE_PIPE);
    PyModule_AddIntMacro(pyuv, UV_READABLE_PIPE);
    PyModule_AddIntMacro(pyuv, UV_WRITABLE_PIPE);
    PyModule_AddIntMacro(pyuv, UV_INHERIT_FD);
    PyModule_AddIntMacro(pyuv, UV_INHERIT_STREAM);

    /* Poll constants */
    PyModule_AddIntMacro(pyuv, UV_READABLE);
    PyModule_AddIntMacro(pyuv, UV_WRITABLE);

    /* Handle types */
    PyModule_AddIntMacro(pyuv, UV_UNKNOWN_HANDLE);
#define XX(uc, lc) PyModule_AddIntMacro(pyuv, UV_##uc);
    UV_HANDLE_TYPE_MAP(XX)
#undef XX

    /* libuv version */
    PyModule_AddStringConstant(pyuv, "LIBUV_VERSION", uv_version_string());

    return pyuv;

fail:
#ifdef PYUV_PYTHON3
    Py_DECREF(pyuv);
#endif
    return NULL;

}

#ifdef PYUV_PYTHON3
PyMODINIT_FUNC
PyInit__cpyuv(void)
{
    return init_pyuv();
}
#else
PyMODINIT_FUNC
init_cpyuv(void)
{
    init_pyuv();
}
#endif


