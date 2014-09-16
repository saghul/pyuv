.. _udp:


.. currentmodule:: pyuv


==============================
:py:class:`UDP` --- UDP handle
==============================


.. py:class:: UDP(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`UDP.loop`).

    The ``UDP`` handle provides asynchronous UDP functionality both as a client and server.

    .. py:method:: bind((ip, port, [flowinfo, [scope_id]]), [flags])

        :param string ip: IP address to bind to.

        :param int port: Port number to bind to.

        :param int flowinfo: Flow info, used only for IPv6. Defaults to 0.

        :param int scope_id: Scope ID, used only for IPv6. Defaults to 0.

        :param int flags: Binding flags. Only pyuv.UV_UDP_IPV6ONLY is supported at the moment, which
            disables dual stack support on IPv6 handles.

        Bind to the specified IP address and port. This function needs to be called always,
        both when acting as a client and as a server. It sets the local IP address and port
        from which the data will be sent.

    .. py:method:: open(fd)

        :param int fd: File descriptor to be opened.

        Open the given file descriptor (or SOCKET in Windows) as a ``UDP`` handle.

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

    .. py:method:: getsockname

        Return tuple containing IP address and port of the local socket. In case of IPv6 sockets, it also returns
        the flow info and scope ID (a 4 element tuple).

    .. py:method:: send((ip, port, [flowinfo, [scope_id]]), data, [callback])

        :param string ip: IP address where data will be sent.

        :param int port: Port number where data will be sent.

        :param int flowinfo: Flow info, used only for IPv6. Defaults to 0.

        :param int scope_id: Scope ID, used only for IPv6. Defaults to 0.

        :param object data: Data to be sent over the ``UDP`` connection. It can be any Python object conforming
            to the buffer interface or a sequence of such objects.

        :param callable callback: Callback to be called after the send operation
            has been performed.

        Send data over the ``UDP`` connection.

        Callback signature: ``callback(udp_handle, error)``.

    .. py:method:: try_send((ip, port), data)

        :param object data: Data to be written on the ``UDP`` connection. It can be any Python object conforming
            to the buffer interface.

        Try to send data on the ``UDP`` connection. It will raise an exception (with UV_EAGAIN errno) if data cannot
        be written immediately or return a number indicating the amount of data written.

    .. py:method:: start_recv(callback)

        :param callable callback: Callback to be called when data is received on the
            bount IP address and port.

        Start receiving data on the bound IP address and port.

        Callback signature: ``callback(udp_handle, (ip, port), flags, data, error)``. The flags attribute can only
        contain pyuv.UV_UDP_PARTIAL, in case the UDP packet was truncated.

    .. py:method:: stop_recv

        Stop receiving data.

    .. py:method:: set_membership(multicast_address, membership, [interface])

        :param string multicast_address: Multicast group to join / leave.

        :param int membership: Flag indicating if the operation is join or
            leave. Flags: ``pyuv.UV_JOIN_GROUP`` and ``pyuv.UV_LEAVE_GROUP``.

        :param string interface: Local interface address to use to join or
            leave the specified multicast group.

        Join or leave a multicast group.

    .. py:method:: set_multicast_ttl(ttl)

        :param int ttl: TTL value to be set.

        Set the multicast Time To Live (TTL).

    .. py:method:: set_multicast_loop(enable)

        :param boolean enable: On /off.

        Set IP multicast loop flag. Makes multicast packets loop back to local sockets.

    .. py:method:: set_broadcast(enable)

        :param boolean enable: On /off.

        Set broadcast on or off.

    .. py:method:: set_ttl(ttl)

        :param int ttl: TTL value to be set.

        Set the Time To Live (TTL).

    .. py:method:: fileno

        Return the internal file descriptor (or SOCKET in Windows) used by the
        ``UDP`` handle.

        .. warning::
            libuv expects you not to modify the file descriptor in any way, and
            if you do, things will very likely break.

    .. py:attribute:: send_buffer_size

        Gets / sets the send buffer size.

    .. py:attribute:: receive_buffer_size

        Gets / sets the receive buffer size.

