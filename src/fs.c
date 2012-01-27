
static PyObject* PyExc_FSError;

typedef struct {
    Loop *loop;
    PyObject *callback;
    uv_buf_t buf;
} fs_req_data_t;


static PyObject *
format_time(time_t sec, unsigned long nsec)
{
    /* TODO: look at stat_float_times in Modules/posixmodule.c */
    PyObject *ival;

    UNUSED_ARG(nsec);

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
process_stat(uv_fs_t* req, PyObject **path, PyObject **stat_data, PyObject **errorno) {
    struct stat *st;
    unsigned long ansec, mnsec, cnsec;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_STAT || req->fs_type == UV_FS_LSTAT || req->fs_type == UV_FS_FSTAT);

    st = (struct stat *)(req->ptr);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
    }

    *stat_data = PyTuple_New(13);
    if (!stat_data) {
        PyErr_NoMemory();
        PyErr_WriteUnraisable(Py_None);
        Py_DECREF(*path);
        Py_DECREF(*errorno);
        *path = NULL;
        *errorno = NULL;
        return;
    }

    if (req->result != -1) {
        PyTuple_SET_ITEM(*stat_data, 0, PyInt_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyTuple_SET_ITEM(*stat_data, 1, PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
        PyTuple_SET_ITEM(*stat_data, 1, PyInt_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG)
        PyTuple_SET_ITEM(*stat_data, 2, PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
        PyTuple_SET_ITEM(*stat_data, 2, PyInt_FromLong((long)st->st_dev));
#endif
        PyTuple_SET_ITEM(*stat_data, 3, PyInt_FromLong((long)st->st_nlink));
        PyTuple_SET_ITEM(*stat_data, 4, PyInt_FromLong((long)st->st_uid));
        PyTuple_SET_ITEM(*stat_data, 5, PyInt_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
        PyTuple_SET_ITEM(*stat_data, 6, PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
        PyTuple_SET_ITEM(*stat_data, 6, PyInt_FromLong(st->st_size));
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
        PyTuple_SET_ITEM(*stat_data, 7, format_time(st->st_atime, ansec));
        PyTuple_SET_ITEM(*stat_data, 8, format_time(st->st_mtime, mnsec));
        PyTuple_SET_ITEM(*stat_data, 9, format_time(st->st_ctime, cnsec));
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
        PyTuple_SET_ITEM(*stat_data, 10, PyInt_FromLong((long)st->st_blksize));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(*stat_data, 10, Py_None);
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
        PyTuple_SET_ITEM(*stat_data, 11, PyInt_FromLong((long)st->st_blocks));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(*stat_data, 11, Py_None);
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
        PyTuple_SET_ITEM(*stat_data, 12, PyInt_FromLong((long)st->st_rdev));
# else
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(*stat_data, 12, Py_None);
#endif
    }

}


static void
stat_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *stat_data, *path;

    process_stat(req, &path, &stat_data, &errorno);
    req_data = (fs_req_data_t*)req->data;

    if (path && stat_data && errorno) {
        result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, stat_data, errorno, NULL);
        if (result == NULL) {
            PyErr_WriteUnraisable(req_data->callback);
        }
        Py_XDECREF(result);
    }

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
unlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_UNLINK);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
mkdir_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_MKDIR);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
rmdir_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_RMDIR);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
rename_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_RENAME);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
chmod_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CHMOD || req->fs_type == UV_FS_FCHMOD);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
link_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_LINK);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
symlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_SYMLINK);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_readlink(uv_fs_t* req, PyObject **path, PyObject **errorno)
{
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_READLINK);

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
        *path = Py_None;
        Py_INCREF(Py_None);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->ptr);
#else
        *path = PyString_FromString(req->ptr);
#endif
    }
}


static void
readlink_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    process_readlink(req, &path, &errorno);
    req_data = (fs_req_data_t*)req->data;

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
chown_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CHOWN || req->fs_type == UV_FS_FCHOWN);

    req_data = (fs_req_data_t*)req->data;

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_open(uv_fs_t* req, PyObject **path, PyObject **fd, PyObject **errorno)
{
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_OPEN);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif  
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
        *fd = Py_None;
        Py_INCREF(Py_None);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
        *fd = PyInt_FromLong((long)req->result);
    }
}


