
/*
 * NOTE: Since libuv uses pipes to communicate processes and pipes
 * are buffered, things will not go so well. Replacing the comunication
 * mechanism with a pty could do the job, perhaps. References:
 *
 * http://stackoverflow.com/questions/4057985/disabling-stdout-buffering-of-a-forked-process
 * http://stackoverflow.com/questions/2055918/forcing-a-program-to-flush-its-standard-output-when-redirected
 */

static PyObject* PyExc_ProcessError;


static void
on_process_exit(uv_process_t *process, int exit_status, int term_signal)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Process *self;
    PyObject *result;

    ASSERT(process);
    self = (Process *)process->data;
    ASSERT(self);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (self->on_exit_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_exit_cb, self, PyInt_FromLong(exit_status), PyInt_FromLong(term_signal), NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_exit_cb);
        }
        Py_XDECREF(result);
    }

    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_process_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Process *self;
    PyObject *result;

    ASSERT(handle);
    self = (Process *)handle->data;
    ASSERT(self);

    if (self->on_close_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_close_cb, self, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_close_cb);
        }
        Py_XDECREF(result);
    }

    handle->data = NULL;
    PyMem_Free(handle);

    /* Refcount was increased in func_close */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static void
on_process_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static PyObject *
Process_func_spawn(Process *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *cwd, *cwd2, *file, *file2, *arg_str, *tmp_str, *key_str, *value_str;
    char **ptr, **process_args, **process_env;
    Py_ssize_t i, n, pos;
    PyObject *key, *value, *item, *tmp, *callback, *arguments, *env, *stdin_pipe, *stdout_pipe, *stderr_pipe;
    uv_process_t *uv_process;
    uv_process_options_t options;

    static char *kwlist[] = {"file", "exit_callback", "args", "env", "cwd", "stdin", "stdout", "stderr", NULL};

    cwd = NULL;
    ptr = process_args = process_env = NULL;
    tmp = arguments = env = NULL;
    stdin_pipe = stdout_pipe = stderr_pipe = Py_None;

    if (self->uv_handle) {
        PyErr_SetString(PyExc_ProcessError, "Process already spawned");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OOO!sO!O!O!:__init__", kwlist, &file, &callback, &arguments, &PyDict_Type, &env, &cwd, &PipeType, &stdin_pipe, &PipeType, &stdout_pipe, &PipeType, &stderr_pipe)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (arguments && !PySequence_Check(arguments)) {
        PyErr_SetString(PyExc_TypeError, "only iterable objects are supported");
        return NULL;
    }

    tmp = (PyObject *)self->on_exit_cb;
    Py_INCREF(callback);
    self->on_exit_cb = callback;
    Py_XDECREF(tmp);

    tmp = (PyObject *)self->stdin_pipe;
    Py_INCREF(stdin_pipe);
    self->stdin_pipe = stdin_pipe;
    Py_XDECREF(tmp);

    tmp = (PyObject *)self->stdout_pipe;
    Py_INCREF(stdout_pipe);
    self->stdout_pipe = stdout_pipe;
    Py_XDECREF(tmp);

    tmp = (PyObject *)self->stderr_pipe;
    Py_INCREF(stderr_pipe);
    self->stderr_pipe = stderr_pipe;
    Py_XDECREF(tmp);

    memset(&options, 0, sizeof(uv_process_options_t));

    options.exit_cb = on_process_exit;

    file2 = (char *) PyMem_Malloc(strlen(file) + 1);
    if (!file2) {
        PyErr_NoMemory();
        goto error;
    }
    strcpy(file2, file);
    options.file = file2;

    if (arguments) {
        n = PySequence_Length(arguments);
        process_args = (char **)PyMem_Malloc(sizeof(char *) * (n + 2));
        if (!process_args) {
            PyErr_NoMemory();
            goto error;
        }
        process_args[0] = file2;
        for (i = 0;i < n; i++) {
            item = PySequence_GetItem(arguments, i);
            if (!item || !PyString_Check(item))
                continue;
            arg_str = PyString_AsString(item);
            tmp_str = (char *) PyMem_Malloc(strlen(arg_str) + 1);
            if (!tmp_str)
                continue;
            strcpy(tmp_str, arg_str);
            process_args[i+1] = tmp_str;
        }
        process_args[i+1] = NULL;
    } else {
        process_args = (char **)PyMem_Malloc(sizeof(char *) * 2);
        if (!process_args) {
            PyErr_NoMemory();
            goto error;
        }
        process_args[0] = file2;
        process_args[1] = NULL;
    }
    options.args = process_args;

    if (cwd) {
        cwd2 = (char *) PyMem_Malloc(strlen(cwd) + 1);
        if (!cwd2) {
            PyErr_NoMemory();
            goto error;
        }
        strcpy(cwd2, cwd);
        options.cwd = cwd2;
    }

    if (env) {
        n = PyDict_Size(env);
        if (n > 0) {
            process_env = (char **)PyMem_Malloc(sizeof(char *) * (n + 1));
            i = 0;
            pos = 0;
            while (PyDict_Next(env, &pos, &key, &value)) {
                if (!PyString_Check(key) || !PyString_Check(value))
                    continue;
                key_str = PyString_AsString(key);
                value_str = PyString_AsString(value);
                tmp_str = (char *) PyMem_Malloc(strlen(key_str) + 1 + strlen(value_str) + 1);
                if (!tmp_str)
                    continue;
                sprintf(tmp_str, "%.*s=%.*s",
                        (int)strlen(key_str), key_str,
                        (int)strlen(value_str), value_str);
                process_env[i] = tmp_str;
                i++;
            }
            process_env[i] = NULL;
        }
    }
    options.env = process_env;

    if (stdin_pipe != Py_None)
        options.stdin_stream = (uv_pipe_t *)((IOStream *)stdin_pipe)->uv_handle;

    if (stdout_pipe != Py_None)
        options.stdout_stream = (uv_pipe_t *)((IOStream *)stdout_pipe)->uv_handle;

    if (stderr_pipe != Py_None)
        options.stderr_stream = (uv_pipe_t *)((IOStream *)stderr_pipe)->uv_handle;

    uv_process = PyMem_Malloc(sizeof(uv_process_t));
    if (!uv_process) {
        PyErr_NoMemory();
        goto error;
    }
    uv_process->data = (void *)self;
    self->uv_handle = uv_process;

    r = uv_spawn(UV_LOOP(self), self->uv_handle, options);

    if (options.args) {
        for (ptr = options.args; *ptr != NULL; ptr++) {
            PyMem_Free(*ptr);
        }
        PyMem_Free(options.args);
    }
    if (options.cwd) {
        PyMem_Free(options.cwd);
    }
    if (options.env) {
        for (ptr = options.env; *ptr != NULL; ptr++) {
            PyMem_Free(*ptr);
        }
        PyMem_Free(options.env);
    }

    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_ProcessError);
        PyMem_Free(uv_process);
        self->uv_handle = NULL;
        goto error;
    }

    Py_RETURN_NONE;

