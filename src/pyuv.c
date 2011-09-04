
#include "pyuv.h"

#include "Loop.c"
#include "Timer.c"
#include "TCPConnection.c"
#include "TCPServer.c"

#include "utils.c"


static PyObject* PyExc_UVError;

/* Module */
PyMODINIT_FUNC
initpyuv(void)
{
    PyObject *pyuv = Py_InitModule("pyuv", NULL);

    PyExc_UVError = PyErr_NewException("pyuv.UVError", NULL, NULL);
    PyModule_AddType(pyuv, "UVError", (PyTypeObject *)PyExc_UVError);

    PyModule_AddType(pyuv, "Loop", &LoopType);

    PyModule_AddType(pyuv, "Timer", &TimerType);
    PyExc_TimerError = PyErr_NewException("pyuv.TimerError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TimerError", (PyTypeObject *)PyExc_TimerError);

    PyModule_AddType(pyuv, "TCPConnection", &TCPConnectionType);
    PyExc_TCPConnectionError = PyErr_NewException("pyuv.TCPConnectionError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TCPConnectionError", (PyTypeObject *)PyExc_TCPConnectionError);

    PyModule_AddType(pyuv, "TCPServer", &TCPServerType);
    PyExc_TCPServerError = PyErr_NewException("pyuv.TCPServerError", PyExc_UVError, NULL);
    PyModule_AddType(pyuv, "TCPServerError", (PyTypeObject *)PyExc_TCPServerError);

    /* Module version (the MODULE_VERSION macro is defined by setup.py) */
    PyModule_AddStringConstant(pyuv, "__version__", macro_str(MODULE_VERSION));
}


