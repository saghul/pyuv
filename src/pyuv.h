
#ifndef PYUV_H
#define PYUV_H

/* python */
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "structseq.h"

/* Python3 */
#if PY_MAJOR_VERSION >= 3
    #define PYUV_PYTHON3
    #define PyInt_FromSsize_t PyLong_FromSsize_t
    #define PyInt_FromLong PyLong_FromLong
    #define PyString_FromString PyBytes_FromString
    #define PyString_AsString PyBytes_AsString
    #define PyString_FromStringAndSize PyBytes_FromStringAndSize
    #define PyString_Check PyBytes_Check
    #define PyString_Size PyBytes_Size
    #define PyString_AS_STRING PyBytes_AS_STRING
    #define PyString_GET_SIZE PyBytes_GET_SIZE
    /* helpers, to avoid too many ifdefs */
    #define PYUVString_FromString PyUnicode_FromString
    #define PYUVString_FromStringAndSize PyUnicode_FromStringAndSize
#else
    /* helpers, to avoid too many ifdefs */
    #define PYUVString_FromString PyString_FromString
    #define PYUVString_FromStringAndSize PyString_FromStringAndSize
#endif

/* libuv */
#include "uv.h"


/* Custom types */
typedef int Bool;
#define True  1
#define False 0


/* Utility macros */
#ifndef __STRING
    #define __STRING(x) #x
#endif
#define __MSTR(x) __STRING(x)

#define UNUSED_ARG(arg)  (void)arg

#if defined(__MINGW32__) || defined(_MSC_VER)
    #define PYUV_WINDOWS
#endif

#define ASSERT(x)                                                           \
    do {                                                                    \
        if (!(x)) {                                                         \
            fprintf (stderr, "%s:%u: %s: Assertion `" #x "' failed.\n",     \
                     __FILE__, __LINE__, __func__);                         \
            abort();                                                        \
        }                                                                   \
    } while(0)                                                              \

#define UV_LOOP(x) (x)->loop->uv_loop

#define UV_HANDLE(x) ((Handle *)x)->uv_handle

#define UV_HANDLE_CLOSED(x) (!UV_HANDLE(x) || uv_is_closing(UV_HANDLE(x)))

#define UV_HANDLE_LOOP(x) UV_LOOP((Handle *)x)

#define RAISE_IF_HANDLE_CLOSED(obj, exc_type, retval)                       \
    do {                                                                    \
        if (UV_HANDLE_CLOSED(obj)) {                                        \
            PyErr_SetString(exc_type, "");                                  \
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


/* Python types definitions */

/* Loop */
typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;
    PyObject *dict;
    uv_loop_t *uv_loop;
    int is_default;
} Loop;

static PyTypeObject LoopType;

/* Handle */
typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;
    PyObject *dict;
    Loop *loop;
    PyObject *on_close_cb;
    uv_handle_t *uv_handle;
} Handle;

static PyTypeObject HandleType;

/* Async */
typedef struct {
    Handle handle;
    PyObject *callback;
} Async;

static PyTypeObject AsyncType;

/* Timer */
typedef struct {
    Handle handle;
    PyObject *callback;
} Timer;

static PyTypeObject TimerType;

/* Prepare */
typedef struct {
    Handle handle;
    PyObject *callback;
} Prepare;

static PyTypeObject PrepareType;

/* Idle */
typedef struct {
    Handle handle;
    PyObject *callback;
} Idle;

static PyTypeObject IdleType;

/* Check */
typedef struct {
    Handle handle;
    PyObject *callback;
} Check;

static PyTypeObject CheckType;

/* Signal */
typedef struct {
    Handle handle;
} Signal;

static PyTypeObject SignalType;

/* Stream */
typedef struct {
    Handle handle;
    PyObject *on_read_cb;
} Stream;

static PyTypeObject StreamType;

/* TCP */
typedef struct {
    Stream stream;
    PyObject *on_new_connection_cb;
} TCP;

static PyTypeObject TCPType;

/* Pipe */
typedef struct {
    Stream stream;
    PyObject *on_new_connection_cb;
} Pipe;

static PyTypeObject PipeType;

/* TTY */
typedef struct {
    Stream stream;
} TTY;

static PyTypeObject TTYType;

/* UDP */
typedef struct {
    Handle handle;
    PyObject *on_read_cb;
} UDP;

static PyTypeObject UDPType;

/* Poll */
typedef struct {
    Handle handle;
    PyObject *callback;
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
    PyObject *on_exit_cb;
    PyObject *stdio;
} Process;

static PyTypeObject ProcessType;

/* FSEvent */
typedef struct {
    Handle handle;
    PyObject *callback;
} FSEvent;

static PyTypeObject FSEventType;

/* FSPoll */
typedef struct {
    Handle handle;
    PyObject *callback;
} FSPoll;

static PyTypeObject FSPollType;

/* ThreadPool */
typedef struct {
    PyObject_HEAD
    Loop *loop;
} ThreadPool;

static PyTypeObject ThreadPoolType;


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
static PyObject* PyExc_ThreadPoolError;
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

/* used by Loop.counters */
static PyTypeObject LoopCountersResultType;

static PyStructSequence_Field loop_counters_result_fields[] = {
    {"async_init", ""},
    {"check_init", ""},
    {"eio_init", ""},
    {"fs_event_init", ""},
    {"fs_poll_init", ""},
    {"handle_init", ""},
    {"idle_init", ""},
    {"pipe_init", ""},
    {"poll_init", ""},
    {"prepare_init", ""},
    {"process_init", ""},
    {"req_init", ""},
    {"stream_init", ""},
    {"tcp_init", ""},
    {"timer_init", ""},
    {"tty_init", ""},
    {"udp_init", ""},
    {NULL}
};

static PyStructSequence_Desc loop_counters_result_desc = {
    "loop_counters_result",
    NULL,
    loop_counters_result_fields,
    16
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


/* Some helper stuff */

/* Temporary hack: libuv should provide uv_inet_pton and uv_inet_ntop. */
#ifdef PYUV_WINDOWS
    #include <inet_net_pton.h>
    #include <inet_ntop.h>
    #define uv_inet_pton ares_inet_pton
    #define uv_inet_ntop ares_inet_ntop
#else /* __POSIX__ */
    #include <arpa/inet.h>
    #define uv_inet_pton inet_pton
    #define uv_inet_ntop inet_ntop
#endif


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


/* borrowed from pyev */
#ifdef PYUV_WINDOWS
    #define PYUV_MAXSTDIO 2048
#endif

#endif


