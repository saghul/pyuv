
/*
 * NOTE: Since libuv uses pipes to communicate processes and pipes
 * are buffered, things will not go so well. Replacing the comunication
 * mechanism with a pty could do the job, perhaps. References:
 *
 * http://stackoverflow.com/questions/4057985/disabling-stdout-buffering-of-a-forked-process
 * http://stackoverflow.com/questions/2055918/forcing-a-program-to-flush-its-standard-output-when-redirected
 */

/* Container for standard IO */
static int
StdIO_tp_init(StdIO *self, PyObject *args, PyObject *kwargs)
{
    int flags = UV_IGNORE;
    int fd = -1;
    PyObject *stream, *tmp;

    static char *kwlist[] = {"stream", "fd", "flags", NULL};

    stream = tmp = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oii:__init__", kwlist, &stream, &fd, &flags)) {
        return -1;
    }

    if (stream != NULL && fd != -1) {
        PyErr_SetString(PyExc_ValueError, "either stream or fd must be specified, but not both");
        return -1;
    }

    if (stream != NULL) {
        if (!PyObject_IsSubclass((PyObject *)stream->ob_type, (PyObject *)&StreamType)) {
            PyErr_SetString(PyExc_TypeError, "Only stream objects are supported");
            return -1;
        }
        if (!((flags & ~(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE | UV_INHERIT_STREAM))==0)) {
            PyErr_SetString(PyExc_ValueError, "invalid flags specified for stream");
            return -1;
        }
    }

    if (fd != -1 && !((flags & ~(UV_INHERIT_FD))==0)) {
        PyErr_SetString(PyExc_ValueError, "invalid flags specified for fd");
        return -1;
    }

    if (stream == NULL && fd == -1 && !((flags & ~(UV_IGNORE))==0)) {
        PyErr_SetString(PyExc_ValueError, "invalid flags specified for ignore");
        return -1;
    }

    tmp = self->stream;
    Py_XINCREF(stream);
    self->stream = stream;
    Py_XDECREF(tmp);

    self->fd = fd;
    self->flags = flags;

    return 0;
}


static PyObject *
StdIO_stream_get(StdIO *self, void *closure)
{
    UNUSED_ARG(closure);
    if (!self->stream) {
        Py_INCREF(Py_None);
        Py_RETURN_NONE;
    } else {
        return self->stream;
    }
}


static PyObject *
StdIO_fd_get(StdIO *self, void *closure)
{
    UNUSED_ARG(closure);
    return PyInt_FromLong((long)self->fd);
}


static PyObject *
StdIO_flags_get(StdIO *self, void *closure)
{
    UNUSED_ARG(closure);
    return PyInt_FromLong((long)self->fd);
}


static PyObject *
StdIO_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    StdIO *self = (StdIO *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
StdIO_tp_traverse(StdIO *self, visitproc visit, void *arg)
{
    Py_VISIT(self->stream);
    return 0;
}


static int
StdIO_tp_clear(StdIO *self)
{
    Py_CLEAR(self->stream);
    return 0;
}


static void
StdIO_tp_dealloc(StdIO *self)
{
    StdIO_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyGetSetDef StdIO_tp_getsets[] = {
    {"stream", (getter)StdIO_stream_get, NULL, "Stream object.", NULL},
    {"fd", (getter)StdIO_fd_get, NULL, "File descriptor.", NULL},
    {"flags", (getter)StdIO_flags_get, NULL, "Flags.", NULL},
    {NULL}
};


static PyTypeObject StdIOType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.StdIO",                                                   /*tp_name*/
    sizeof(StdIO),                                                  /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)StdIO_tp_dealloc,                                   /*tp_dealloc*/
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
    (traverseproc)StdIO_tp_traverse,                                /*tp_traverse*/
    (inquiry)StdIO_tp_clear,                                        /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    0,                                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    StdIO_tp_getsets,                                               /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)StdIO_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    StdIO_tp_new,                                                   /*tp_new*/
};


/* Process handle */

static PyObject* PyExc_ProcessError;


static void
on_process_exit(uv_process_t *process, int exit_status, int term_signal)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Process *self;
    PyObject *result, *py_exit_status, *py_term_signal;

    ASSERT(process);
    self = (Process *)process->data;
    ASSERT(self);

    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    py_exit_status = PyInt_FromLong(exit_status);
    py_term_signal = PyInt_FromLong(term_signal);

    if (self->on_exit_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_exit_cb, self, py_exit_status, py_term_signal, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(self->on_exit_cb);
        }
        Py_XDECREF(result);
        Py_DECREF(py_exit_status);
        Py_DECREF(py_term_signal);
    }

    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
