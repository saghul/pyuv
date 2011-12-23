
static PyObject* PyExc_FSError;

typedef struct {
    Loop *loop;
    PyObject *callback;
    PyObject *data;
} fs_req_data_t;


static PyObject *
format_time(time_t sec, unsigned long nsec)
{
    PyObject *ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
    ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
    ival = PyInt_FromLong((long)sec);
#endif
    if (!ival) {
        return PyInt_FromLong(0);
    }
    return ival;
}


static void
stat_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_STAT || req->fs_type == UV_FS_LSTAT || req->fs_type == UV_FS_FSTAT);

    struct stat *st = (struct stat *)(req->ptr);
    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    unsigned long ansec, mnsec, cnsec;
    PyObject *result, *errorno, *stat_data;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    stat_data = PyTuple_New(13);
    if (!stat_data) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(req_data->callback);
        goto end;
    }

    if (req->result != -1) {
        PyTuple_SET_ITEM(stat_data, 0, PyInt_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyTuple_SET_ITEM(stat_data, 1, PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
        PyTuple_SET_ITEM(stat_data, 1, PyInt_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG)
        PyTuple_SET_ITEM(stat_data, 2, PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
        PyTuple_SET_ITEM(stat_data, 2, PyInt_FromLong((long)st->st_dev));
#endif
        PyTuple_SET_ITEM(stat_data, 3, PyInt_FromLong((long)st->st_nlink));
        PyTuple_SET_ITEM(stat_data, 4, PyInt_FromLong((long)st->st_uid));
        PyTuple_SET_ITEM(stat_data, 5, PyInt_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyTuple_SET_ITEM(stat_data, 6, PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
        PyTuple_SET_ITEM(stat_data, 6, PyInt_FromLong(st->st_size));
#endif
#if defined(HAVE_STAT_TV_NSEC)
        ansec = st->st_atim.tv_nsec;
        mnsec = st->st_mtim.tv_nsec;
        cnsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STAT_TV_NSEC2)
        ansec = st->st_atimespec.tv_nsec;
        mnsec = st->st_mtimespec.tv_nsec;
        cnsec = st->st_ctimespec.tv_nsec;
#elif defined(HAVE_STAT_NSEC)
        ansec = st->st_atime_nsec;
        mnsec = st->st_mtime_nsec;
        cnsec = st->st_ctime_nsec;
#else
        ansec = mnsec = cnsec = 0;
#endif
        PyTuple_SET_ITEM(stat_data, 7, format_time(st->st_atime, ansec));
        PyTuple_SET_ITEM(stat_data, 8, format_time(st->st_mtime, mnsec));
        PyTuple_SET_ITEM(stat_data, 9, format_time(st->st_ctime, cnsec));
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
        PyTuple_SET_ITEM(stat_data, 10, PyInt_FromLong((long)st->st_blksize));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(stat_data, 10, Py_None);
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
        PyTuple_SET_ITEM(stat_data, 11, PyInt_FromLong((long)st->st_blocks));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(stat_data, 11, Py_None);
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
        PyTuple_SET_ITEM(stat_data, 12, PyInt_FromLong((long)st->st_rdev));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(stat_data, 12, Py_None);
#endif
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, stat_data, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

end:

    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    req->data = NULL;
    uv_fs_req_cleanup(req);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
unlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_UNLINK);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
mkdir_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_MKDIR);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
rmdir_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_RMDIR);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
rename_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_RENAME);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
chmod_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CHMOD || req->fs_type == UV_FS_FCHMOD);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
link_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_LINK);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
symlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_SYMLINK);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
readlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_READLINK);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno, *path;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
        path = Py_None;
    } else {
        errorno = PyInt_FromLong(0);
        path = PyString_FromString(req->ptr);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, path, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
chown_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CHOWN || req->fs_type == UV_FS_FCHOWN);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
open_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_OPEN);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
close_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CLOSE);

    fs_req_data_t *req_data = (fs_req_data_t*)(req->data);

    PyObject *result, *errorno;

    result = PyInt_FromLong((long)req->result);
    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = PyInt_FromLong(0);
    }

    PyObject *cb_result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, req_data->data, result, errorno, NULL);
    if (cb_result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(cb_result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    Py_DECREF(req_data->data);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static PyObject *
stat_func(PyObject *self, PyObject *args, PyObject *kwargs, int type)
{
    int r;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sO|O:stat", kwlist, &LoopType, &loop, &path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }
   
    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    if (type == UV_FS_STAT) {
        r = uv_fs_stat(loop->uv_loop, fs_req, path, stat_cb);
    } else {
        r = uv_fs_lstat(loop->uv_loop, fs_req, path, stat_cb);
    }
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_stat(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return stat_func(self, args, kwargs, UV_FS_STAT);
}


static PyObject *
FS_func_lstat(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return stat_func(self, args, kwargs, UV_FS_LSTAT);
}


static PyObject *
FS_func_fstat(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int fd;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "fd", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iO|O:stat", kwlist, &LoopType, &loop, &fd, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }
   
    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_fstat(loop->uv_loop, fs_req, fd, stat_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_unlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sO|O:stat", kwlist, &LoopType, &loop, &path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_unlink(loop->uv_loop, fs_req, path, unlink_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_mkdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int mode;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "mode", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!siO|O:stat", kwlist, &LoopType, &loop, &path, &mode, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_mkdir(loop->uv_loop, fs_req, path, mode, mkdir_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_rmdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sO|O:rmdir", kwlist, &LoopType, &loop, &path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_rmdir(loop->uv_loop, fs_req, path, rmdir_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_rename(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    char *new_path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssO|O:rmdir", kwlist, &LoopType, &loop, &path, &new_path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_rename(loop->uv_loop, fs_req, path, new_path, rename_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_chmod(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int mode;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "mode", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!siO|O:stat", kwlist, &LoopType, &loop, &path, &mode, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_chmod(loop->uv_loop, fs_req, path, mode, chmod_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_fchmod(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int mode;
    int fd;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "fd", "mode", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iiO|O:stat", kwlist, &LoopType, &loop, &fd, &mode, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_fchmod(loop->uv_loop, fs_req, fd, mode, chmod_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_link(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    char *new_path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssO|O:rmdir", kwlist, &LoopType, &loop, &path, &new_path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_link(loop->uv_loop, fs_req, path, new_path, link_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_symlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int flags;
    char *path;
    char *new_path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "new_path", "flags", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssiO|O:rmdir", kwlist, &LoopType, &loop, &path, &new_path, &flags, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_symlink(loop->uv_loop, fs_req, path, new_path, flags, symlink_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_readlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sO|O:rmdir", kwlist, &LoopType, &loop, &path, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_readlink(loop->uv_loop, fs_req, path, readlink_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_chown(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int uid;
    int gid;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "uid", "gid", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!siiO|O:rmdir", kwlist, &LoopType, &loop, &path, &uid, &gid, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_chown(loop->uv_loop, fs_req, path, uid, gid, chown_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_fchown(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int fd;
    int uid;
    int gid;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "fd", "uid", "gid", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iiiO|O:rmdir", kwlist, &LoopType, &loop, &fd, &uid, &gid, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_fchown(loop->uv_loop, fs_req, fd, uid, gid, chown_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_open(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int flags;
    int mode;
    char *path;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "path", "flags", "mode", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!siiO|O:open", kwlist, &LoopType, &loop, &path, &flags, &mode, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_open(loop->uv_loop, fs_req, path, flags, mode, open_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_close(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int r;
    int fd;
    Loop *loop;
    PyObject *callback;
    PyObject *data = Py_None;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;

    static char *kwlist[] = {"loop", "fd", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iO|O:close", kwlist, &LoopType, &loop, &fd, &callback, &data)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto error;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto error;
    }

    Py_INCREF(loop);
    Py_INCREF(callback);
    Py_INCREF(data);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->data = data;

    fs_req->data = (void *)req_data;
    r = uv_fs_close(loop->uv_loop, fs_req, fd, close_cb);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto error;
    }

    Py_RETURN_NONE;

error:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_DECREF(callback);
        Py_DECREF(data);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyMethodDef
FS_methods[] = {
    { "stat", (PyCFunction)FS_func_stat, METH_VARARGS|METH_KEYWORDS, "stat" },
    { "lstat", (PyCFunction)FS_func_lstat, METH_VARARGS|METH_KEYWORDS, "lstat" },
    { "fstat", (PyCFunction)FS_func_fstat, METH_VARARGS|METH_KEYWORDS, "fstat" },
    { "unlink", (PyCFunction)FS_func_unlink, METH_VARARGS|METH_KEYWORDS, "Remove a file from the filesystem." },
    { "mkdir", (PyCFunction)FS_func_mkdir, METH_VARARGS|METH_KEYWORDS, "Create a directory." },
    { "rmdir", (PyCFunction)FS_func_rmdir, METH_VARARGS|METH_KEYWORDS, "Remove a directory." },
    { "rename", (PyCFunction)FS_func_rename, METH_VARARGS|METH_KEYWORDS, "Rename a file." },
    { "chmod", (PyCFunction)FS_func_chmod, METH_VARARGS|METH_KEYWORDS, "Change file permissions." },
    { "fchmod", (PyCFunction)FS_func_fchmod, METH_VARARGS|METH_KEYWORDS, "Change file permissions." },
    { "link", (PyCFunction)FS_func_link, METH_VARARGS|METH_KEYWORDS, "Create hardlink." },
    { "symlink", (PyCFunction)FS_func_symlink, METH_VARARGS|METH_KEYWORDS, "Create symbolic link." },
    { "readlink", (PyCFunction)FS_func_readlink, METH_VARARGS|METH_KEYWORDS, "Get the path to which the symbolic link points." },
    { "chown", (PyCFunction)FS_func_chown, METH_VARARGS|METH_KEYWORDS, "Change file ownership." },
    { "fchown", (PyCFunction)FS_func_fchown, METH_VARARGS|METH_KEYWORDS, "Change file ownership." },
    { "open", (PyCFunction)FS_func_open, METH_VARARGS|METH_KEYWORDS, "Open file." },
    { "close", (PyCFunction)FS_func_close, METH_VARARGS|METH_KEYWORDS, "Close file." },
    { NULL }
};


PyObject *
init_fs(void)
{
    PyObject *module;
    module = Py_InitModule("pyuv.fs", FS_methods);
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddIntMacro(module, UV_FS_SYMLINK_DIR);

    return module;
}


