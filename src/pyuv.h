
#ifndef PYUV_H
#define PYUV_H

/* python */
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "structseq.h"
#include "bytesobject.h"

/* Python3 */
#if PY_MAJOR_VERSION >= 3
    #define PYUV_PYTHON3
    #define PyInt_FromSsize_t PyLong_FromSsize_t
    #define PyInt_FromLong PyLong_FromLong
    #define PYUV_BYTES "y"
#else
    #define PYUV_BYTES "s"
#endif

/* libuv */
#include "uv.h"


/* Custom types */
typedef int Bool;
#define True  1
#define False 0


/* Utility macros */
#define PYUV_STRINGIFY_HELPER(x) #x
#define PYUV_STRINGIFY(x) PYUV_STRINGIFY_HELPER(x)

#define PYUV_CONTAINER_OF(ptr, type, field)                                  \
      ((type *) ((char *) (ptr) - ((long) &((type *) 0)->field)))

#define UNUSED_ARG(arg)  (void)arg

#if defined(__MINGW32__) || defined(_MSC_VER)
    #define PYUV_WINDOWS
    #define PYUV_MAXSTDIO 2048
#endif

#ifdef _MSC_VER
    #define INLINE __inline
#else
    #define INLINE inline
#endif

#define ASSERT(x)                                                           \
    do {                                                                    \
        if (!(x)) {                                                         \
            fprintf (stderr, "%s:%u: Assertion `" #x "' failed.\n",         \
                     __FILE__, __LINE__);                                   \
            abort();                                                        \
        }                                                                   \
    } while(0)                                                              \

#define HANDLE(x) ((Handle *)x)

#define UV_LOOP(x) (x)->loop->uv_loop

#define UV_HANDLE(x) HANDLE(x)->uv_handle

#define UV_HANDLE_LOOP(x) UV_LOOP(HANDLE(x))

#define REQUEST(x) ((Request *)x)

#define UV_REQUEST(x) REQUEST(x)->req_ptr

#define RAISE_IF_INITIALIZED(obj, retval)                                           \
    do {                                                                            \
        if ((obj)->initialized) {                                                   \
            PyErr_SetString(PyExc_RuntimeError, "Object was already initialized");  \
            return retval;                                                          \
        }                                                                           \
    } while(0)                                                                      \

#define RAISE_IF_NOT_INITIALIZED(obj, retval)                                                             \
    do {                                                                                                  \
        if (!((obj)->initialized)) {                                                                      \
            PyErr_SetString(PyExc_RuntimeError, "Object was not initialized, forgot to call __init__?");  \
            return retval;                                                                                \
        }                                                                                                 \
    } while(0)                                                                                            \

#define RAISE_IF_HANDLE_INITIALIZED(obj, retval)      RAISE_IF_INITIALIZED((Handle *)obj, retval)

#define RAISE_IF_HANDLE_NOT_INITIALIZED(obj, retval)  RAISE_IF_NOT_INITIALIZED((Handle *)obj, retval)

#define RAISE_IF_HANDLE_CLOSED(obj, exc_type, retval)                       \
    do {                                                                    \
        if (uv_is_closing(UV_HANDLE(obj))) {                                \
            PyErr_SetString(exc_type, "Handle is closing/closed");          \
            return retval;                                                  \
        }                                                                   \
    } while(0)                                                              \

#define RAISE_UV_EXCEPTION(loop, exc_type)                                          \
    do {                                                                            \
        uv_err_t err = uv_last_error(loop);                                         \
        PyObject *exc_data = Py_BuildValue("(is)", err.code, uv_strerror(err));     \
        if (exc_data != NULL) {                                                     \
            PyErr_SetObject(exc_type, exc_data);                                    \
            Py_DECREF(exc_data);                                                    \
        }                                                                           \
    } while(0)                                                                      \

