
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
    uv_timer_t timer;
    Pipe *pipe;
    PyObject *callback;
} abstract_connect_req;


static void
pyuv__deallocate_handle_data(uv_handle_t *handle)
{
    PyMem_Free(handle->data);
}


static void
pyuv__pipe_connect_abstract_cb(uv_timer_t *timer)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *result, *error;
    abstract_connect_req *req;

    ASSERT(timer != NULL);
    req = (abstract_connect_req *) timer->data;

    error = Py_None;
    Py_INCREF(error);

    result = PyObject_CallFunctionObjArgs(req->callback, req->pipe, error, NULL);
    if (result == NULL) {
        handle_uncaught_exception(HANDLE(req->pipe)->loop);
    }

    Py_XDECREF(result);
    Py_DECREF(error);

    Py_DECREF(req->callback);
    Py_DECREF(req->pipe);

    uv_close((uv_handle_t *) &req->timer, pyuv__deallocate_handle_data);

    PyGILState_Release(gstate);
}


static PyObject *
Pipe_func_bind_abstract(Pipe *self, const char *name, int len)
{
    int fd = -1, err;
    struct sockaddr_un saddr;

    err = socket(AF_UNIX, SOCK_STREAM, 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(-errno, PyExc_PipeError);
        goto error;
    }
    fd = err;

    /* Clip overly long paths, mimics libuv behavior. */

    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    if (len >= (int) sizeof(saddr.sun_path))
        len = sizeof(saddr.sun_path) - 1;
    memcpy(saddr.sun_path, name, len);

    err = bind(fd, (struct sockaddr *) &saddr, sizeof(saddr.sun_family) + len);
    if (err < 0) {
        RAISE_UV_EXCEPTION(-errno, PyExc_PipeError);
        goto error;
    }

    /* uv_pipe_open() puts the fd in non-blocking mode so no need to do that
     * ourselves. */

    err = uv_pipe_open(&self->pipe_h, fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        goto error;
    }

    Py_INCREF(Py_None);
    return Py_None;

error:
    if (fd != -1)
        close(fd);

    return NULL;
}


static PyObject *
Pipe_func_connect_abstract(Pipe *self, const char *name, int len, PyObject *callback)
{
    int fd = -1, err;
    struct sockaddr_un saddr;
    abstract_connect_req *req = NULL;

    err = socket(AF_UNIX, SOCK_STREAM, 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(-errno, PyExc_PipeError);
        goto error;
    }
    fd = err;

    /* This is a local (AF_UNIX) socket and connect() on Linux is not
     * supposed to block as far as I can tell. And even if it would block for a
     * very small amount of time that would be OK. */

    if (len >= (int) sizeof(saddr.sun_path))
        len = sizeof(saddr.sun_path) - 1;
    saddr.sun_family = AF_UNIX;
    memset(saddr.sun_path, 0, sizeof(saddr.sun_path));
    memcpy(saddr.sun_path, name, len);
    saddr.sun_path[len] = '\0';

    err = connect(fd, (struct sockaddr *) &saddr, sizeof(saddr.sun_family) + len);
    if (err < 0) {
        RAISE_UV_EXCEPTION(-errno, PyExc_PipeError);
        goto error;
    }

    err = uv_pipe_open(&self->pipe_h, fd);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        goto error;
    }
    fd = -1;

    /* Now we need to fire the callback. We can't just call it here as it's
     * expected to be fired from the event loop. Use a zero-timeout uv_timer to
     * schedule it for the next loop iteration. */

    req = PyMem_Malloc(sizeof(abstract_connect_req));
    if (req == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    err = uv_timer_init(UV_HANDLE_LOOP(self), &req->timer);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        goto error;
    }
    req->pipe = self;
    req->callback = callback;
    req->timer.data = req;

    Py_INCREF(req->pipe);
    Py_INCREF(req->callback);

    err = uv_timer_start(&req->timer, pyuv__pipe_connect_abstract_cb, 0, 0);
    if (err < 0) {
        RAISE_UV_EXCEPTION(err, PyExc_PipeError);
        goto error;
    }

    Py_INCREF(Py_None);
    return Py_None;

error:
    if (fd != -1)
        close(fd);
    if (req != NULL)
        PyMem_Free(req);

    return NULL;
}
