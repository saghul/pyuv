
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
        Py_RETURN_NONE;
    } else {
        Py_INCREF(self->stream);
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
    return PyInt_FromLong((long)self->flags);
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

#define RAISE_IF_SPAWNED(obj, retval)                                               \
    do {                                                                            \
        if ((obj)->spawned) {                                                       \
            PyErr_SetString(PyExc_ProcessError, "Process already spawned");         \
            return retval;                                                          \
        }                                                                           \
    } while(0)                                                                      \

#define RAISE_IF_NOT_SPAWNED(obj, retval)                                           \
    do {                                                                            \
        if (!((obj)->spawned)) {                                                    \
            PyErr_SetString(PyExc_ProcessError, "Process was not spawned");         \
            return retval;                                                          \
        }                                                                           \
    } while(0)                                                                      \

static void
on_process_exit(uv_process_t *handle, int64_t exit_status, int term_signal)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Process *self;
    PyObject *result, *py_exit_status, *py_term_signal;

    ASSERT(handle);

    self = PYUV_CONTAINER_OF(handle, Process, process_h);

    py_exit_status = PyInt_FromLong(exit_status);
    py_term_signal = PyInt_FromLong(term_signal);

    if (self->on_exit_cb != Py_None) {
        result = PyObject_CallFunctionObjArgs(self->on_exit_cb, self, py_exit_status, py_term_signal, NULL);
        if (result == NULL) {
            handle_uncaught_exception(HANDLE(self)->loop);
        }
        Py_XDECREF(result);
        Py_DECREF(py_exit_status);
        Py_DECREF(py_term_signal);
    }

    /* Refcount was increased in the spawn function */
    Py_DECREF(self);

    PyGILState_Release(gstate);
}


static PyObject *
Process_func_spawn(Process *self, PyObject *args, PyObject *kwargs)
{
    int err, flags, stdio_count;
    unsigned int uid, gid;
    Py_ssize_t i, n, pos, size;
    PyObject *key, *value, *item, *tmp, *callback, *arguments, *env, *stdio, *ret, *executable, *cwd;
    uv_process_options_t options;
    uv_stdio_container_t *stdio_container;

    static char *kwlist[] = {"args", "executable", "env", "cwd", "uid", "gid", "flags", "stdio", "exit_callback", NULL};

    cwd = executable = Py_None;
    tmp = arguments = env = stdio = NULL;
    stdio_container = NULL;
    flags = uid = gid = stdio_count = 0;

    RAISE_IF_SPAWNED(self, NULL);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO!OIIiOO:__init__", kwlist, &arguments, &executable, &PyDict_Type, &env, &cwd, &uid, &gid, &flags, &stdio, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    if (!PyBytes_Check(arguments) && !PyUnicode_Check(arguments) && !PySequence_Check(arguments)) {
        PyErr_SetString(PyExc_TypeError, "only string or iterable objects are supported for 'args'");
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

    /* process args */

    if (PyBytes_Check(arguments) || PyUnicode_Check(arguments)) {
        options.args = PyMem_Malloc(sizeof *(options.args) * 2);
        if (!options.args) {
            PyErr_NoMemory();
            ret = NULL;
            goto cleanup;
        }
        options.args[0] = pyuv_dup_strobj(arguments);
        if (!options.args[0]) {
            ret = NULL;
            goto cleanup;
        }
        options.args[1] = NULL;
    } else {
        /* it's a sequence object */
        n = PySequence_Length(arguments);
        if (n < 1) {
            PyErr_SetString(PyExc_ValueError, "'args' must contain at least one element");
            return NULL;
        }
        options.args = PyMem_Malloc(sizeof *(options.args) * (n + 1));
        if (!options.args) {
            PyErr_NoMemory();
            ret = NULL;
            goto cleanup;
        }
        for (i = 0; i < n; i++) {
            item = PySequence_GetItem(arguments, i);
            if (!item) {
                options.args[i] = NULL;
                ret = NULL;
                goto cleanup;
            }
            options.args[i] = pyuv_dup_strobj(item);
            if (!options.args[i]) {
                Py_DECREF(item);
                ret = NULL;
                goto cleanup;
            }
            Py_DECREF(item);
        }
        options.args[n] = NULL;
    }

    /* process file */

    if (executable != Py_None) {
        options.file = pyuv_dup_strobj(executable);
        if (!options.file) {
            ret = NULL;
            goto cleanup;
        }
    } else {
        size = strlen(options.args[0]) + 1;
        options.file = PyMem_Malloc(size);
        if (!options.file) {
            PyErr_NoMemory();
            ret = NULL;
            goto cleanup;
        }
        memcpy((void*)options.file, options.args[0], size);
    }

    /* process cwd */
    if (cwd != Py_None) {
        options.cwd = pyuv_dup_strobj(cwd);
        if (!options.cwd) {
            ret = NULL;
            goto cleanup;
        }
    }

    /* process env */

    if (env) {
        char *key_str, *value_str;
        PyObject *key_bytes, *value_bytes;
        n = PyDict_Size(env);
        if (n > 0) {
            options.env = PyMem_Malloc(sizeof *(options.env) * (n + 1));
            if (!options.env) {
                PyErr_NoMemory();
                ret = NULL;
                goto cleanup;
            }
            i = 0;
            pos = 0;
            while (PyDict_Next(env, &pos, &key, &value)) {
                key_bytes = value_bytes = NULL;
                if (!pyuv_PyUnicode_FSConverter(key, &key_bytes)) {
                    options.env[i] = NULL;
                    ret = NULL;
                    goto cleanup;
                }
                if (!pyuv_PyUnicode_FSConverter(value, &value_bytes)) {
                    Py_DECREF(key_bytes);
                    options.env[i] = NULL;
                    ret = NULL;
                    goto cleanup;
                }
                key_str = PyBytes_AS_STRING(key_bytes);
                value_str = PyBytes_AS_STRING(value_bytes);
                size = PyBytes_GET_SIZE(key_bytes) + PyBytes_GET_SIZE(value_bytes) + 2;
                options.env[i] = PyMem_Malloc(size);
                if (!options.env[i]) {
                    options.env[i] = NULL;
                    PyErr_NoMemory();
                    Py_DECREF(key_bytes);
                    Py_DECREF(value_bytes);
                    ret = NULL;
                    goto cleanup;
                }
                PyOS_snprintf(options.env[i], size, "%s=%s", key_str, value_str);
                Py_DECREF(key_bytes);
                Py_DECREF(value_bytes);
                i++;
            }
            options.env[i] = NULL;
        }
    }

    /* process stdio container */

    if (stdio) {
        n = PySequence_Length(stdio);
        stdio_container = PyMem_Malloc(sizeof *stdio_container * n);
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

    err = uv_spawn(UV_HANDLE_LOOP(self), &self->process_h, &options);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_ProcessError);
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

    HANDLE(self)->initialized = True;
    self->spawned = True;

    ret = Py_None;

    /* Increase refcount so that object is not removed before the exit callback is called */
    Py_INCREF(self);

cleanup:
    if (options.args) {
        for (i = 0; options.args[i] != NULL; ++i) {
            PyMem_Free(options.args[i]);
        }
        PyMem_Free(options.args);
    }

    if (options.env) {
        for (i = 0; options.env[i] != NULL; ++i) {
            PyMem_Free(options.env[i]);
        }
        PyMem_Free(options.env);
    }

    PyMem_Free((void*)options.cwd);
    PyMem_Free((void*)options.file);
    PyMem_Free(options.stdio);

    Py_XINCREF(ret);
    return ret;
}