#define RAISE_STREAM_EXCEPTION(handle)                                              \
    do {                                                                            \
        PyObject *exc_type;                                                         \
        switch ((handle)->type) {                                                   \
            case UV_TCP:                                                            \
                exc_type = PyExc_TCPError;                                          \
                break;                                                              \
            case UV_NAMED_PIPE:                                                     \
                exc_type = PyExc_PipeError;                                         \
                break;                                                              \
            case UV_TTY:                                                            \
                exc_type = PyExc_TTYError;                                          \
                break;                                                              \
            default:                                                                \
                ASSERT(0 && "invalid stream handle type");                          \
        }                                                                           \
        RAISE_UV_EXCEPTION((handle)->loop, exc_type);                               \
    } while(0)                                                                      \

#define PYUV_SLAB_SIZE 65536


/* Python types definitions */

/* Loop */
typedef struct {
    PyObject_HEAD
    PyObject *excepthook_cb;
    PyObject *weakreflist;
    PyObject *dict;
    uv_loop_t *uv_loop;
    int is_default;
} Loop;

static PyTypeObject LoopType;

/* Handle */
typedef struct {
    PyObject_HEAD
    uv_handle_t *uv_handle;
    Bool initialized;
    PyObject *weakreflist;
    PyObject *dict;
    Loop *loop;
    PyObject *on_close_cb;
} Handle;

static PyTypeObject HandleType;

/* Async */
typedef struct {
    Handle handle;
    uv_async_t async_h;
    PyObject *callback;
} Async;

static PyTypeObject AsyncType;

/* Timer */
typedef struct {
    Handle handle;
    uv_timer_t timer_h;
    PyObject *callback;
} Timer;

static PyTypeObject TimerType;

/* Prepare */
typedef struct {
    Handle handle;
    uv_prepare_t prepare_h;
    PyObject *callback;
} Prepare;

static PyTypeObject PrepareType;

/* Idle */
typedef struct {
    Handle handle;
    uv_idle_t idle_h;
    PyObject *callback;
} Idle;

static PyTypeObject IdleType;

/* Check */
typedef struct {
    Handle handle;
    uv_check_t check_h;
    PyObject *callback;
} Check;

static PyTypeObject CheckType;

/* Signal */
typedef struct {
    Handle handle;
    uv_signal_t signal_h;
    PyObject *callback;
} Signal;

static PyTypeObject SignalType;

/* SignalChecker */
typedef struct {
    Handle handle;
    uv_poll_t poll_h;
    long fd;
} SignalChecker;

static PyTypeObject SignalCheckerType;

/* Stream */
typedef struct {
    Handle handle;
    PyObject *on_read_cb;
} Stream;

static PyTypeObject StreamType;

/* TCP */
typedef struct {
    Stream stream;
    uv_tcp_t tcp_h;
    PyObject *on_new_connection_cb;
} TCP;

static PyTypeObject TCPType;

/* Pipe */
typedef struct {
    Stream stream;
    uv_pipe_t pipe_h;
    PyObject *on_new_connection_cb;
} Pipe;

static PyTypeObject PipeType;

/* TTY */
typedef struct {
    Stream stream;
    uv_tty_t tty_h;
} TTY;

static PyTypeObject TTYType;

/* UDP */
typedef struct {
    Handle handle;
    uv_udp_t udp_h;
    PyObject *on_read_cb;
} UDP;

static PyTypeObject UDPType;

/* Poll */
typedef struct {
    Handle handle;
    uv_poll_t poll_h;
    PyObject *callback;
    long fd;
} Poll;

static PyTypeObject PollType;

/* Process */
typedef struct {
    PyObject_HEAD
    PyObject *stream;
    int fd;
    int flags;
} StdIO;

static PyTypeObject StdIOType;

typedef struct {
    Handle handle;
    Bool spawned;
    uv_process_t process_h;
    PyObject *on_exit_cb;
    PyObject *stdio;
} Process;

static PyTypeObject ProcessType;

