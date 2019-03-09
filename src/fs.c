
static INLINE PyObject *
format_time(uv_timespec_t tspec)
{
    return PyFloat_FromDouble(tspec.tv_sec + 1e-9*tspec.tv_nsec);
}


static INLINE void
stat_to_pyobj(const uv_stat_t *st, PyObject *stat_data) {
    PyStructSequence_SET_ITEM(stat_data, 0, PyLong_FromUnsignedLongLong(st->st_mode));
    PyStructSequence_SET_ITEM(stat_data, 1, PyLong_FromUnsignedLongLong(st->st_ino));
    PyStructSequence_SET_ITEM(stat_data, 2, PyLong_FromUnsignedLongLong(st->st_dev));
    PyStructSequence_SET_ITEM(stat_data, 3, PyLong_FromUnsignedLongLong(st->st_nlink));
    PyStructSequence_SET_ITEM(stat_data, 4, PyLong_FromUnsignedLongLong(st->st_uid));
    PyStructSequence_SET_ITEM(stat_data, 5, PyLong_FromUnsignedLongLong(st->st_gid));
    PyStructSequence_SET_ITEM(stat_data, 6, PyLong_FromUnsignedLongLong(st->st_size));
    PyStructSequence_SET_ITEM(stat_data, 7, format_time(st->st_atim));
    PyStructSequence_SET_ITEM(stat_data, 8, format_time(st->st_mtim));
    PyStructSequence_SET_ITEM(stat_data, 9, format_time(st->st_ctim));
    PyStructSequence_SET_ITEM(stat_data, 10, PyLong_FromUnsignedLongLong(st->st_blksize));
    PyStructSequence_SET_ITEM(stat_data, 11, PyLong_FromUnsignedLongLong(st->st_blocks));
    PyStructSequence_SET_ITEM(stat_data, 12, PyLong_FromUnsignedLongLong(st->st_rdev));
    PyStructSequence_SET_ITEM(stat_data, 13, PyLong_FromUnsignedLongLong(st->st_flags));
    PyStructSequence_SET_ITEM(stat_data, 14, PyLong_FromUnsignedLongLong(st->st_gen));
    PyStructSequence_SET_ITEM(stat_data, 15, format_time(st->st_birthtim));
}


/*
 * NOTE: This function is called either by libuv as a callback or by us when a synchronous
 * operation is performed.
 */
static void
pyuv__process_fs_req(uv_fs_t* req) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    Loop *loop;
    FSRequest *fs_req;
    PyObject *result, *errorno, *r, *path, *item;

    ASSERT(req);
    fs_req = PYUV_CONTAINER_OF(req, FSRequest, req);
    loop = REQUEST(fs_req)->loop;

    if (req->path != NULL) {
        path = Py_BuildValue("s", req->path);
    } else {
        PYUV_SET_NONE(path);
    }

    if (req->result < 0) {
        errorno = PyLong_FromLong((long)req->result);
        PYUV_SET_NONE(r);
    } else {
        PYUV_SET_NONE(errorno);
        switch (req->fs_type) {
            case UV_FS_STAT:
            case UV_FS_LSTAT:
            case UV_FS_FSTAT:
                r = PyStructSequence_New(&StatResultType);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                } else {
                    stat_to_pyobj(&req->statbuf, r);
                }
                break;
            case UV_FS_UNLINK:
            case UV_FS_MKDIR:
            case UV_FS_RMDIR:
            case UV_FS_RENAME:
            case UV_FS_CHMOD:
            case UV_FS_FCHMOD:
            case UV_FS_LINK:
            case UV_FS_SYMLINK:
            case UV_FS_CHOWN:
            case UV_FS_LCHOWN:
            case UV_FS_FCHOWN:
            case UV_FS_CLOSE:
            case UV_FS_FSYNC:
            case UV_FS_FDATASYNC:
            case UV_FS_FTRUNCATE:
            case UV_FS_UTIME:
            case UV_FS_FUTIME:
            case UV_FS_ACCESS:
            case UV_FS_COPYFILE:
                PYUV_SET_NONE(r);
                break;
            case UV_FS_READLINK:
            case UV_FS_REALPATH:
                r = Py_BuildValue("s", req->ptr);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                }
                break;
            case UV_FS_WRITE:
                r = PyLong_FromLong((long)req->result);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                }
                PyBuffer_Release(&fs_req->view);
                break;
            case UV_FS_OPEN:
            case UV_FS_SENDFILE:
                r = PyLong_FromLong((long)req->result);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                }
                break;
            case UV_FS_READ:
                r = PyBytes_FromStringAndSize(fs_req->buf.base, req->result);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                }
                PyMem_Free(fs_req->buf.base);
                break;
            case UV_FS_SCANDIR:
                r = PyList_New(0);
                if (!r) {
                    PyErr_Clear();
                    PYUV_SET_NONE(r);
                } else {
                    uv_dirent_t ent;
                    while (uv_fs_scandir_next(req, &ent) != UV_EOF) {
                        item = PyStructSequence_New(&DirEntType);
                        if (!item) {
                            PyErr_Clear();
                            break;
                        }
                        PyStructSequence_SET_ITEM(item, 0, Py_BuildValue("s", ent.name));
                        PyStructSequence_SET_ITEM(item, 1, PyLong_FromLong((long)ent.type));
                        PyList_Append(r, item);
                        Py_DECREF(item);
                    }
                }
                break;
            default:
                ASSERT(!"unknown fs req type");
                break;
        }
    }

    /* Save result, path and error in the FSRequest object */
    fs_req->path = path;
    fs_req->result = r;
    fs_req->error = errorno;

    if (fs_req->callback != Py_None) {
        result = PyObject_CallFunctionObjArgs(fs_req->callback, fs_req, NULL);
        if (result == NULL) {
            handle_uncaught_exception(loop);
        }
        Py_XDECREF(result);
    }

    uv_fs_req_cleanup(req);
    UV_REQUEST(fs_req) = NULL;
    Py_DECREF(fs_req);

    PyGILState_Release(gstate);
}