Process_func_spawn(Process *self, PyObject *args, PyObject *kwargs)
{
    int r, flags, len, stdio_count;
    unsigned int uid, gid;
    char *cwd, *cwd2, *file, *file2, *arg_str, *tmp_str, *key_str, *value_str;
    char **ptr, **process_args, **process_env;
    Py_ssize_t i, n, pos;
    PyObject *key, *value, *item, *tmp, *callback, *arguments, *env, *stdio, *ret;
    uv_process_t *uv_process;
    uv_process_options_t options;
    uv_stdio_container_t *stdio_container;

    static char *kwlist[] = {"file", "exit_callback", "args", "env", "cwd", "uid", "gid", "flags", "stdio", NULL};

    cwd = NULL;
    ptr = process_args = process_env = NULL;
    tmp = arguments = env = stdio = NULL;
    stdio_container = NULL;
    flags = uid = gid = stdio_count = 0;

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_ProcessError, "Process already spawned");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|OOO!sIIiO:__init__", kwlist, &file, &callback, &arguments, &PyDict_Type, &env, &cwd, &uid, &gid, &flags, &stdio)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (arguments && !PySequence_Check(arguments)) {
        PyErr_SetString(PyExc_TypeError, "only iterable objects are supported for 'args'");
        return NULL;
    }

    if (stdio && !PySequence_Check(stdio)) {
        PyErr_SetString(PyExc_TypeError, "only iterable objects are supported for 'stdio'");
        return NULL;
    }

    memset(&options, 0, sizeof(uv_process_options_t));

    options.uid = uid;
    options.gid = gid;
    options.flags = flags;
    options.exit_cb = on_process_exit;

    file2 = (char *) PyMem_Malloc(strlen(file) + 1);
    if (!file2) {
        PyErr_NoMemory();
        ret = NULL;
        goto cleanup;
    }
    strcpy(file2, file);
    options.file = file2;

    if (arguments) {
        n = PySequence_Length(arguments);
        process_args = (char **)PyMem_Malloc(sizeof(char *) * (n + 2));
        if (!process_args) {
            PyErr_NoMemory();
            PyMem_Free(file2);
            ret = NULL;
            goto cleanup;
        }
        process_args[0] = file2;
        i = 0;
        while (i < n) {
            item = PySequence_GetItem(arguments, i);
            if (!item || !PyArg_Parse(item, "s;args contains a non-string value", &arg_str)) {
                Py_XDECREF(item);
                ret = NULL;
                goto cleanup;
            }
            tmp_str = (char *) PyMem_Malloc(strlen(arg_str) + 1);
            if (!tmp_str) {
                Py_DECREF(item);
                ret = NULL;
                goto cleanup;
            }
            strcpy(tmp_str, arg_str);
            process_args[i+1] = tmp_str;
            Py_DECREF(item);
            i++;
        }
        process_args[i+1] = NULL;
    } else {
        process_args = (char **)PyMem_Malloc(sizeof(char *) * 2);
        if (!process_args) {
            PyErr_NoMemory();
            PyMem_Free(file2);
            ret = NULL;
            goto cleanup;
        }
        process_args[0] = file2;
        process_args[1] = NULL;
    }
    options.args = process_args;

    if (cwd) {
        cwd2 = (char *) PyMem_Malloc(strlen(cwd) + 1);
        if (!cwd2) {
            PyErr_NoMemory();
            ret = NULL;
            goto cleanup;
        }
        strcpy(cwd2, cwd);
        options.cwd = cwd2;
    }

    if (env) {
        n = PyDict_Size(env);
        if (n > 0) {
            process_env = (char **)PyMem_Malloc(sizeof(char *) * (n + 1));
            if (!process_env) {
                PyErr_NoMemory();
                ret = NULL;
                goto cleanup;
            }
            i = 0;
            pos = 0;
            while (PyDict_Next(env, &pos, &key, &value)) {
                if (!PyArg_Parse(key, "s;env contains a non-string key", &key_str) || !PyArg_Parse(value, "s;env contains a non-string value", &value_str)) {
                    ret = NULL;
                    goto cleanup;
                }
                len = strlen(key_str) + strlen(value_str) + 2;
                tmp_str = (char *) PyMem_Malloc(len);
                if (!tmp_str) {
                    PyErr_NoMemory();
                    ret = NULL;
                    goto cleanup;
                }
                PyOS_snprintf(tmp_str, len, "%s=%s", key_str, value_str);
                process_env[i] = tmp_str;
                i++;
            }
            process_env[i] = NULL;
        }
    }
    options.env = process_env;

    if (stdio) {
        n = PySequence_Length(stdio);
        stdio_container = (uv_stdio_container_t *)PyMem_Malloc(sizeof(uv_stdio_container_t) * n);
        if (!stdio_container) {
            PyErr_NoMemory();
            ret = NULL;
            goto cleanup;
        }
        item = NULL;
        for (i = 0;i < n; i++) {
            item = PySequence_GetItem(stdio, i);
            if (!item || !PyObject_TypeCheck(item, &StdIOType)) {
                Py_XDECREF(item);
                PyErr_SetString(PyExc_TypeError, "a StdIO instance is required");
                ret = NULL;
                goto cleanup;
            }
            stdio_count++;
            stdio_container[i].flags = ((StdIO *)item)->flags;
            if (((StdIO *)item)->flags & (UV_CREATE_PIPE | UV_INHERIT_STREAM)) {
                stdio_container[i].data.stream = (uv_stream_t *)(UV_HANDLE(((StdIO *)item)->stream));
            } else if (((StdIO *)item)->flags & UV_INHERIT_FD) {
                stdio_container[i].data.fd = ((StdIO *)item)->fd;
            }
            Py_DECREF(item);
        }
    }
    options.stdio = stdio_container;
    options.stdio_count = stdio_count;

    uv_process = PyMem_Malloc(sizeof(uv_process_t));
    if (!uv_process) {
        PyErr_NoMemory();
        ret = NULL;
        goto cleanup;
    }
    uv_process->data = (void *)self;
    UV_HANDLE(self) = (uv_handle_t *)uv_process;

    r = uv_spawn(UV_HANDLE_LOOP(self), uv_process, options);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_ProcessError);
        PyMem_Free(uv_process);
        UV_HANDLE(self) = NULL;
        ret = NULL;
        goto cleanup;
    }

    tmp = (PyObject *)self->on_exit_cb;
    Py_INCREF(callback);
    self->on_exit_cb = callback;
    Py_XDECREF(tmp);

    tmp = self->stdio;
    Py_XINCREF(stdio);
    self->stdio = stdio;
    Py_XDECREF(tmp);

    ret = Py_None;

