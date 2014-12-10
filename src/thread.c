
static PyObject *
Barrier_func_wait(Barrier *self)
{
    int r;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    r = uv_barrier_wait(&self->uv_barrier);
    Py_END_ALLOW_THREADS

    return PyBool_FromLong((long)r);
}


static int
Barrier_tp_init(Barrier *self, PyObject *args, PyObject *kwargs)
{
    unsigned int count;

    UNUSED_ARG(kwargs);

    RAISE_IF_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "I:__init__", &count)) {
        return -1;
    }

    if (uv_barrier_init(&self->uv_barrier, count)) {
        PyErr_SetString(PyExc_ThreadError, "Error initializing Barrier");
        return -1;
    }

    self->initialized = True;
    return 0;
}


static PyObject *
Barrier_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Barrier *self;

    self = (Barrier *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static void
Barrier_tp_dealloc(Barrier *self)
{
    if (self->initialized) {
        uv_barrier_destroy(&self->uv_barrier);
    }
    Py_TYPE(self)->tp_free(self);
}


static PyMethodDef
Barrier_tp_methods[] = {
    { "wait", (PyCFunction)Barrier_func_wait, METH_NOARGS, "Synchronize participating threads at the barrier" },
    { NULL }
};


static PyTypeObject BarrierType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.thread.Barrier",                                   /*tp_name*/
    sizeof(Barrier),                                                /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Barrier_tp_dealloc,                                 /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Barrier_tp_methods,                                             /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Barrier_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    Barrier_tp_new,                                                 /*tp_new*/
};


