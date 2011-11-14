
#ifndef PYUV_H
#define PYUV_H

/* python */
#include "Python.h"
#include "structmember.h"

/* system */
#include <arpa/inet.h>
#include <sys/socket.h>

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


/* Python types definitions */
/* Loop */
typedef struct {
    PyObject_HEAD
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
    Bool initialized;
    Bool closed;
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
    Bool initialized;
    Bool closed;
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
    Bool initialized;
    Bool closed;
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
    Bool initialized;
    Bool closed;
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
    Bool initialized;
    Bool closed;
} Check;

static PyTypeObject CheckType;

/* Signal */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_close_cb;
    uv_prepare_t *uv_handle;
    Bool initialized;
    Bool closed;
} Signal;

static PyTypeObject SignalType;

/* IOStream */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_read_cb;
    PyObject *on_close_cb;
    uv_stream_t *uv_handle;
    Bool initialized;
    Bool closed;
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

/* UDP */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_read_cb;
    PyObject *on_close_cb;
    uv_udp_t *uv_handle;
    Bool initialized;
    Bool closed;
} UDP;

static PyTypeObject UDPType;

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
    Bool initialized;
} ThreadPool;

static PyTypeObject ThreadPoolType;

#endif


