.. _refcount:


*************************
Reference counting scheme
*************************

(This section is about the reference counting scheme used by libuv, it's not related in any
way to the reference counting model used by CPython)

The event loop runs (when `Loop.run` is called) until there are no more active handles. What
does it mean for a handle to be 'active'? Depends on the handle type:

 * Timers: active when ticking
 * Sockets (TCP, UDP, Pipe, TTY): active when reading, writing or listening
 * Process: active until the child process dies
 * Idle, Prepare, Check, Poll, FSEvent, FSPoll: active once they are started
 * Async: always active, until closed

All handles have the `ref` read-write property available in order to modify the default behavior.
These functions operate at the handle level (that is, the *handle* is referenced, not the loop) so if a
handle is ref'd it will maintain the loop alive even if not active.