static PyObject *
Mutex_func_lock(Mutex *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_mutex_lock(&self->uv_mutex);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Mutex_func_unlock(Mutex *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_mutex_unlock(&self->uv_mutex);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Mutex_func_trylock(Mutex *self)
{
    int r;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    r = uv_mutex_trylock(&self->uv_mutex);
    Py_END_ALLOW_THREADS

    return PyBool_FromLong((long)(r == 0));
}


static int
Mutex_tp_init(Mutex *self, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(args);
    UNUSED_ARG(kwargs);

    RAISE_IF_INITIALIZED(self, -1);

    if (uv_mutex_init(&self->uv_mutex)) {
        PyErr_SetString(PyExc_ThreadError, "Error initializing Mutex");
        return -1;
    }

    self->initialized = True;
    return 0;
}


static PyObject *
Mutex_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Mutex *self;

    self = (Mutex *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static void
Mutex_tp_dealloc(Mutex *self)
{
    if (self->initialized) {
        uv_mutex_destroy(&self->uv_mutex);
    }
    Py_TYPE(self)->tp_free(self);
}


static PyMethodDef
Mutex_tp_methods[] = {
    { "lock", (PyCFunction)Mutex_func_lock, METH_NOARGS, "Lock the mutex object" },
    { "unlock", (PyCFunction)Mutex_func_unlock, METH_NOARGS, "Unlock the mutex object" },
    { "trylock", (PyCFunction)Mutex_func_trylock, METH_NOARGS, "Try to lock the mutex, return True if the lock was acquired, False otherwise" },
    { "__enter__", (PyCFunction)Mutex_func_lock, METH_NOARGS, "" },
    { "__exit__", (PyCFunction)Mutex_func_unlock, METH_VARARGS, "" },
    { NULL }
};


static PyTypeObject MutexType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.thread.Mutex",                                     /*tp_name*/
    sizeof(Mutex),                                                  /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Mutex_tp_dealloc,                                   /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Mutex_tp_methods,                                               /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Mutex_tp_init,                                        /*tp_init*/
    0,                                                              /*tp_alloc*/
    Mutex_tp_new,                                                   /*tp_new*/
};


static PyObject *
RWLock_func_rdlock(RWLock *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_rwlock_rdlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
RWLock_func_rdunlock(RWLock *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_rwlock_rdunlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
RWLock_func_tryrdlock(RWLock *self)
{
    int r;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    r = uv_rwlock_tryrdlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    return PyBool_FromLong((long)(r == 0));
}


static PyObject *
RWLock_func_wrlock(RWLock *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_rwlock_wrlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
RWLock_func_wrunlock(RWLock *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_rwlock_wrunlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
RWLock_func_trywrlock(RWLock *self)
{
    int r;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    r = uv_rwlock_trywrlock(&self->uv_rwlock);
    Py_END_ALLOW_THREADS

    return PyBool_FromLong((long)(r == 0));
}


static int
RWLock_tp_init(RWLock *self, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(args);
    UNUSED_ARG(kwargs);

    RAISE_IF_INITIALIZED(self, -1);

    if (uv_rwlock_init(&self->uv_rwlock)) {
        PyErr_SetString(PyExc_ThreadError, "Error initializing RWLock");
        return -1;
    }

    self->initialized = True;
    return 0;
}


static PyObject *
RWLock_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    RWLock *self;

    self = (RWLock *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static void
RWLock_tp_dealloc(RWLock *self)
{
    if (self->initialized) {
        uv_rwlock_destroy(&self->uv_rwlock);
    }
    Py_TYPE(self)->tp_free(self);
}


static PyMethodDef
RWLock_tp_methods[] = {
    { "rdlock", (PyCFunction)RWLock_func_rdlock, METH_NOARGS, "Lock the rwlock for reading" },
    { "rdunlock", (PyCFunction)RWLock_func_rdunlock, METH_NOARGS, "Unlock the rwlock for reading" },
    { "tryrdlock", (PyCFunction)RWLock_func_tryrdlock, METH_NOARGS, "Try to lock the rwlock for reading" },
    { "wrlock", (PyCFunction)RWLock_func_wrlock, METH_NOARGS, "Lock the rwlock for writing" },
    { "wrunlock", (PyCFunction)RWLock_func_wrunlock, METH_NOARGS, "Unlock the rwlock for writing" },
    { "trywrlock", (PyCFunction)RWLock_func_trywrlock, METH_NOARGS, "Try to lock the rwlock for writing" },
    { NULL }
};


static PyTypeObject RWLockType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.thread.RWLock",                                    /*tp_name*/
    sizeof(RWLock),                                                 /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)RWLock_tp_dealloc,                                  /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    RWLock_tp_methods,                                              /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)RWLock_tp_init,                                       /*tp_init*/
    0,                                                              /*tp_alloc*/
    RWLock_tp_new,                                                  /*tp_new*/
};


static PyObject *
Condition_func_signal(Condition *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_cond_signal(&self->uv_condition);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Condition_func_broadcast(Condition *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_cond_broadcast(&self->uv_condition);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Condition_func_wait(Condition *self, PyObject *args)
{
    Mutex *pymutex;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    if (!PyArg_ParseTuple(args, "O!:wait", &MutexType, &pymutex)) {
        return NULL;
    }

    Py_INCREF(pymutex);

    Py_BEGIN_ALLOW_THREADS
    uv_cond_wait(&self->uv_condition, &pymutex->uv_mutex);
    Py_END_ALLOW_THREADS

    Py_DECREF(pymutex);
    Py_RETURN_NONE;
}


static PyObject *
Condition_func_timedwait(Condition *self, PyObject *args)
{
    int r;
    double timeout;
    Mutex *pymutex;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    if (!PyArg_ParseTuple(args, "O!d:timedwait", &MutexType, &pymutex, &timeout)) {
        return NULL;
    }

    Py_INCREF(pymutex);

    Py_BEGIN_ALLOW_THREADS
    r = uv_cond_timedwait(&self->uv_condition, &pymutex->uv_mutex, (uint64_t)(timeout*1e9));
    Py_END_ALLOW_THREADS

    Py_DECREF(pymutex);
    return PyBool_FromLong((long)(r == 0));
}


static int
Condition_tp_init(Condition *self, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(args);
    UNUSED_ARG(kwargs);

    RAISE_IF_INITIALIZED(self, -1);

    if (uv_cond_init(&self->uv_condition)) {
        PyErr_SetString(PyExc_ThreadError, "Error initializing Condition");
        return -1;
    }

    self->initialized = True;
    return 0;
}


static PyObject *
Condition_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Condition *self;

    self = (Condition *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static void
Condition_tp_dealloc(Condition *self)
{
    if (self->initialized) {
        uv_cond_destroy(&self->uv_condition);
    }
    Py_TYPE(self)->tp_free(self);
}


static PyMethodDef
Condition_tp_methods[] = {
    { "signal", (PyCFunction)Condition_func_signal, METH_NOARGS, "Unblock at least one of the threads that are blocked on this condition" },
    { "broadcast", (PyCFunction)Condition_func_broadcast, METH_NOARGS, "Unblock all threads currently blocked on this condition" },
    { "wait", (PyCFunction)Condition_func_wait, METH_VARARGS, "Block on this condition variable, mutex needs to be locked" },
    { "timedwait", (PyCFunction)Condition_func_timedwait, METH_VARARGS, "Block on this condition variable for the given amount of time, mutex needs to be locked" },
    { NULL }
};


static PyTypeObject ConditionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.thread.Condition",                                 /*tp_name*/
    sizeof(Condition),                                              /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Condition_tp_dealloc,                               /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Condition_tp_methods,                                           /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Condition_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    Condition_tp_new,                                               /*tp_new*/
};


static PyObject *
Semaphore_func_post(Semaphore *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_sem_post(&self->uv_semaphore);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Semaphore_func_wait(Semaphore *self)
{
    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    uv_sem_wait(&self->uv_semaphore);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


static PyObject *
Semaphore_func_trywait(Semaphore *self)
{
    int r;

    RAISE_IF_NOT_INITIALIZED(self, NULL);

    Py_BEGIN_ALLOW_THREADS
    r = uv_sem_trywait(&self->uv_semaphore);
    Py_END_ALLOW_THREADS

    return PyBool_FromLong((long)(r == 0));
}


static int
Semaphore_tp_init(Semaphore *self, PyObject *args, PyObject *kwargs)
{
    unsigned int value = 1;

    UNUSED_ARG(kwargs);

    RAISE_IF_INITIALIZED(self, -1);

    if (!PyArg_ParseTuple(args, "|I:__init__", &value)) {
        return -1;
    }

    if (uv_sem_init(&self->uv_semaphore, value)) {
        PyErr_SetString(PyExc_ThreadError, "Error initializing Semaphore");
        return -1;
    }

    self->initialized = True;
    return 0;
}


static PyObject *
Semaphore_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Semaphore *self;

    self = (Semaphore *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->initialized = False;
    return (PyObject *)self;
}


static void
Semaphore_tp_dealloc(Semaphore *self)
{
    if (self->initialized) {
        uv_sem_destroy(&self->uv_semaphore);
    }
    Py_TYPE(self)->tp_free(self);
}


static PyMethodDef
Semaphore_tp_methods[] = {
    { "post", (PyCFunction)Semaphore_func_post, METH_NOARGS, "Increment (unlock) the semaphore" },
    { "wait", (PyCFunction)Semaphore_func_wait, METH_NOARGS, "Decrement (lock) the semaphore" },
    { "trywait", (PyCFunction)Semaphore_func_trywait, METH_NOARGS, "Try to decrement (lock) the semaphore" },
    { "__enter__", (PyCFunction)Semaphore_func_wait, METH_NOARGS, "" },
    { "__exit__", (PyCFunction)Semaphore_func_post, METH_VARARGS, "" },
    { NULL }
};


static PyTypeObject SemaphoreType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv._cpyuv.thread.Semaphore",                                 /*tp_name*/
    sizeof(Semaphore),                                              /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)Semaphore_tp_dealloc,                               /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /*tp_flags*/
    0,                                                              /*tp_doc*/
    0,                                                              /*tp_traverse*/
    0,                                                              /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    Semaphore_tp_methods,                                           /*tp_methods*/
    0,                                                              /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)Semaphore_tp_init,                                    /*tp_init*/
    0,                                                              /*tp_alloc*/
    Semaphore_tp_new,                                               /*tp_new*/
};


#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_thread_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv._cpyuv.thread",   /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    NULL,                   /*m_methods*/
};
#endif

PyObject *
init_thread(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_thread_module);
#else
    module = Py_InitModule("pyuv._cpyuv.thread", NULL);
#endif
    if (module == NULL) {
        return NULL;
    }

    PyUVModule_AddType(module, "Barrier", &BarrierType);
    PyUVModule_AddType(module, "Condition", &ConditionType);
    PyUVModule_AddType(module, "Mutex", &MutexType);
    PyUVModule_AddType(module, "RWLock", &RWLockType);
    PyUVModule_AddType(module, "Semaphore", &SemaphoreType);

    return module;
}

