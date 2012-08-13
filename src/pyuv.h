
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

/* DNSResolver */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    ares_channel channel;
} DNSResolver;

static PyTypeObject DNSResolverType;

/* ThreadPool */
typedef struct {
    PyObject_HEAD
    Loop *loop;
} ThreadPool;

static PyTypeObject ThreadPoolType;


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