static PyObject *
Process_func_kill(Process *self, PyObject *args)
{
    int signum, err;

    RAISE_IF_HANDLE_NOT_INITIALIZED(self, NULL);
    RAISE_IF_NOT_SPAWNED(self, NULL);
    RAISE_IF_HANDLE_CLOSED(self, PyExc_HandleClosedError, NULL);

    if (!PyArg_ParseTuple(args, "i:kill", &signum)) {
        return NULL;
    }

    err = uv_process_kill(&self->process_h, signum);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_ProcessError);
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Process_pid_get(Process *self, void *closure)
{
    UNUSED_ARG(closure);

    if (!HANDLE(self)->initialized || !self->spawned) {
        Py_RETURN_NONE;
    }
    return PyInt_FromLong((long)self->process_h.pid);
}


static int
Process_tp_init(Process *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;

    UNUSED_ARG(kwargs);

    RAISE_IF_HANDLE_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "O!:__init__", &LoopType, &loop)) {
        return -1;
    }

    initialize_handle(HANDLE(self), loop);

    /* uv_process_t handles are not initialized explicitly, so workaround it. See tp_dealloc for the rest */
    HANDLE(self)->initialized = False;

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
    Process *self;

    self = (Process *)HandleType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }

    self->process_h.data = self;
    UV_HANDLE(self) = (uv_handle_t *)&self->process_h;
    self->spawned = False;

    return (PyObject *)self;
}


static void
Process_tp_dealloc(Process *self)
{
    HANDLE(self)->initialized = self->spawned;
    HandleType.tp_dealloc((PyObject *)self);
}


static int
Process_tp_traverse(Process *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_exit_cb);
    Py_VISIT(self->stdio);
    return HandleType.tp_traverse((PyObject *)self, visit, arg);
}


static int
Process_tp_clear(Process *self)
{
    Py_CLEAR(self->on_exit_cb);
    Py_CLEAR(self->stdio);
    return HandleType.tp_clear((PyObject *)self);
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
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

