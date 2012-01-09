.. _tcp:


.. currentmodule:: pyuv


==============================
:py:class:`TCP` --- TCP handle
==============================


.. py:class:: TCP(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`TCP.loop`).

    The ``TCP`` handle provides asynchronous TCP functionallity both as a client and server.

    .. py:method:: bind((ip, port))

        :param string ip: IP address to bind to.

        :param int port: Port number to bind to.

        Bind to the specified IP address and port. The ``TCP`` handle is acting as a server
        in this case.

    .. py:method:: listen(callback, [backlog])

        :param callable callback: Callback to be called on every new connection.
            :py:meth:`accept` should be called in that callback in order to accept the
            incoming connection.

        :param int backlog: Indicates the length of the queue of incoming connections. It
            defaults to 128.

        Start listening for new connections.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: accept(client)

        :param object client: Client object where to accept the connection.

        Accept a new incoming connection which was pending. This function needs to be
        called in the callback given to the :py:meth:`listen` function.

    .. py:method:: connect((ip, port), callback)

        :param string ip: IP address to connect to.

        :param int port: Port number to connect to.

        :param callable callback: Callback to be called when the connection to the
            remote endpoint has been made.

        Initiate a client connection to the specified IP address and port.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: getsockname

        Return tuple containing IP address and port of the local socket.

    .. py:method:: getpeername

        Return tuple containing IP address and port of the remote endpoint's socket.

    .. py:method:: shutdown([callback])

        :param callable callback: Callback to be called after shutdown has been performed.

        Shutdown the outgoing (write) direction of the ``TCP`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: close([callback])

        :param callable callback: Callback to be called after the handle has been closed.

        Close the ``TCP`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(tcp_handle)``.

    .. py:method:: write(data, [callback])

        :param object data: Data to be written on the ``TCP`` connection. It can be either
            a string or any iterable containing strings.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``TCP`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

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

        This setting applies to Windows only.
        Enable / disable simultaneous asynchronous accept requests that are queued
        by the operating system when listening for new tcp connections. This setting
        is used to tune a tcp server for the desired performance. Having simultaneous
        accepts can significantly improve the rate of accepting connections (which
        is why it is enabled by default).

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.