static INLINE PyObject *
pyuv__fs_stat(PyObject *args, PyObject *kwargs, int type)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:stat", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    if (type == UV_FS_STAT) {
        err = uv_fs_stat(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    } else {
        err = uv_fs_lstat(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    }
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_stat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return pyuv__fs_stat(args, kwargs, UV_FS_STAT);
}


static PyObject *
FS_func_lstat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return pyuv__fs_stat(args, kwargs, UV_FS_LSTAT);
}


static PyObject *
FS_func_fstat(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!l|O:fstat", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_fstat(loop->uv_loop, &fs_req->req, (uv_file)fd, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_unlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:unlink", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_unlink(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_mkdir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, mode;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:mkdir", kwlist, &LoopType, &loop, &path, &mode, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_mkdir(loop->uv_loop, &fs_req->req, path, mode, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_rmdir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:rmdir", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_rmdir(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_rename(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path, *new_path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ss|O:rename", kwlist, &LoopType, &loop, &path, &new_path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_rename(loop->uv_loop, &fs_req->req, path, new_path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_chmod(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, mode;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:chmod", kwlist, &LoopType, &loop, &path, &mode, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_chmod(loop->uv_loop, &fs_req->req, path, mode, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_fchmod(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, mode;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!li|O:fchmod", kwlist, &LoopType, &loop, &fd, &mode, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_fchmod(loop->uv_loop, &fs_req->req, (uv_file)fd, mode, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_link(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path, *new_path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "new_path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ss|O:link", kwlist, &LoopType, &loop, &path, &new_path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_link(loop->uv_loop, &fs_req->req, path, new_path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_symlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, flags;
    char *path, *new_path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "new_path", "flags", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssi|O:symlink", kwlist, &LoopType, &loop, &path, &new_path, &flags, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_symlink(loop->uv_loop, &fs_req->req, path, new_path, flags, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_readlink(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:readlink", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_readlink(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static INLINE PyObject *
pyuv__fs_chown(PyObject *args, PyObject *kwargs, int type)
{
    int err, uid, gid;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "uid", "gid", "callback", NULL};

    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sii|O:chown", kwlist, &LoopType, &loop, &path, &uid, &gid, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    if (type == UV_FS_CHOWN) {
        err = uv_fs_chown(loop->uv_loop, &fs_req->req, path, uid, gid, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    } else {
        err = uv_fs_lchown(loop->uv_loop, &fs_req->req, path, uid, gid, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    }

    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_chown(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return pyuv__fs_chown(args, kwargs, UV_FS_CHOWN);
}


static PyObject *
FS_func_lchown(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    UNUSED_ARG(obj);
    return pyuv__fs_chown(args, kwargs, UV_FS_LCHOWN);
}


static PyObject *
FS_func_fchown(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, uid, gid;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "uid", "gid", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!lii|O:fchown", kwlist, &LoopType, &loop, &fd, &uid, &gid, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_fchown(loop->uv_loop, &fs_req->req, (uv_file)fd, uid, gid, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_open(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, flags, mode;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "flags", "mode", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sii|O:open", kwlist, &LoopType, &loop, &path, &flags, &mode, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_open(loop->uv_loop, &fs_req->req, path, flags, mode, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_close(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!l|O:close", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_close(loop->uv_loop, &fs_req->req, (uv_file)fd, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_read(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, length;
    int64_t offset;
    long fd;
    char *buf;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "length", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    buf = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!liL|O:read", kwlist, &LoopType, &loop, &fd, &length, &offset, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    buf = PyMem_Malloc(length);
    if (!buf) {
        PyErr_NoMemory();
        Py_DECREF(fs_req);
        return NULL;
    }

    fs_req->buf.base = buf;
    fs_req->buf.len = length;

    err = uv_fs_read(loop->uv_loop, &fs_req->req, (uv_file)fd, &fs_req->buf, 1, offset, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        PyMem_Free(buf);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_write(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    int64_t offset;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;
    Py_buffer view;
    uv_buf_t buf;

    static char *kwlist[] = {"loop", "fd", "write_data", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ly*L|O:write", kwlist, &LoopType, &loop, &fd, &view, &offset, &callback)) {
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        PyBuffer_Release(&view);
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        PyBuffer_Release(&view);
        Py_DECREF(fs_req);
        return NULL;
    }

    memcpy(&fs_req->view, &view, sizeof(Py_buffer));
    buf = uv_buf_init(fs_req->view.buf, fs_req->view.len);

    err = uv_fs_write(loop->uv_loop, &fs_req->req, (uv_file)fd, &buf, 1, offset, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        PyBuffer_Release(&fs_req->view);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        /* buffer view is released in pyuv__process_fs_req */
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_fsync(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!l|O:fsync", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_fsync(loop->uv_loop, &fs_req->req, (uv_file)fd, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_fdatasync(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!l|O:fdatasync", kwlist, &LoopType, &loop, &fd, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_fdatasync(loop->uv_loop, &fs_req->req, (uv_file)fd, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_ftruncate(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    int64_t offset;
    long fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "offset", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!lL|O:ftruncate", kwlist, &LoopType, &loop, &fd, &offset, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_ftruncate(loop->uv_loop, &fs_req->req, (uv_file)fd, offset, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_scandir(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:scandir", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_scandir(loop->uv_loop, &fs_req->req, path, 0, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_sendfile(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, length;
    int64_t in_offset;
    long out_fd, in_fd;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "out_fd", "in_fd", "in_offset", "length", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!llLi|O:sendfile", kwlist, &LoopType, &loop, &out_fd, &in_fd, &in_offset, &length, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_sendfile(loop->uv_loop, &fs_req->req, (uv_file)out_fd, (uv_file)in_fd, in_offset, length, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_utime(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    double atime, mtime;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "atime", "mtime", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!sdd|O:utime", kwlist, &LoopType, &loop, &path, &atime, &mtime, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_utime(loop->uv_loop, &fs_req->req, path, atime, mtime, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_futime(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    long fd;
    double atime, mtime;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "fd", "atime", "mtime", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ldd|O:futime", kwlist, &LoopType, &loop, &fd, &atime, &mtime, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_futime(loop->uv_loop, &fs_req->req, (uv_file)fd, atime, mtime, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_access(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, flags;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "flags", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!si|O:access", kwlist, &LoopType, &loop, &path, &flags, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_access(loop->uv_loop, &fs_req->req, path, flags, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_realpath(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err;
    char *path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!s|O:realpath", kwlist, &LoopType, &loop, &path, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_realpath(loop->uv_loop, &fs_req->req, path, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
}


static PyObject *
FS_func_copyfile(PyObject *obj, PyObject *args, PyObject *kwargs)
{
    int err, flags;
    char *path, *new_path;
    Loop *loop;
    FSRequest *fs_req;
    PyObject *callback, *ret;

    static char *kwlist[] = {"loop", "path", "new_path", "flags", "callback", NULL};

    UNUSED_ARG(obj);
    fs_req = NULL;
    callback = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!ssi|O:copyfile", kwlist, &LoopType, &loop, &path, &new_path, &flags, &callback)) {
        return NULL;
    }

    if (callback != Py_None && !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "a callable is required");
        return NULL;
    }

    fs_req = (FSRequest *)PyObject_CallFunctionObjArgs((PyObject *)&FSRequestType, loop, callback, NULL);
    if (!fs_req) {
        return NULL;
    }

    err = uv_fs_copyfile(loop->uv_loop, &fs_req->req, path, new_path, flags, (callback != Py_None) ? pyuv__process_fs_req : NULL);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_FSError);
        Py_DECREF(fs_req);
        return NULL;
    }

    Py_INCREF(fs_req);
    if (callback != Py_None) {
        /* No need to cleanup, it will be done in the callback */
        return (PyObject *)fs_req;
    } else {
        pyuv__process_fs_req(&fs_req->req);
        Py_INCREF(fs_req->result);
        ret = fs_req->result;
        Py_DECREF(fs_req);
        return ret;
    }
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
    { "lchown", (PyCFunction)FS_func_lchown, METH_VARARGS|METH_KEYWORDS, "Change file ownership." },
    { "fchown", (PyCFunction)FS_func_fchown, METH_VARARGS|METH_KEYWORDS, "Change file ownership." },
    { "open", (PyCFunction)FS_func_open, METH_VARARGS|METH_KEYWORDS, "Open file." },
    { "close", (PyCFunction)FS_func_close, METH_VARARGS|METH_KEYWORDS, "Close file." },
    { "read", (PyCFunction)FS_func_read, METH_VARARGS|METH_KEYWORDS, "Read data from a file." },
    { "write", (PyCFunction)FS_func_write, METH_VARARGS|METH_KEYWORDS, "Write data to a file." },
    { "fsync", (PyCFunction)FS_func_fsync, METH_VARARGS|METH_KEYWORDS, "Sync all changes made to a file." },
    { "fdatasync", (PyCFunction)FS_func_fdatasync, METH_VARARGS|METH_KEYWORDS, "Sync data changes made to a file." },
    { "ftruncate", (PyCFunction)FS_func_ftruncate, METH_VARARGS|METH_KEYWORDS, "Truncate the contents of a file to the specified offset." },
    { "scandir", (PyCFunction)FS_func_scandir, METH_VARARGS|METH_KEYWORDS, "List files from a directory." },
    { "sendfile", (PyCFunction)FS_func_sendfile, METH_VARARGS|METH_KEYWORDS, "Sends a regular file to a stream socket." },
    { "utime", (PyCFunction)FS_func_utime, METH_VARARGS|METH_KEYWORDS, "Update file times." },
    { "futime", (PyCFunction)FS_func_futime, METH_VARARGS|METH_KEYWORDS, "Update file times." },
    { "access", (PyCFunction)FS_func_access, METH_VARARGS|METH_KEYWORDS, "Check access to file." },
    { "realpath", (PyCFunction)FS_func_realpath, METH_VARARGS|METH_KEYWORDS, "Returns the canonicalized absolute path." },
    { "copyfile", (PyCFunction)FS_func_copyfile, METH_VARARGS|METH_KEYWORDS, "Copies a file to another destination." },
    { NULL }
};


static PyModuleDef pyuv_fs_module = {
    PyModuleDef_HEAD_INIT,
    "pyuv._cpyuv.fs",       /*m_name*/
    NULL,                   /*m_doc*/
    -1,                     /*m_size*/
    FS_methods,             /*m_methods*/
};

PyObject *
init_fs(void)
{
    PyObject *module;
    module = PyModule_Create(&pyuv_fs_module);
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddIntMacro(module, UV_RENAME);
    PyModule_AddIntMacro(module, UV_CHANGE);
    PyModule_AddIntMacro(module, UV_FS_EVENT_WATCH_ENTRY);
    PyModule_AddIntMacro(module, UV_FS_EVENT_STAT);
    PyModule_AddIntMacro(module, UV_FS_SYMLINK_DIR);
    PyModule_AddIntMacro(module, UV_FS_SYMLINK_JUNCTION);
    PyModule_AddIntMacro(module, UV_DIRENT_UNKNOWN);
    PyModule_AddIntMacro(module, UV_DIRENT_FILE);
    PyModule_AddIntMacro(module, UV_DIRENT_DIR);
    PyModule_AddIntMacro(module, UV_DIRENT_LINK);
    PyModule_AddIntMacro(module, UV_DIRENT_FIFO);
    PyModule_AddIntMacro(module, UV_DIRENT_SOCKET);
    PyModule_AddIntMacro(module, UV_DIRENT_CHAR);
    PyModule_AddIntMacro(module, UV_DIRENT_BLOCK);
    PyModule_AddIntMacro(module, UV_FS_COPYFILE_EXCL);
    PyModule_AddIntMacro(module, UV_FS_COPYFILE_FICLONE);
    PyModule_AddIntMacro(module, UV_FS_COPYFILE_FICLONE_FORCE);

    FSEventType.tp_base = &HandleType;
    FSPollType.tp_base = &HandleType;

    PyUVModule_AddType(module, "FSEvent", &FSEventType);
    PyUVModule_AddType(module, "FSPoll", &FSPollType);

    /* initialize PyStructSequence types */
    if (StatResultType.tp_name == 0)
        PyStructSequence_InitType(&StatResultType, &stat_result_desc);
    if (DirEntType.tp_name == 0)
        PyStructSequence_InitType(&DirEntType, &dirent_desc);

    return module;
}

