
static PyObject* PyExc_UVError;


PyObject *
init_error(void)
{
    PyObject *module;
    module = Py_InitModule("pyuv.error", NULL);
    if (module == NULL) {
        return NULL;
    }

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
    PyExc_TTYError = PyErr_NewException("pyuv.error.TTYError", PyExc_IOStreamError, NULL);
    PyExc_UDPError = PyErr_NewException("pyuv.error.UDPError", PyExc_UVError, NULL);
    PyExc_DNSError = PyErr_NewException("pyuv.error.DNSError", NULL, NULL);
    PyExc_ThreadPoolError = PyErr_NewException("pyuv.error.ThreadPoolError", PyExc_UVError, NULL);
    PyExc_FSError = PyErr_NewException("pyuv.error.FSError", PyExc_UVError, NULL);
    PyExc_FSEventError = PyErr_NewException("pyuv.error.FSEventError", PyExc_UVError, NULL);
    PyExc_ProcessError = PyErr_NewException("pyuv.error.ProcessError", PyExc_UVError, NULL);

    PyUVModule_AddType(module, "UVError", (PyTypeObject *)PyExc_UVError);
    PyUVModule_AddType(module, "AsyncError", (PyTypeObject *)PyExc_AsyncError);
    PyUVModule_AddType(module, "TimerError", (PyTypeObject *)PyExc_TimerError);
    PyUVModule_AddType(module, "PrepareError", (PyTypeObject *)PyExc_PrepareError);
    PyUVModule_AddType(module, "IdleError", (PyTypeObject *)PyExc_IdleError);
    PyUVModule_AddType(module, "CheckError", (PyTypeObject *)PyExc_CheckError);
    PyUVModule_AddType(module, "SignalError", (PyTypeObject *)PyExc_SignalError);
    PyUVModule_AddType(module, "IOStreamError", (PyTypeObject *)PyExc_IOStreamError);
    PyUVModule_AddType(module, "TCPError", (PyTypeObject *)PyExc_TCPError);
    PyUVModule_AddType(module, "PipeError", (PyTypeObject *)PyExc_PipeError);
    PyUVModule_AddType(module, "TTYError", (PyTypeObject *)PyExc_TTYError);
    PyUVModule_AddType(module, "UDPError", (PyTypeObject *)PyExc_UDPError);
    PyUVModule_AddType(module, "DNSError", (PyTypeObject *)PyExc_DNSError);
    PyUVModule_AddType(module, "ThreadPoolError", (PyTypeObject *)PyExc_ThreadPoolError);
    PyUVModule_AddType(module, "FSError", (PyTypeObject *)PyExc_FSError);
    PyUVModule_AddType(module, "FSEventError", (PyTypeObject *)PyExc_FSEventError);
    PyUVModule_AddType(module, "ProcessError", (PyTypeObject *)PyExc_ProcessError);

    return module;
}

