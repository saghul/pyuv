.. _util:


.. currentmodule:: pyuv


==============================================
:py:mod:`pyuv.util` --- Miscelaneous utilities
==============================================


.. py:function:: pyuv.util.hrtime

    Get the high-resolution system time. It's given in nanoseconds, even if the
    system doesn't support nanosecond precision.

.. py:function:: pyuv.util.get_free_memory

    Get the system free memory (in bytes).

.. py:function:: pyuv.util.get_total_memory

    Get the system total memory (in bytes).

.. py:function:: pyuv.util.loadavg

    Get the system load average.

.. py:function:: pyuv.util.uptime

    Get the current uptime.

.. py:function:: pyuv.util.resident_set_size

    Get the current resident memory size.

.. py:function:: pyuv.util.interface_addresses

    Get interface addresses information.

.. py:function:: pyuv.util.cpu_info

    Get CPUs information.

.. py:function:: pyuv.util.getrusage

    Get information about resource utilization. getrusage(2) implementation
    which always uses RUSAGE_SELF. Limited support on Windows.

.. py:function:: pyuv.util.guess_handle_type

    Given a file descriptor, returns the handle type in the form of a constant (integer).
    The user can compare it with constants exposed in pyuv.*, such as UV_TTY, UV_TCP, and so on.

.. py:class:: pyuv.util.SignalChecker(loop, fd)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`SignalChecker.loop`).

    :param int fd: File descriptor to be monitored for readability.

    `SignalChecker` is a handle which can be used to interact with signals set up by the standard `signal` module.

    Here is how it works: the user is required to get a pair of file descriptors and put them in nonblocking mode. These descriptors can be either
    a `os.pipe()`, a `socket.socketpair()` or a manually made pair of connected sockets. One of these descriptors will be used just for
    writing, and the other end for reading. The user must use `signal.set_wakeup_fd` and register the write descriptor. The read descriptor
    must be passed to this handle. When the process receives a signal Python will write on the write end of the socket pair and it will cause
    the SignalChecker to wakeup the loop and call the Python C API function to process signals: `PyErr_CheckSignals`.

    It's better to use the `Signal` handle also provided by this library, because it doesn't need any work from the user and it works on any thread. This
    handle merely exists for some cases which you don't probably care about, or if you want to use the `signal` module directly.

    .. py:method:: start

        Start the signal checking handle.

    .. py:method:: stop

        Stop the signal checking handle.

