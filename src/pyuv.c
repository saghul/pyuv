
#include "pyuv.h"

#include "utils.c"

#include "Loop.c"
#include "Async.c"
#include "Timer.c"
#include "TCPConnection.c"
#include "TCPServer.c"
#include "UDPServer.c"



static PyObject* PyExc_UVError;

/* Module */
PyMODINIT_FUNC
initpyuv(void)
{
    PyObject *pyuv = Py_InitModule("pyuv", NULL);

    PyExc_UVError = PyErr_NewException("pyuv.UVError", NULL, NULL);
    PyModule_AddType(pyuv, "UVError", (PyTypeObject *)PyExc_UVError);

    PyModule_AddType(pyuv, "Loop", &LoopType);

    PyModule_AddType(pyuv, "Async", &AsyncType);
    PyExc_AsyncError = PyErr_NewException("pyuv.AsyncError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "AsyncError", (PyTypeObject *)PyExc_AsyncError);

    PyModule_AddType(pyuv, "Timer", &TimerType);
    PyExc_TimerError = PyErr_NewException("pyuv.TimerError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TimerError", (PyTypeObject *)PyExc_TimerError);

    PyModule_AddType(pyuv, "TCPConnection", &TCPConnectionType);
    PyExc_TCPConnectionError = PyErr_NewException("pyuv.TCPConnectionError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TCPConnectionError", (PyTypeObject *)PyExc_TCPConnectionError);

    PyModule_AddType(pyuv, "TCPServer", &TCPServerType);
    PyExc_TCPServerError = PyErr_NewException("pyuv.TCPServerError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TCPServerError", (PyTypeObject *)PyExc_TCPServerError);

    PyModule_AddType(pyuv, "UDPServer", &UDPServerType);
    PyExc_UDPServerError = PyErr_NewException("pyuv.UDPServerError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "UDPServerError", (PyTypeObject *)PyExc_UDPServerError);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", macro_str(MODULE_VERSION));
}