/* FSEvent */
typedef struct {
    Handle handle;
    uv_fs_event_t fsevent_h;
    PyObject *callback;
} FSEvent;

static PyTypeObject FSEventType;

/* FSPoll */
typedef struct {
    Handle handle;
    uv_fs_poll_t fspoll_h;
    PyObject *callback;
} FSPoll;

static PyTypeObject FSPollType;

/* Barrier */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_barrier_t uv_barrier;
} Barrier;

static PyTypeObject BarrierType;

/* Condition */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_cond_t uv_condition;
} Condition;

static PyTypeObject ConditionType;

/* Mutex */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_mutex_t uv_mutex;
} Mutex;

static PyTypeObject MutexType;

/* RWLock */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_rwlock_t uv_rwlock;
} RWLock;

static PyTypeObject RWLockType;

/* Semaphore */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_sem_t uv_semaphore;
} Semaphore;

static PyTypeObject SemaphoreType;

/* Request */
typedef struct {
    PyObject_HEAD
    Bool initialized;
    uv_req_t *req_ptr;
    Loop *loop;
} Request;

static PyTypeObject RequestType;

/* GAIRequest */
typedef struct {
    Request request;
    uv_getaddrinfo_t req;
    PyObject *callback;
} GAIRequest;

static PyTypeObject GAIRequestType;

/* WorkRequest */
typedef struct {
    Request request;
    uv_work_t req;
    PyObject *work_cb;
    PyObject *done_cb;
} WorkRequest;

static PyTypeObject WorkRequestType;

/* FSRequest */
typedef struct {
    Request request;
    uv_fs_t req;
    PyObject *callback;
} FSRequest;

static PyTypeObject FSRequestType;


/* Exceptions */
static PyObject* PyExc_AsyncError;
static PyObject* PyExc_CheckError;
static PyObject* PyExc_FSError;
static PyObject* PyExc_FSEventError;
static PyObject* PyExc_FSPollError;
static PyObject* PyExc_HandleError;
static PyObject* PyExc_HandleClosedError;
static PyObject* PyExc_IdleError;
static PyObject* PyExc_PipeError;
static PyObject* PyExc_PollError;
static PyObject* PyExc_PrepareError;
static PyObject* PyExc_ProcessError;
static PyObject* PyExc_SignalError;
static PyObject* PyExc_StreamError;
static PyObject* PyExc_TCPError;
static PyObject* PyExc_ThreadError;
static PyObject* PyExc_TimerError;
static PyObject* PyExc_TTYError;
static PyObject* PyExc_UDPError;
static PyObject* PyExc_UVError;


/* PyStructSequence types */

/* used by getaddrinfo */
static PyTypeObject AddrinfoResultType;

static PyStructSequence_Field addrinfo_result_fields[] = {
    {"family", ""},
    {"socktype", ""},
    {"proto", ""},
    {"canonname", ""},
    {"sockaddr", ""},
    {NULL}
};

static PyStructSequence_Desc addrinfo_result_desc = {
    "addrinfo_result",
    NULL,
    addrinfo_result_fields,
    5
};


/* used by fs stat functions */
static PyTypeObject StatResultType;

static PyStructSequence_Field stat_result_fields[] = {
    {"st_mode",        "protection bits"},
    {"st_ino",         "inode"},
    {"st_dev",         "device"},
    {"st_nlink",       "number of hard links"},
    {"st_uid",         "user ID of owner"},
    {"st_gid",         "group ID of owner"},
    {"st_size",        "total size, in bytes"},
    {"st_atime",       "time of last access"},
    {"st_mtime",       "time of last modification"},
    {"st_ctime",       "time of last change"},
    {"st_blksize",     "blocksize for filesystem I/O"},
    {"st_blocks",      "number of blocks allocated"},
    {"st_rdev",        "device type (if inode device)"},
    {"st_flags",       "user defined flags for file"},
    {"st_gen",         "generation number"},
    {"st_birthtime",   "time of creation"},
    {NULL}
};

