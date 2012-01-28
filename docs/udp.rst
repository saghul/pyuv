.. _udp:


.. currentmodule:: pyuv


==============================
:py:class:`UDP` --- UDP handle
==============================


.. py:class:: UDP(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`UDP.loop`).

    The ``UDP`` handle provides asynchronous UDP functionallity both as a client and server.

    .. py:method:: bind((ip, port))

        :param string ip: IP address to bind to.

        :param int port: Port number to bind to.

        Bind to the specified IP address and port. This function needs to be called always,
        both when acting as a client and as a server. It sets the local IP address and port
        from which the data will be sent.

    .. py:method:: close([callback])

        :param callable callback: Callback to be called after the handle has been closed.

        Close the ``UDP`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(udp_handle)``.

    .. py:method:: send(data, [callback])

        :param object data: Data to be sent over the ``UDP`` connection. It can be either
            a string or any iterable containing strings.

        :param callable callback: Callback to be called after the send operation
            has been performed.

        Send data over the ``UDP`` connection.

        Callback signature: ``callback(udp_handle, error)``.

    .. py:method:: start_recv(callback)

        :param callable callback: Callback to be called when data is received on the
            bount IP address and port.

        Start receiving data on the bound IP address and port.

        Callback signature: ``callback(udp_handle, (ip, port), data, error)``.

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

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.


