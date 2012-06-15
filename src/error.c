
#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_error_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv.error",           /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    NULL,                   /*m_methods*/
};
#endif

PyObject *
init_error(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_error_module);
#else
    module = Py_InitModule("pyuv.error", NULL);
#endif
    if (module == NULL) {
        return NULL;
    }

    PyExc_UVError = PyErr_NewException("pyuv.error.UVError", NULL, NULL);
    PyExc_HandleError = PyErr_NewException("pyuv.error.HandleError", PyExc_UVError, NULL);
    PyExc_HandleClosedError = PyErr_NewException("pyuv.error.HandleClosedError", PyExc_HandleError, NULL);
    PyExc_AsyncError = PyErr_NewException("pyuv.error.AsyncError", PyExc_HandleError, NULL);
    PyExc_TimerError = PyErr_NewException("pyuv.error.TimerError", PyExc_HandleError, NULL);
    PyExc_PrepareError = PyErr_NewException("pyuv.error.PrepareError", PyExc_HandleError, NULL);
    PyExc_IdleError = PyErr_NewException("pyuv.error.IdleError", PyExc_HandleError, NULL);
    PyExc_CheckError = PyErr_NewException("pyuv.error.CheckError", PyExc_HandleError, NULL);
    PyExc_SignalError = PyErr_NewException("pyuv.error.SignalError", PyExc_HandleError, NULL);
    PyExc_StreamError = PyErr_NewException("pyuv.error.StreamError", PyExc_HandleError, NULL);
    PyExc_TCPError = PyErr_NewException("pyuv.error.TCPError", PyExc_StreamError, NULL);
    PyExc_PipeError = PyErr_NewException("pyuv.error.PipeError", PyExc_StreamError, NULL);
    PyExc_TTYError = PyErr_NewException("pyuv.error.TTYError", PyExc_StreamError, NULL);
    PyExc_UDPError = PyErr_NewException("pyuv.error.UDPError", PyExc_HandleError, NULL);
    PyExc_PollError = PyErr_NewException("pyuv.error.PollError", PyExc_HandleError, NULL);
    PyExc_DNSError = PyErr_NewException("pyuv.error.DNSError", NULL, NULL);
    PyExc_ThreadPoolError = PyErr_NewException("pyuv.error.ThreadPoolError", PyExc_UVError, NULL);
    PyExc_FSError = PyErr_NewException("pyuv.error.FSError", PyExc_UVError, NULL);
    PyExc_FSEventError = PyErr_NewException("pyuv.error.FSEventError", PyExc_HandleError, NULL);
    PyExc_FSPollError = PyErr_NewException("pyuv.error.FSPollError", PyExc_HandleError, NULL);
    PyExc_ProcessError = PyErr_NewException("pyuv.error.ProcessError", PyExc_HandleError, NULL);

    PyUVModule_AddType(module, "UVError", (PyTypeObject *)PyExc_UVError);
    PyUVModule_AddType(module, "HandleError", (PyTypeObject *)PyExc_HandleError);
    PyUVModule_AddType(module, "AsyncError", (PyTypeObject *)PyExc_AsyncError);
    PyUVModule_AddType(module, "TimerError", (PyTypeObject *)PyExc_TimerError);
    PyUVModule_AddType(module, "PrepareError", (PyTypeObject *)PyExc_PrepareError);
    PyUVModule_AddType(module, "IdleError", (PyTypeObject *)PyExc_IdleError);
    PyUVModule_AddType(module, "CheckError", (PyTypeObject *)PyExc_CheckError);
    PyUVModule_AddType(module, "SignalError", (PyTypeObject *)PyExc_SignalError);
    PyUVModule_AddType(module, "StreamError", (PyTypeObject *)PyExc_StreamError);
    PyUVModule_AddType(module, "TCPError", (PyTypeObject *)PyExc_TCPError);
    PyUVModule_AddType(module, "PipeError", (PyTypeObject *)PyExc_PipeError);
    PyUVModule_AddType(module, "TTYError", (PyTypeObject *)PyExc_TTYError);
    PyUVModule_AddType(module, "UDPError", (PyTypeObject *)PyExc_UDPError);
    PyUVModule_AddType(module, "PollError", (PyTypeObject *)PyExc_PollError);
    PyUVModule_AddType(module, "DNSError", (PyTypeObject *)PyExc_DNSError);
    PyUVModule_AddType(module, "ThreadPoolError", (PyTypeObject *)PyExc_ThreadPoolError);
    PyUVModule_AddType(module, "FSError", (PyTypeObject *)PyExc_FSError);
    PyUVModule_AddType(module, "FSEventError", (PyTypeObject *)PyExc_FSEventError);
    PyUVModule_AddType(module, "FSPollError", (PyTypeObject *)PyExc_FSPollError);
    PyUVModule_AddType(module, "ProcessError", (PyTypeObject *)PyExc_ProcessError);

    return module;
}

