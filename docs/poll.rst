.. _poll:


.. currentmodule:: pyuv


================================
:py:class:`Poll` --- Poll handle
================================


.. py:class:: Poll(loop, fd)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Poll.loop`).

    :param int fd: File descriptor to be monitored for readability or writability.

    ``Poll`` handles can be used to monitor an arbitrary file descriptor for readability or writability.
        On Unix any file descriptor is supported but on Windows only sockets are supported.

        .. note::
            (From the libuv documentation)
            The uv_poll watcher is used to watch file descriptors for readability and
            writability, similar to the purpose of poll(2).

            The purpose of uv_poll is to enable integrating external libraries that
            rely on the event loop to signal it about the socket status changes, like
            c-ares or libssh2. Using uv_poll_t for any other other purpose is not
            recommended; uv_tcp_t, uv_udp_t, etc. provide an implementation that is
            much faster and more scalable than what can be achieved with uv_poll_t,
            especially on Windows.

            It is possible that uv_poll occasionally signals that a file descriptor is
            readable or writable even when it isn't. The user should therefore always
            be prepared to handle EAGAIN or equivalent when it attempts to read from or
            write to the fd.

            The user should not close a file descriptor while it is being polled by an
            active uv_poll watcher. This can cause the poll watcher to report an error,
            but it might also start polling another socket. However the fd can be safely
            closed immediately after a call to uv_poll_stop() or uv_close().

            On windows only sockets can be polled with uv_poll. On unix any file
            descriptor that would be accepted by poll(2) can be used with uv_poll.

            IMPORTANT: It is not okay to have multiple active uv_poll watchers for the same socket.
            This can cause libuv to assert. See this issue: https://github.com/saghul/pyuv/issues/54

    .. py:method:: start(events, callback)

        :param int events: Mask of events that will be detected. The possible events are `pyuv.UV_READABLE`
            or `pyuv.UV_WRITABLE`.
        :param callable callback: Function that will be called when the ``Poll``
            handle receives events.

        Start or update the event mask of the ``Poll`` handle.

        Callback signature: ``callback(poll_handle, events, errorno)``.

    .. py:method:: stop

        Stop the ``Poll`` handle.

    .. py:method:: fileno

        Returns the file descriptor being monitored or -1 if the handle is closed.