static PyStructSequence_Desc stat_result_desc = {
    "stat_result",
    NULL,
    stat_result_fields,
    16
};


/* used by interface_addresses */
static PyTypeObject InterfaceAddressesResultType;

static PyStructSequence_Field interface_addresses_result_fields[] = {
    {"name", ""},
    {"is_internal", ""},
    {"address", ""},
    {NULL}
};

static PyStructSequence_Desc interface_addresses_result_desc = {
    "interface_addresses_result",
    NULL,
    interface_addresses_result_fields,
    3
};


/* used by cpu_info */
static PyTypeObject CPUInfoResultType;

static PyStructSequence_Field cpu_info_result_fields[] = {
    {"model", ""},
    {"speed", ""},
    {"times", ""},
    {NULL}
};

static PyStructSequence_Desc cpu_info_result_desc = {
    "cpu_info_result",
    NULL,
    cpu_info_result_fields,
    3
};


/* used by cpu_info */
static PyTypeObject CPUInfoTimesResultType;

static PyStructSequence_Field cpu_info_times_result_fields[] = {
    {"sys", ""},
    {"user", ""},
    {"idle", ""},
    {"irq", ""},
    {"nice", ""},
    {NULL}
};

static PyStructSequence_Desc cpu_info_times_result_desc = {
    "cpu_info_times_result",
    NULL,
    cpu_info_times_result_fields,
    5
};


/* Some helper stuff */


/* Add a type to a module */
static int
PyUVModule_AddType(PyObject *module, const char *name, PyTypeObject *type)
{
    if (PyType_Ready(type)) {
        return -1;
    }
    Py_INCREF(type);
    if (PyModule_AddObject(module, name, (PyObject *)type)) {
        Py_DECREF(type);
        return -1;
    }
    return 0;
}


/* Add a type to a module */
static int
PyUVModule_AddObject(PyObject *module, const char *name, PyObject *value)
{
    Py_INCREF(value);
    if (PyModule_AddObject(module, name, value)) {
        Py_DECREF(value);
        return -1;
    }
    return 0;
}


static INLINE int
pyseq2uvbuf(PyObject *seq, Py_buffer **rviews, uv_buf_t **rbufs, int *rbuf_count)
{
    int i, j, buf_count;
    uv_buf_t *uv_bufs = NULL;
    Py_buffer *views = NULL;
    PyObject *data_fast = NULL;

    i = 0;
    *rviews = NULL;
    *rbufs = NULL;
    *rbuf_count = 0;

    if ((data_fast = PySequence_Fast(seq, "argument 1 must be an iterable")) == NULL) {
        goto error;
    }

    buf_count = PySequence_Fast_GET_SIZE(data_fast);
    if (buf_count > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "argument 1 is too long");
        goto error;
    }

    if (buf_count == 0) {
        PyErr_SetString(PyExc_ValueError, "argument 1 is empty");
        goto error;
    }

    uv_bufs = PyMem_Malloc(sizeof *uv_bufs * buf_count);
    views = PyMem_Malloc(sizeof *views * buf_count);
    if (!uv_bufs || !views) {
        PyErr_NoMemory();
        goto error;
    }

    for (i = 0; i < buf_count; i++) {
        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(data_fast, i), PYUV_BYTES"*;argument 1 must be an iterable of buffer-compatible objects", &views[i])) {
            goto error;
        }
        uv_bufs[i].base = views[i].buf;
        uv_bufs[i].len = views[i].len;
    }

    *rviews = views;
    *rbufs = uv_bufs;
    *rbuf_count = buf_count;
    Py_XDECREF(data_fast);
    return 0;

error:
    for (j = 0; j < i; j++) {
        PyBuffer_Release(&views[j]);
    }
    PyMem_Free(views);
    PyMem_Free(uv_bufs);
    Py_XDECREF(data_fast);
    return -1;
}


