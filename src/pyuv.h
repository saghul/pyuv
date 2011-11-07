
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

#define SELF_LOOP self->loop->uv_loop
#define PARENT_LOOP parent->loop->uv_loop

#define RAISE_ERROR(loop, exc, ret)                 \
    do {                                            \
        uv_err_t err = uv_last_error(loop);         \
        PyErr_SetString(exc, uv_strerror(err));     \
        return ret;                                 \
    } while (0)                                     \


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
    PyObject *callback;
    PyObject *data;
    uv_async_t *uv_async;
    Bool initialized;
    Bool closed;
} Async;

static PyTypeObject AsyncType;

/* Timer */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *callback;
    PyObject *data;
    uv_timer_t *uv_timer;
    Bool initialized;
    Bool closed;
} Timer;

static PyTypeObject TimerType;

/* Prepare */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *callback;
    PyObject *data;
    uv_prepare_t *uv_prepare;
    Bool initialized;
    Bool closed;
} Prepare;

static PyTypeObject PrepareType;

/* Idle */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *callback;
    PyObject *data;
    uv_idle_t *uv_idle;
    Bool initialized;
    Bool closed;
} Idle;

static PyTypeObject IdleType;

/* Check */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *callback;
    PyObject *data;
    uv_check_t *uv_check;
    Bool initialized;
    Bool closed;
} Check;

static PyTypeObject CheckType;

/* Signal */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    uv_prepare_t *uv_prepare;
    Bool initialized;
    Bool closed;
} Signal;

static PyTypeObject SignalType;

/* IOStream */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_disconnect_cb;
    PyObject *on_read_cb;
    uv_stream_t *uv_stream;
    Bool initialized;
    Bool closed;
} IOStream;

static PyTypeObject IOStreamType;

/* TCPServer */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_new_connection_cb;
    uv_tcp_t *uv_tcp_server;
    Bool initialized;
    Bool closed;
} TCPServer;

static PyTypeObject TCPServerType;

/* TCPClient */
typedef struct {
    IOStream iostream;
} TCPClient;

static PyTypeObject TCPClientType;

/* TCPClientConnection */
typedef struct {
    IOStream iostream;
} TCPClientConnection;

static PyTypeObject TCPClientConnectionType;

/* UDPConnection */
typedef struct {
    PyObject_HEAD
    Loop *loop;
    PyObject *on_read_cb;
    uv_udp_t *uv_udp_handle;
    Bool bound;
    Bool initialized;
} UDPConnection;

static PyTypeObject UDPConnectionType;

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


