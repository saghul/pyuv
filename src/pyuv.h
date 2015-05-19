
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
    #define PyInt_Check PyLong_Check
    #define PyInt_AsLong PyLong_AsLong
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

#if defined(_MSC_VER)
# include <malloc.h>
# define STACK_ARRAY(type, name, size) type* name = (type*)_alloca((size) * sizeof(type))
#else
# define STACK_ARRAY(type, name, size) type name[size]
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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

#define RAISE_UV_EXCEPTION(err, exc_type)                                           \
    do {                                                                            \
        PyObject *exc_data = Py_BuildValue("(is)", err, uv_strerror(err));          \
        if (exc_data != NULL) {                                                     \
            PyErr_SetObject(exc_type, exc_data);                                    \
            Py_DECREF(exc_data);                                                    \
        }                                                                           \
    } while(0)                                                                      \

#define RAISE_STREAM_EXCEPTION(err, handle)                                         \
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
        RAISE_UV_EXCEPTION(err, exc_type);                                          \
    } while(0)                                                                      \

#define PYUV_SET_NONE(x)     \
    do {                     \
        Py_INCREF(Py_None);  \
        (x) = Py_None;       \
    } while(0)               \

#define PYUV_SLAB_SIZE 65536


/* Custom pyuv handle flags */
#define PYUV__PYREF    (1 << 1)

#define PYUV_HANDLE_INCREF(obj)                        \
    do {                                               \
        if (!(HANDLE(obj)->flags & PYUV__PYREF)) {     \
            HANDLE(obj)->flags |= PYUV__PYREF;         \
            Py_INCREF(obj);                            \
        }                                              \
    } while(0)                                         \

#define PYUV_HANDLE_DECREF(obj)                        \
    do {                                               \
        if (HANDLE(obj)->flags & PYUV__PYREF) {        \
            HANDLE(obj)->flags &= ~PYUV__PYREF;        \
            Py_DECREF(obj);                            \
        }                                              \
    } while(0)                                         \


/* Non-pyuv handles are unlikely to contain that exact data, so this is at least
   somewhat better than a guaranteed SIGSEGV when accessing`loop.handles`. */
#define IS_PYUV_HANDLE(ptr) (ptr && ((Handle*)ptr)->handle_magic == PYUV_HANDLE_MAGIC)
#define PYUV_HANDLE_MAGIC &HandleType

/* Python types definitions */

/* Loop */
typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;
    PyObject *dict;
    uv_loop_t loop_struct;
    uv_loop_t *uv_loop;
    int is_default;
    struct {
        char slab[PYUV_SLAB_SIZE];
        Bool in_use;
    } buffer;
} Loop;

static PyTypeObject LoopType;

/* Handle */
typedef struct {
    PyObject_HEAD
    void *handle_magic;
    uv_handle_t *uv_handle;
    int flags;
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
    PyObject *dict;
} Request;

static PyTypeObject RequestType;

/* GAIRequest */
typedef struct {
    Request request;
    uv_getaddrinfo_t req;
    PyObject *callback;
} GAIRequest;

static PyTypeObject GAIRequestType;

/* GNIRequest */
typedef struct {
    Request request;
    uv_getnameinfo_t req;
    PyObject *callback;
} GNIRequest;

static PyTypeObject GNIRequestType;

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
    PyObject *path;
    PyObject *result;
    PyObject *error;
    /* for write requests */
    Py_buffer view;
    /* for read requests */
    uv_buf_t buf;
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
    {"netmask", ""},
    {"mac", ""},
    {NULL}
};

static PyStructSequence_Desc interface_addresses_result_desc = {
    "interface_addresses_result",
    NULL,
    interface_addresses_result_fields,
    5
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


/* used by getrusage */
static PyTypeObject RusageResultType;

static PyStructSequence_Field rusage_result_fields[] = {
    {"ru_utime",        "user time used"},
    {"ru_stime",        "system time used"},
    {"ru_maxrss",       "max. resident set size"},
    {"ru_ixrss",        "shared memory size"},
    {"ru_idrss",        "unshared data size"},
    {"ru_isrss",        "unshared stack size"},
    {"ru_minflt",       "page faults not requiring I/O"},
    {"ru_majflt",       "page faults requiring I/O"},
    {"ru_nswap",        "number of swap outs"},
    {"ru_inblock",      "block input operations"},
    {"ru_oublock",      "block output operations"},
    {"ru_msgsnd",       "IPC messages sent"},
    {"ru_msgrcv",       "IPC messages received"},
    {"ru_nsignals",     "signals received"},
    {"ru_nvcsw",        "voluntary context switches"},
    {"ru_nivcsw",       "involuntary context switches"},
    {NULL}
};

static PyStructSequence_Desc rusage_result_desc = {
    "rusage_result",
    NULL,
    rusage_result_fields,
    16
};


#endif