/* parse a Python tuple containing host, port, flowinfo, scope_id into a sockaddr struct */
static int
pyuv_parse_addr_tuple(PyObject *addr, struct sockaddr_storage *ss)
{
    char *host;
    int port;
    unsigned int scope_id, flowinfo;
    struct in_addr addr4;
    struct in6_addr addr6;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;

    flowinfo = scope_id = 0;

    if (!PyTuple_Check(addr)) {
        PyErr_Format(PyExc_TypeError, "address must be tuple, not %.500s", Py_TYPE(addr)->tp_name);
        return -1;
    }

    if (!PyArg_ParseTuple(addr, "si|II", &host, &port, &flowinfo, &scope_id)) {
        return -1;
    }

    if (port < 0 || port > 0xffff) {
        PyErr_SetString(PyExc_OverflowError, "port must be 0-65535");
        return -1;
    }

    if (flowinfo > 0xfffff) {
        PyErr_SetString(PyExc_OverflowError, "flowinfo must be 0-1048575");
        return -1;
    }

    memset(ss, 0, sizeof(struct sockaddr_storage));

    if (uv_inet_pton(AF_INET, host, &addr4).code == UV_OK) {
        /* it's an IPv4 address */
        sa4 = (struct sockaddr_in *)ss;
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons((short)port);
        sa4->sin_addr = addr4;
        return 0;
    } else if (uv_inet_pton(AF_INET6, host, &addr6).code == UV_OK) {
        /* it's an IPv6 address */
        sa6 = (struct sockaddr_in6 *)ss;
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons((short)port);
        sa6->sin6_addr = addr6;
        sa6->sin6_flowinfo = flowinfo;
        sa6->sin6_scope_id = scope_id;
        return 0;
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid IP address");
        return -1;
    }
}


/* Modified from Python Modules/socketmodule.c */
static PyObject *
makesockaddr(struct sockaddr *addr, int addrlen)
{
    static char buf[INET6_ADDRSTRLEN+1];
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    if (addrlen == 0) {
        /* No address */
        Py_RETURN_NONE;
    }

    switch (addr->sa_family) {
    case AF_INET:
    {
        addr4 = (struct sockaddr_in*)addr;
        uv_ip4_name(addr4, buf, sizeof(buf));
        return Py_BuildValue("si", buf, ntohs(addr4->sin_port));
    }

    case AF_INET6:
    {
        addr6 = (struct sockaddr_in6*)addr;
        uv_ip6_name(addr6, buf, sizeof(buf));
        return Py_BuildValue("siII", buf, ntohs(addr6->sin6_port), ntohl(addr6->sin6_flowinfo), addr6->sin6_scope_id);
    }

    default:
        /* If we don't know the address family, don't raise an exception -- return it as a tuple. */
        return Py_BuildValue("is#", addr->sa_family, addr->sa_data, sizeof(addr->sa_data));
    }
}


/* handle uncausht exception in a callback */
static INLINE void
handle_uncaught_exception(Loop *loop)
{
    PyObject *type, *val, *tb, *result;

    ASSERT(loop);
    ASSERT(PyErr_Occurred());

    if (loop->excepthook_cb != NULL && loop->excepthook_cb != Py_None) {
        PyErr_Fetch(&type, &val, &tb);
        PyErr_NormalizeException(&type, &val, &tb);

        if (!val) {
            val = Py_None;
            Py_INCREF(Py_None);
        }
        if (!tb) {
            tb = Py_None;
            Py_INCREF(Py_None);
        }

        result = PyObject_CallFunctionObjArgs(loop->excepthook_cb, type, val, tb, NULL);
        if (result == NULL) {
            PyErr_Print();
        }
        Py_XDECREF(result);

        Py_DECREF(type);
        Py_DECREF(val);
        Py_DECREF(tb);

        /* just in case*/
        PyErr_Clear();
    } else {
        PyErr_Print();
    }

}


#endif


