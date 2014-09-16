.. _tcp:


.. currentmodule:: pyuv


==============================
:py:class:`TCP` --- TCP handle
==============================


.. py:class:: TCP(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`TCP.loop`).

    The ``TCP`` handle provides asynchronous TCP functionality both as a client and server.

    .. py:method:: bind((ip, port, [flowinfo, [scope_id]]), [flags])

        :param string ip: IP address to bind to.

        :param int port: Port number to bind to.

        :param int flowinfo: Flow info, used only for IPv6. Defaults to 0.

        :param int scope_id: Scope ID, used only for IPv6. Defaults to 0.

        :param int flags: Binding flags. Only pyuv.UV_TCP_IPV6ONLY is supported at the moment, which
            disables dual stack support on IPv6 handles.

        Bind to the specified IP address and port.

    .. py:method:: listen(callback, [backlog])

        :param callable callback: Callback to be called on every new connection.
            :py:meth:`accept` should be called in that callback in order to accept the
            incoming connection.

        :param int backlog: Indicates the length of the queue of incoming connections. It
            defaults to 511.

        Start listening for new connections.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: accept(client)

        :param object client: Client object where to accept the connection.

        Accept a new incoming connection which was pending. This function needs to be
        called in the callback given to the :py:meth:`listen` function.

    .. py:method:: connect((ip, port, [flowinfo, [scope_id]]), callback)

        :param string ip: IP address to connect to.

        :param int port: Port number to connect to.

        :param int flowinfo: Flow info, used only for IPv6. Defaults to 0.

        :param int scope_id: Scope ID, used only for IPv6. Defaults to 0.

        :param callable callback: Callback to be called when the connection to the
            remote endpoint has been made.

        Initiate a client connection to the specified IP address and port.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: open(fd)

        :param int fd: File descriptor to be opened.

        Open the given file descriptor (or SOCKET in Windows) as a ``TCP`` handle.

        ..note::
            The file descriptor will be closed when the `TCP` handle is closed, so if it
            was tasken from a Python socket object, it will be useless afterwards.

        .. note::
            Once a file desctiptor has been passed to the open function, the handle 'owns' it.
            When calling close() on the handle, the file descriptor will be closed. If you'd like
            to keep using it afterwards it's recommended to duplicate it (using os.dup) before
            passing it to this function.

        .. note::
            The fd won't be put in non-blocking mode, the user is responsible for doing it.

    .. py:method:: getsockname

        Return tuple containing IP address and port of the local socket. In case of IPv6 sockets, it also returns
        the flow info and scope ID (a 4 element tuple).

    .. py:method:: getpeername

        Return tuple containing IP address and port of the remote endpoint's socket. In case of IPv6 sockets, it also returns
        the flow info and scope ID (a 4 element tuple).

    .. py:method:: shutdown([callback])

        :param callable callback: Callback to be called after shutdown has been performed.

        Shutdown the outgoing (write) direction of the ``TCP`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: write(data, [callback])

        :param object data: Data to be written on the ``TCP`` connection. It can be any Python object conforming
            to the buffer interface or a sequence of such objects.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``TCP`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: try_write(data)

        :param object data: Data to be written on the ``TCP`` connection. It can be any Python object conforming
            to the buffer interface.

        Try to write data on the ``TCP`` connection. It will raise an exception (with UV_EAGAIN errno) if data cannot
        be written immediately or return a number indicating the amount of data written.

    .. py:method:: start_read(callback)

        :param callable callback: Callback to be called when data is read from the
            remote endpoint.

        Start reading for incoming data from the remote endpoint.

        Callback signature: ``callback(tcp_handle, data, error)``.

    .. py:method:: stop_read

        Stop reading data from the remote endpoint.

    .. py:method:: nodelay(enable)

        :param boolean enable: Enable / disable nodelay option.

        Enable / disable Nagle's algorithm.

    .. py:method:: keepalive(enable, delay)

        :param boolean enable: Enable / disable keepalive option.

        :param int delay: Initial delay, in seconds.

        Enable / disable TCP keep-alive.

    .. py:method:: simultaneous_accepts(enable)

        :param boolean enable: Enable / disable simultaneous accepts.

        Enable / disable simultaneous asynchronous accept requests that are queued
        by the operating system when listening for new tcp connections. This setting
        is used to tune a tcp server for the desired performance. Having simultaneous
        accepts can significantly improve the rate of accepting connections (which
        is why it is enabled by default) but may lead to uneven load distribution in
        multi-process setups.

    .. py:method:: fileno

        Return the internal file descriptor (or SOCKET in Windows) used by the
        ``TCP`` handle.

        .. warning::
            libuv expects you not to modify the file descriptor in any way, and
            if you do, things will very likely break.

    .. py:attribute:: send_buffer_size

        Gets / sets the send buffer size.

    .. py:attribute:: receive_buffer_size

        Gets / sets the receive buffer size.

    .. py:attribute:: write_queue_size

        *Read only*

        Returns the size of the write queue.

    .. py:attribute:: readable

        *Read only*

        Indicates if this handle is readable.

    .. py:attribute:: writable

        *Read only*

        Indicates if this handle is writable.

