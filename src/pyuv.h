
#ifndef PYUV_H
#define PYUV_H

/* python */
#include "Python.h"
#include "structmember.h"

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

#define ASSERT(x)                                                           \
    do {                                                                    \
        if (!(x)) {                                                         \
            fprintf (stderr, "%s:%u: %s: Assertion `" #x "' failed.\n",     \
                     __FILE__, __LINE__, __func__);                         \
            abort();                                                        \
        }                                                                   \
    } while(0)                                                              \

#define UV_LOOP(x) x->loop->uv_loop

#if defined(__MINGW32__) || defined(_MSC_VER)
    #define PYUV_WINDOWS
#endif


/* Python types definitions */

/* Loop */
typedef struct {
    PyObject_HEAD
    PyObject *data;
    uv_loop_t *uv_loop;
    int is_default;
} Loop;

static PyTypeObject LoopType;

/* Async */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *callback;
    PyObject *data;
    uv_async_t *uv_handle;
} Async;

static PyTypeObject AsyncType;

/* Timer */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *callback;
    PyObject *data;
    uv_timer_t *uv_handle;
} Timer;

static PyTypeObject TimerType;

/* Prepare */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *callback;
    PyObject *data;
    uv_prepare_t *uv_handle;
} Prepare;

static PyTypeObject PrepareType;

/* Idle */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *callback;
    PyObject *data;
    uv_idle_t *uv_handle;
} Idle;

static PyTypeObject IdleType;

/* Check */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *callback;
    PyObject *data;
    uv_check_t *uv_handle;
} Check;

static PyTypeObject CheckType;

/* Signal */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *data;
    uv_prepare_t *uv_handle;
} Signal;

static PyTypeObject SignalType;

/* IOStream */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_read_cb;
    PyObject *on_close_cb;
    PyObject *data;
    uv_stream_t *uv_handle;
} IOStream;

static PyTypeObject IOStreamType;

/* TCP */
typedef struct {
    IOStream iostream;
    PyObject *on_new_connection_cb;
} TCP;

static PyTypeObject TCPType;

/* Pipe */
typedef struct {
    IOStream iostream;
    PyObject *on_new_connection_cb;
} Pipe;

static PyTypeObject PipeType;

/* TTY */
typedef struct {
    IOStream iostream;
} TTY;

static PyTypeObject TTYType;

/* UDP */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_read_cb;
    PyObject *on_close_cb;
    PyObject *data;
    uv_udp_t *uv_handle;
} UDP;

static PyTypeObject UDPType;

/* DNSResolver */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *data;
    ares_channel channel;
} DNSResolver;

static PyTypeObject DNSResolverType;

/* ThreadPool */
typedef struct {
    PyObject_HEAD
} ThreadPool;

static PyTypeObject ThreadPoolType;

/* Process */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    PyObject *on_exit_cb;
    PyObject *stdin_pipe;
    PyObject *stdout_pipe;
    PyObject *stderr_pipe;
    PyObject *data;
    uv_process_t *uv_handle;
} Process;

static PyTypeObject ProcessType;

/* FSEvent */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_fsevent_cb;
    PyObject *on_close_cb;
    PyObject *data;
    uv_fs_event_t *uv_handle;
} FSEvent;

static PyTypeObject FSEventType;

#endif


