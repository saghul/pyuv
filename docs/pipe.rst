.. _pipe:


.. currentmodule:: pyuv


======================================
:py:class:`Pipe` --- Named pipe handle
======================================


.. py:class:: Pipe(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Pipe.loop`).

    The ``Pipe`` handle provides asynchronous named pipe functionallity both as a client and server.

    .. py:method:: bind(name)

        :param string name: Name of the pipe to bind to.

        Bind to the specified pipe name. The ``Pipe`` handle is acting as a server
        in this case.

    .. py:method:: listen(callback, [backlog])

        :param callable callback: Callback to be called on every new connection.
            :py:meth:`accept` should be called in that callback in order to accept the
            incoming connection.

        :param int backlog: Indicates the length of the queue of incoming connections. It
            defaults to 128.

        Start listening for new connections.

        Callback signature: ``callback(pipe_handle, error)``.

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

    .. py:method:: close([callback])

        :param callable callback: Callback to be called after the handle has been closed.

        Close the ``Pipe`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(pipe_handle)``.

    .. py:method:: write(data, [callback])

        :param object data: Data to be written on the ``Pipe`` connection. It can be either
            a string or any iterable containing strings.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``Pipe`` connection.

        Callback signature: ``callback(pipe_handle, error)``.

    .. py:method:: write2(data, handle, [callback])

        :param object data: Data to be written on the ``Pipe`` connection. It can be either
            a string or any iterable containing strings.

        :param object handle: Handle to send over the ``Pipe``. Currently only ``TCP`` handles
            are supported.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``Pipe`` connection.

        Callback signature: ``callback(pipe_handle, error)``.

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

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.