error:
    Py_DECREF(self->on_exit_cb);
    Py_DECREF(self->stdin_pipe);
    Py_DECREF(self->stdout_pipe);
    Py_DECREF(self->stderr_pipe);
    return NULL;

}


static PyObject *
Process_func_kill(Process *self, PyObject *args)
{
    int signum, r;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_ProcessError, "Process wasn't spawned yet");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:kill", &signum)) {
        return NULL;
    }

    r = uv_process_kill(self->uv_handle, signum);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_ProcessError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Process_func_close(Process *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_ProcessError, "Process wasn't spawned yet");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "|O:close", &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return NULL;
    }

    Py_INCREF(callback);
    self->on_close_cb = callback;

    /* Increase refcount so that object is not removed before the callback is called */
    Py_INCREF(self);

    uv_close((uv_handle_t *)self->uv_handle, on_process_close);
    self->uv_handle = NULL;

    Py_RETURN_NONE;
}


static PyObject *
Process_pid_get(Process *self, void *closure)
{
    UNUSED_ARG(closure);

    if (!self->uv_handle) {
        Py_RETURN_NONE;
    }
    return PyInt_FromLong((long)self->uv_handle->pid);
}


static int
Process_tp_init(Process *self, PyObject *args, PyObject *kwargs)
{
    PyObject *tmp = NULL;
    Loop *loop;

    UNUSED_ARG(kwargs);

    if (self->uv_handle) {
        PyErr_SetString(PyExc_ProcessError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    return 0;
}


static PyObject *
Process_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Process *self = (Process *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
Process_tp_traverse(Process *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->on_exit_cb);
    Py_VISIT(self->stdin_pipe);
    Py_VISIT(self->stdout_pipe);
    Py_VISIT(self->stderr_pipe);
    Py_VISIT(self->loop);
    return 0;
}


static int
Process_tp_clear(Process *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->on_exit_cb);
    Py_CLEAR(self->stdin_pipe);
    Py_CLEAR(self->stdout_pipe);
    Py_CLEAR(self->stderr_pipe);
    Py_CLEAR(self->loop);
    return 0;
}


static void
Process_tp_dealloc(Process *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_process_dealloc_close);
        self->uv_handle = NULL;
    }
    Process_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
Process_tp_methods[] = {
    { "spawn", (PyCFunction)Process_func_spawn, METH_VARARGS|METH_KEYWORDS, "Spawn the child process." },
    { "kill", (PyCFunction)Process_func_kill, METH_VARARGS, "Kill this process with the specified signal number." },
    { "close", (PyCFunction)Process_func_close, METH_VARARGS, "Close the Process." },
    { NULL }
};


static PyMemberDef Process_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Process, loop), READONLY, "Loop where this Process is running on."},
    {"data", T_OBJECT, offsetof(Process, data), 0, "Arbitrary data."},
    {NULL}
};


static PyGetSetDef Process_tp_getsets[] = {
    {"pid", (getter)Process_pid_get, NULL, "Process ID", NULL},
    {NULL}
};


static PyTypeObject ProcessType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.Process",                                                 /*tp_name*/
    sizeof(Process),                                                /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Process_tp_dealloc,                                 /*tp_dealloc*/
    0,                                                              /*tp_print*/
    0,                                                              /*tp_getattr*/
    0,                                                              /*tp_setattr*/
    0,                                                              /*tp_compare*/
    0,                                                              /*tp_repr*/
    0,                                                              /*tp_as_number*/
    0,                                                              /*tp_as_sequence*/
    0,                                                              /*tp_as_mapping*/
    0,                                                              /*tp_hash */
    0,                                                              /*tp_call*/
    0,                                                              /*tp_str*/
    0,                                                              /*tp_getattro*/
    0,                                                              /*tp_setattro*/
    0,                                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    0,                                                              /*tp_doc*/
    (traverseproc)Process_tp_traverse,                              /*tp_traverse*/
    (inquiry)Process_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Process_tp_methods,                                             /*tp_methods*/
    Process_tp_members,                                             /*tp_members*/
    Process_tp_getsets,                                             /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Process_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    Process_tp_new,                                                 /*tp_new*/
};