cleanup:
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
    if (options.stdio) {
        PyMem_Free(options.stdio);
    }

    Py_XINCREF(ret);
    return ret;
}


static PyObject *
Process_func_kill(Process *self, PyObject *args)
{
    int signum, r;

    if (UV_HANDLE_CLOSED(self)) {
        PyErr_SetString(PyExc_ProcessError, "Process wasn't spawned yet");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i:kill", &signum)) {
        return NULL;
    }

    r = uv_process_kill((uv_process_t *)UV_HANDLE(self), signum);
    if (r != 0) {
        RAISE_UV_EXCEPTION(UV_HANDLE_LOOP(self), PyExc_ProcessError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Process_pid_get(Process *self, void *closure)
{
    UNUSED_ARG(closure);

    if (UV_HANDLE_CLOSED(self)) {
        Py_RETURN_NONE;
    }
    return PyInt_FromLong((long)((uv_process_t *)UV_HANDLE(self))->pid);
}


static int
Process_tp_init(Process *self, PyObject *args, PyObject *kwargs)
{
    PyObject *tmp = NULL;
    Loop *loop;

    UNUSED_ARG(kwargs);

    if (UV_HANDLE(self)) {
        PyErr_SetString(PyExc_ProcessError, "Object already initialized");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    tmp = (PyObject *)((Handle *)self)->loop;
    Py_INCREF(loop);
    ((Handle *)self)->loop = loop;
    Py_XDECREF(tmp);

    return 0;
}


static PyObject *
Process_func_disable_stdio_inheritance(PyObject *cls)
{
    UNUSED_ARG(cls);
    uv_disable_stdio_inheritance();
    Py_RETURN_NONE;
}


static PyObject *
Process_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Process *self = (Process *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    return (PyObject *)self;
}


static int
Process_tp_traverse(Process *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_exit_cb);
    Py_VISIT(self->stdio);
    HandleType.tp_traverse((PyObject *)self, visit, arg);
    return 0;
}


static int
Process_tp_clear(Process *self)
{
    Py_CLEAR(self->on_exit_cb);
    Py_CLEAR(self->stdio);
    HandleType.tp_clear((PyObject *)self);
    return 0;
}


static PyMethodDef
Process_tp_methods[] = {
    { "spawn", (PyCFunction)Process_func_spawn, METH_VARARGS|METH_KEYWORDS, "Spawn the child process." },
    { "kill", (PyCFunction)Process_func_kill, METH_VARARGS, "Kill this process with the specified signal number." },
    { "disable_stdio_inheritance", (PyCFunction)Process_func_disable_stdio_inheritance, METH_NOARGS|METH_CLASS, "Disables inheritance for file descriptors / handles that this process inherited from its parent." },
    { NULL }
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
    0,                                                              /*tp_dealloc*/
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
    0,                                                              /*tp_members*/
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


