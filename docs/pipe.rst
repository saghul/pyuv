.. _pipe:


.. currentmodule:: pyuv


======================================
:py:class:`Pipe` --- Named pipe handle
======================================


.. py:class:: Pipe(loop, ipc)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Pipe.loop`).

    :param boolean ipc: Indicate if this ``Pipe`` will be used for IPC connection.

    The ``Pipe`` handle provides asynchronous named pipe functionality both as a client and server,
    supporting cross-process communication and handle sharing.

    .. py:method:: bind(name)

        :param string name: Name of the pipe to bind to.

        Bind to the specified pipe name. The ``Pipe`` handle is acting as a server
        in this case.

    .. py:method:: listen(callback, [backlog])

        :param callable callback: Callback to be called on every new connection.
            :py:meth:`accept` should be called in that callback in order to accept the
            incoming connection.

        :param int backlog: Indicates the length of the queue of incoming connections. It
            defaults to 511.

        Start listening for new connections.

        Callback signature: ``callback(pipe_handle, error)``.

    .. py:method:: open(fd)

        :param int fd: File descriptor to be opened.

        Open the given file descriptor (or HANDLE in Windows) as a ``Pipe``.

        ..note::
            The file descriptor will be closed when the `UDP` handle is closed, so if it
            was tasken from a Python socket object, it will be useless afterwards.

        .. note::
            Once a file desctiptor has been passed to the open function, the handle 'owns' it.
            When calling close() on the handle, the file descriptor will be closed. If you'd like
            to keep using it afterwards it's recommended to duplicate it (using os.dup) before
            passing it to this function.

        .. note::
            The fd won't be put in non-blocking mode, the user is responsible for doing it.

    .. py:method:: accept(client)

        :param object client: Client object where to accept the connection.

        Accept a new incoming connection which was pending. This function needs to be
        called in the callback given to the :py:meth:`listen` function or in the callback
        given to the :py:meth:`start_read2` is there is any pending handle.

    .. py:method:: connect(name, callback)

        :param string name: Name of the pipe to connect to.

        :param callable callback: Callback to be called when the connection to the
            remote endpoint has been made.

        Initiate a client connection to the specified named pipe.

        Callback signature: ``callback(pipe_handle, error)``.

    .. py:method:: shutdown([callback])

        :param callable callback: Callback to be called after shutdown has been performed.

        Shutdown the outgoing (write) direction of the ``Pipe`` connection.

        Callback signature: ``callback(pipe_handle, error)``.

    .. py:method:: write(data, [callback, [handle]])

        :param object data: Data to be written on the ``Pipe`` connection. It can be any Python object conforming
            to the buffer interface or a sequence of such objects.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        :param object handle: Handle to send over the ``Pipe``. Currently only ``TCP``, ``UDP`` and ``Pipe`` handles
            are supported.

        Write data on the ``Pipe`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: try_write(data)

        :param object data: Data to be written on the ``Pipe`` connection. It can be any Python object conforming
            to the buffer interface.

        Try to write data on the ``Pipe`` connection. It will raise an exception if data cannot be written immediately
        or a number indicating the amount of data written.

    .. py:method:: start_read(callback)

        :param callable callback: Callback to be called when data is read from the
            remote endpoint.

        Start reading for incoming data from the remote endpoint.

        Callback signature: ``callback(pipe_handle, data, error)``.

    .. py:method:: start_read2(callback)

        :param callable callback: Callback to be called when data is read from the
            remote endpoint.

        Start reading for incoming data or a handle from the remote endpoint.

        Callback signature: ``callback(pipe_handle, data, pending, error)``.

    .. py:method:: stop_read

        Stop reading data from the remote endpoint.

    .. py:method:: pending_instances(count)

        :param int count: Number of pending instances.

        This setting applies to Windows only. Set the number of pending pipe instance
        handles when the pipe server is waiting for connections.

    .. py:method:: fileno

        Return the internal file descriptor (or HANDLE in Windows) used by the
        ``Pipe`` handle.

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