static void
open_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *fd, *errorno, *path;

    process_open(req, &path, &fd, &errorno);
    req_data = (fs_req_data_t*)req->data;

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, fd, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
close_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_CLOSE);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif  
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_read(uv_fs_t* req, PyObject **path, PyObject **read_data, PyObject **errorno)
{
    fs_req_data_t *req_data;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_READ);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
        *read_data = Py_None;
        Py_INCREF(Py_None);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
#ifdef PYUV_PYTHON3
        *read_data = PyUnicode_FromStringAndSize(req_data->buf.base, req->result);
#else
        *read_data = PyString_FromStringAndSize(req_data->buf.base, req->result);
#endif
    }
}


static void
read_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *read_data, *path;

    process_read(req, &path, &read_data, &errorno);
    req_data = (fs_req_data_t*)(req->data);

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, read_data, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data->buf.base);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_write(uv_fs_t* req, PyObject **path, PyObject **bytes_written, PyObject **errorno)
{
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_WRITE);

    *bytes_written = PyInt_FromLong((long)req->result);
    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
    }
}


static void
write_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *bytes_written, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_WRITE);

    process_write(req, &path, &bytes_written, &errorno);
    req_data = (fs_req_data_t*)(req->data);

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, bytes_written, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data->buf.base);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
fsync_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_FSYNC || req->fs_type == UV_FS_FDATASYNC);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
ftruncate_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_FTRUNCATE);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_readdir(uv_fs_t* req, PyObject **path, PyObject **files, PyObject **errorno)
{
    int r;
    char *ptr;
    PyObject *item;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_READDIR);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
        *files = Py_None;
        Py_INCREF(Py_None);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
        *files = PyList_New(0);
        if (!files) {
            PyErr_NoMemory();
            PyErr_WriteUnraisable(Py_None);
            *files = Py_None;
            Py_INCREF(Py_None);
        } else {
            r = req->result;
            ptr = req->ptr;
            while (r--) {
#ifdef PYUV_PYTHON3
                item = PyUnicode_FromString(ptr);
#else
                item = PyString_FromString(ptr);
#endif
                PyList_Append(*files, item);
                Py_DECREF(item);
                ptr += strlen(ptr) + 1;
            }
        }
    }

}


static void
readdir_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *files, *path;

    process_readdir(req, &path, &files, &errorno);
    req_data = (fs_req_data_t*)(req->data);

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, files, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
process_sendfile(uv_fs_t* req, PyObject **path, PyObject **bytes_written, PyObject **errorno)
{
    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_SENDFILE);

    *bytes_written = PyInt_FromLong((long)req->result);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        *path = PyUnicode_FromString(req->path);
#else
        *path = PyString_FromString(req->path);
#endif
    } else {
        *path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        *errorno = PyInt_FromLong((long)req->errorno);
    } else {
        *errorno = Py_None;
        Py_INCREF(Py_None);
    }
}


static void
sendfile_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *bytes_written, *path;

    process_sendfile(req, &path, &bytes_written, &errorno);
    req_data = (fs_req_data_t*)(req->data);

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, bytes_written, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static void
utime_cb(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    fs_req_data_t *req_data;
    PyObject *result, *errorno, *path;

    ASSERT(req);
    ASSERT(req->fs_type == UV_FS_UTIME || req->fs_type == UV_FS_FUTIME);

    req_data = (fs_req_data_t*)(req->data);

    if (req->path != NULL) {
#ifdef PYUV_PYTHON3
        path = PyUnicode_FromString(req->path);
#else
        path = PyString_FromString(req->path);
#endif
    } else {
        path = Py_None;
        Py_INCREF(Py_None);
    }

    if (req->result < 0) {
        errorno = PyInt_FromLong((long)req->errorno);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    result = PyObject_CallFunctionObjArgs(req_data->callback, req_data->loop, path, errorno, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(req_data->callback);
    }
    Py_XDECREF(result);

    uv_fs_req_cleanup(req);
    Py_DECREF(req_data->loop);
    Py_DECREF(req_data->callback);
    PyMem_Free(req_data);
    PyMem_Free(req);

    PyGILState_Release(gstate);
}


static PyObject *
stat_func(PyObject *args, PyObject *kwargs, int type)
{
    int r;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *stat_data, *py_errorno, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:stat", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    if (type == UV_FS_STAT) {
        r = uv_fs_stat(loop->uv_loop, fs_req, path, (callback != NULL) ? stat_cb : NULL);
    } else {
        r = uv_fs_lstat(loop->uv_loop, fs_req, path, (callback != NULL) ? stat_cb : NULL);
    }
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_stat(fs_req, &py_path, &stat_data, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = stat_data;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_stat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return stat_func(args, kwargs, UV_FS_STAT);
}


static PyObject *
FS_func_lstat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return stat_func(args, kwargs, UV_FS_LSTAT);
}


static PyObject *
FS_func_fstat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *stat_data, *py_errorno, *ret;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!i|O:fstat", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_fstat(loop->uv_loop, fs_req, fd, (callback != NULL) ? stat_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_stat(fs_req, &py_path, &stat_data, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = stat_data;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_unlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:unlink", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_unlink(loop->uv_loop, fs_req, path, (callback != NULL) ? unlink_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_mkdir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, mode;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:mkdir", kwlist, &LoopType, &loop, &path, &mode, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_mkdir(loop->uv_loop, fs_req, path, mode, (callback != NULL) ? mkdir_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_rmdir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:rmdir", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_rmdir(loop->uv_loop, fs_req, path, (callback != NULL) ? rmdir_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_rename(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path, *new_path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ss|O:rename", kwlist, &LoopType, &loop, &path, &new_path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_rename(loop->uv_loop, fs_req, path, new_path, (callback != NULL) ? rename_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_chmod(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, mode;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:chmod", kwlist, &LoopType, &loop, &path, &mode, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_chmod(loop->uv_loop, fs_req, path, mode, (callback != NULL) ? chmod_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_fchmod(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, mode, fd;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ii|O:fchmod", kwlist, &LoopType, &loop, &fd, &mode, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_fchmod(loop->uv_loop, fs_req, fd, mode, (callback != NULL) ? chmod_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_link(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path, *new_path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ss|O:link", kwlist, &LoopType, &loop, &path, &new_path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_link(loop->uv_loop, fs_req, path, new_path, (callback != NULL) ? link_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_symlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, flags;
    char *path, *new_path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "new_path", "flags", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssi|O:symlink", kwlist, &LoopType, &loop, &path, &new_path, &flags, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_symlink(loop->uv_loop, fs_req, path, new_path, flags, (callback != NULL) ? symlink_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_readlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:readlink", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_readlink(loop->uv_loop, fs_req, path, (callback != NULL) ? readlink_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_readlink(fs_req, &py_path, &py_errorno);
        Py_DECREF(py_errorno);
        ret = py_path;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_chown(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, uid, gid;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "uid", "gid", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sii|O:chown", kwlist, &LoopType, &loop, &path, &uid, &gid, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_chown(loop->uv_loop, fs_req, path, uid, gid, (callback != NULL) ? chown_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_fchown(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd, uid, gid;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "uid", "gid", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iii|O:fchown", kwlist, &LoopType, &loop, &fd, &uid, &gid, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_fchown(loop->uv_loop, fs_req, fd, uid, gid, (callback != NULL) ? chown_cb : NULL);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_open(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, flags, mode;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *fd, *ret;

    static char *kwlist[] = {"loop", "path", "flags", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sii|O:open", kwlist, &LoopType, &loop, &path, &flags, &mode, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_open(loop->uv_loop, fs_req, path, flags, mode, (callback != NULL) ? open_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_open(fs_req, &py_path, &fd, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = fd;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_close(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!i|O:close", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_close(loop->uv_loop, fs_req, fd, (callback != NULL) ? close_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_read(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd, length, offset;
    char *buf_data = NULL;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *read_data, *ret;

    static char *kwlist[] = {"loop", "fd", "length", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iii|O:read", kwlist, &LoopType, &loop, &fd, &length, &offset, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    buf_data = PyMem_Malloc(length);
    if (!buf_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }
    memset(buf_data, 0, length);

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->buf = uv_buf_init(buf_data, length);

    fs_req->data = (void *)req_data;
    r = uv_fs_read(loop->uv_loop, fs_req, fd, req_data->buf.base, length, offset, (callback != NULL) ? read_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_read(fs_req, &py_path, &read_data, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = read_data;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    if (buf_data) {
        PyMem_Free(buf_data);
    }
    return ret;
}


static PyObject *
FS_func_write(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd, offset, write_len;
    char *write_str, *buf_data;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *written_bytes, *ret;

    static char *kwlist[] = {"loop", "fd", "write_data", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    write_str = buf_data = NULL;
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!is#i|O:write", kwlist, &LoopType, &loop, &fd, &write_str, &write_len, &offset, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    buf_data = (char *)PyMem_Malloc(write_len+1);
    if (!buf_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }
    memcpy(buf_data, write_str, write_len+1);

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;
    req_data->buf = uv_buf_init(buf_data, write_len);

    fs_req->data = (void *)req_data;
    r = uv_fs_write(loop->uv_loop, fs_req, fd, req_data->buf.base, req_data->buf.len, offset, (callback != NULL) ? write_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_write(fs_req, &py_path, &written_bytes, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = written_bytes;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    if (buf_data) {
        PyMem_Free(buf_data);
    }
    return ret;
}


static PyObject *
FS_func_fsync(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!i|O:fsync", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_fsync(loop->uv_loop, fs_req, fd, (callback != NULL) ? fsync_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_fdatasync(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!i|O:fdatasync", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_fdatasync(loop->uv_loop, fs_req, fd, (callback != NULL) ? fsync_cb : NULL);
    if (r != 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_ftruncate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd, offset;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ii|O:ftruncate", kwlist, &LoopType, &loop, &fd, &offset, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_ftruncate(loop->uv_loop, fs_req, fd, offset, (callback != NULL) ? ftruncate_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


/* Aapparently 'flags' doesn't do much (nothing on Windows) because libuv uses ptr from eio, instead of ptr2 */
static PyObject *
FS_func_readdir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, flags;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *files, *ret;

    static char *kwlist[] = {"loop", "path", "flags", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:readdir", kwlist, &LoopType, &loop, &path, &flags, &callback)) {
        return NULL;
    }

    if (callback !=NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_readdir(loop->uv_loop, fs_req, path, flags, (callback != NULL) ? readdir_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_readdir(fs_req, &py_path, &files, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = files;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_sendfile(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, out_fd, in_fd, in_offset, length;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback, *py_path, *py_errorno, *bytes_written, *ret;

    static char *kwlist[] = {"loop", "out_fd", "in_fd", "in_offset", "length", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iiii|O:sendfile", kwlist, &LoopType, &loop, &out_fd, &in_fd, &in_offset, &length, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        ret = NULL;
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_sendfile(loop->uv_loop, fs_req, out_fd, in_fd, in_offset, length, (callback != NULL) ? sendfile_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        ret = NULL;
        goto end;
    }

    if (callback != NULL) {
        /* No need to cleanup, it will be done in the callback */
        Py_RETURN_NONE;
    } else {
        process_sendfile(fs_req, &py_path, &bytes_written, &py_errorno);
        Py_DECREF(py_path);
        Py_DECREF(py_errorno);
        ret = bytes_written;
    }

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return ret;
}


static PyObject *
FS_func_utime(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r;
    double atime, mtime;
    char *path;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "path", "atime", "mtime", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sdd|O:utime", kwlist, &LoopType, &loop, &path, &atime, &mtime, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_utime(loop->uv_loop, fs_req, path, atime, mtime, (callback != NULL) ? utime_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
        PyMem_Free(req_data);
    }
    return NULL;
}


static PyObject *
FS_func_futime(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int r, fd;
    double atime, mtime;
    uv_fs_t *fs_req = NULL;
    fs_req_data_t *req_data = NULL;
    Loop *loop;
    PyObject *callback;

    static char *kwlist[] = {"loop", "fd", "atime", "mtime", "callback", NULL};

    UNUSED_ARG(obj);
    callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!idd|O:futime", kwlist, &LoopType, &loop, &fd, &atime, &mtime, &callback)) {
        return NULL;
    }

    if (callback != NULL && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = PyMem_Malloc(sizeof(uv_fs_t));
    if (!fs_req) {
        PyErr_NoMemory();
        goto end;
    }

    req_data = PyMem_Malloc(sizeof(fs_req_data_t));
    if (!req_data) {
        PyErr_NoMemory();
        goto end;
    }

    Py_INCREF(loop);
    Py_XINCREF(callback);

    req_data->loop = loop;
    req_data->callback = callback;

    fs_req->data = (void *)req_data;
    r = uv_fs_futime(loop->uv_loop, fs_req, fd, atime, mtime, (callback != NULL) ? utime_cb : NULL);
    if (r < 0) {
        raise_uv_exception(loop, PyExc_FSError);
        goto end;
    }

    Py_RETURN_NONE;

end:
    if (fs_req) {
        PyMem_Free(fs_req);
    }
    if (req_data) {
        Py_DECREF(loop);
        Py_XDECREF(callback);
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
    { "read", (PyCFunction)FS_func_read, METH_VARARGS|METH_KEYWORDS, "Read data from a file." },
    { "write", (PyCFunction)FS_func_write, METH_VARARGS|METH_KEYWORDS, "Write data to a file." },
    { "fsync", (PyCFunction)FS_func_fsync, METH_VARARGS|METH_KEYWORDS, "Sync all changes made to a file." },
    { "fdatasync", (PyCFunction)FS_func_fdatasync, METH_VARARGS|METH_KEYWORDS, "Sync data changes made to a file." },
    { "ftruncate", (PyCFunction)FS_func_ftruncate, METH_VARARGS|METH_KEYWORDS, "Truncate the contents of a file to the specified offset." },
    { "readdir", (PyCFunction)FS_func_readdir, METH_VARARGS|METH_KEYWORDS, "List files from a directory." },
    { "sendfile", (PyCFunction)FS_func_sendfile, METH_VARARGS|METH_KEYWORDS, "Sends a regular file to a stream socket." },
    { "utime", (PyCFunction)FS_func_utime, METH_VARARGS|METH_KEYWORDS, "Update file times." },
    { "futime", (PyCFunction)FS_func_futime, METH_VARARGS|METH_KEYWORDS, "Update file times." },
    { NULL }
};


static PyObject* PyExc_FSEventError;


static void
on_fsevent_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    FSEvent *self;
    PyObject *result;

    ASSERT(handle);
    self = (FSEvent *)handle->data;
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
on_fsevent_dealloc_close(uv_handle_t *handle)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    ASSERT(handle);
    handle->data = NULL;
    PyMem_Free(handle);
    PyGILState_Release(gstate);
}


static void
on_fsevent_callback(uv_fs_event_t *handle, const char *filename, int events, int status)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    FSEvent *self;
    PyObject *result, *py_filename, *py_events, *errorno;

    ASSERT(handle);
    ASSERT(status == 0);

    self = (FSEvent *)handle->data;
    ASSERT(self);
    /* Object could go out of scope in the callback, increase refcount to avoid it */
    Py_INCREF(self);

    if (filename) {
#ifdef PYUV_PYTHON3
        py_filename = PyUnicode_FromString(filename);
#else
        py_filename = PyString_FromString(filename);
#endif
    } else {
        py_filename = Py_None;
        Py_INCREF(Py_None);
    }

    if (status < 0) {
        uv_err_t err = uv_last_error(UV_LOOP(self));
        errorno = PyInt_FromLong((long)err.code);
    } else {
        errorno = Py_None;
        Py_INCREF(Py_None);
    }

    py_events = PyInt_FromLong((long)events);

    result = PyObject_CallFunctionObjArgs(self->on_fsevent_cb, self, py_filename, py_events, errorno, NULL);
    if (result == NULL) {
	PyErr_WriteUnraisable(self->on_fsevent_cb);
    }
    Py_XDECREF(result);

    Py_DECREF(self);
    PyGILState_Release(gstate);
}


static PyObject *
FSEvent_func_start(FSEvent *self, PyObject *args, PyObject *kwargs)
{
    int r, flags;
    char *path;
    uv_fs_event_t *fs_event = NULL;
    PyObject *tmp, *callback;

    static char *kwlist[] = {"path", "callback", "flags", NULL};

    tmp = NULL;

    if (self->uv_handle) {
        PyErr_SetString(PyExc_FSEventError, "FSEvent was already started");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sOi:start", kwlist, &path, &callback, &flags)) {
        return NULL;
    }

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_event = PyMem_Malloc(sizeof(uv_fs_event_t));
    if (!fs_event) {
        PyErr_NoMemory();
        return NULL;
    }
    fs_event->data = (void *)self;
    self->uv_handle = fs_event;

    r = uv_fs_event_init(UV_LOOP(self), self->uv_handle, path, on_fsevent_callback, flags);
    if (r != 0) {
        raise_uv_exception(self->loop, PyExc_FSEventError);
        PyMem_Free(fs_event);
        self->uv_handle = NULL;
        return NULL;
    }

    tmp = self->on_fsevent_cb;
    Py_INCREF(callback);
    self->on_fsevent_cb = callback;
    Py_XDECREF(tmp);

    Py_RETURN_NONE;
}


static PyObject *
FSEvent_func_close(FSEvent *self, PyObject *args)
{
    PyObject *callback = Py_None;

    if (!self->uv_handle) {
        PyErr_SetString(PyExc_FSEventError, "FSEvent has not been started");
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

    uv_close((uv_handle_t *)self->uv_handle, on_fsevent_close);
    self->uv_handle = NULL;

    Py_RETURN_NONE;
}


static int
FSEvent_tp_init(FSEvent *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *tmp = NULL;

    UNUSED_ARG(kwargs);

    if (self->uv_handle) {
        PyErr_SetString(PyExc_FSEventError, "Object already initialized");
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
FSEvent_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FSEvent *self = (FSEvent *)PyType_GenericNew(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    self->uv_handle = NULL;
    return (PyObject *)self;
}


static int
FSEvent_tp_traverse(FSEvent *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->on_fsevent_cb);
    Py_VISIT(self->on_close_cb);
    Py_VISIT(self->loop);
    return 0;
}


static int
FSEvent_tp_clear(FSEvent *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->on_fsevent_cb);
    Py_CLEAR(self->on_close_cb);
    Py_CLEAR(self->loop);
    return 0;
}


static void
FSEvent_tp_dealloc(FSEvent *self)
{
    if (self->uv_handle) {
        uv_close((uv_handle_t *)self->uv_handle, on_fsevent_dealloc_close);
        self->uv_handle = NULL;
    }
    FSEvent_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMethodDef
FSEvent_tp_methods[] = {
    { "start", (PyCFunction)FSEvent_func_start, METH_VARARGS|METH_KEYWORDS, "Start the FSEvent." },
    { "close", (PyCFunction)FSEvent_func_close, METH_VARARGS, "Close the FSEvent." },
    { NULL }
};


static PyMemberDef FSEvent_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(FSEvent, loop), READONLY, "Loop where this FSEvent is running on."},
    {"data", T_OBJECT, offsetof(FSEvent, data), 0, "Arbitrary data."},
    {NULL}
};


static PyTypeObject FSEventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyuv.fs.FSEvent",                                              /*tp_name*/
    sizeof(FSEvent),                                                /*tp_basicsize*/
    0,                                                              /*tp_itemsize*/
    (destructor)FSEvent_tp_dealloc,                                 /*tp_dealloc*/
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
    (traverseproc)FSEvent_tp_traverse,                              /*tp_traverse*/
    (inquiry)FSEvent_tp_clear,                                      /*tp_clear*/
    0,                                                              /*tp_richcompare*/
    0,                                                              /*tp_weaklistoffset*/
    0,                                                              /*tp_iter*/
    0,                                                              /*tp_iternext*/
    FSEvent_tp_methods,                                             /*tp_methods*/
    FSEvent_tp_members,                                             /*tp_members*/
    0,                                                              /*tp_getsets*/
    0,                                                              /*tp_base*/
    0,                                                              /*tp_dict*/
    0,                                                              /*tp_descr_get*/
    0,                                                              /*tp_descr_set*/
    0,                                                              /*tp_dictoffset*/
    (initproc)FSEvent_tp_init,                                      /*tp_init*/
    0,                                                              /*tp_alloc*/
    FSEvent_tp_new,                                                 /*tp_new*/
};

#ifdef PYUV_PYTHON3
static PyModuleDef pyuv_fs_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv.fs",              /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    FS_methods,             /*m_methods*/
};
#endif

PyObject *
init_fs(void)
{
    PyObject *module;
#ifdef PYUV_PYTHON3
    module = PyModule_Create(&pyuv_fs_module);
#else
    module = Py_InitModule("pyuv.fs", FS_methods);
#endif
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddIntMacro(module, UV_FS_SYMLINK_DIR);
    PyModule_AddIntMacro(module, UV_RENAME);
    PyModule_AddIntMacro(module, UV_CHANGE);
    PyModule_AddIntMacro(module, UV_FS_EVENT_WATCH_ENTRY);
    PyModule_AddIntMacro(module, UV_FS_EVENT_STAT);

    PyUVModule_AddType(module, "FSEvent", &FSEventType);

    return module;
}


