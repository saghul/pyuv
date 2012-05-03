.. _dns:


.. currentmodule:: pyuv


=============================================================
:py:mod:`puyv.dns` --- Asynchronous DNS resolver using c-ares
=============================================================


.. py:class:: pyuv.dns.DNSResolver(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this resolver runs (accessible through :py:attr:`DNSResolver.loop`).

    The ``DNSResolver`` provides asynchronous DNS operations.

    .. py:method:: gethostbyname(name, callback)

        :param callable callback: Callback to be called with the result of the query.
        
        :param string name: Name to query.

        Retrieves host information corresponding to a host name from a host database.

        Callback signature: ``callback(resolver, result, errorno)``

        .. note::
            ``gethostbyname`` is an obsolete function, new applications should use :py:meth:`getnameinfo`
            or :py:meth:`getaddrinfo` instead.


    .. py:method:: gethostbyaddr(name, callback)

        :param callable callback: Callback to be called with the result of the query.
        
        :param string name: Name to query.

        Retrieves the host information corresponding to a network address.

        Callback signature: ``callback(resolver, result, errorno)``

        .. note::
            ``gethostbyaddr`` is an obsolete function, new applications should use :py:meth:`getnameinfo`
            or :py:meth:`getaddrinfo` instead.


    .. py:method:: getnameinfo(name, port, flags, callback)

        :param callable callback: Callback to be called with the result of the query.
        
        :param string name: Name to query.

        :param int port: Port of the service to query.

        :param int flags: Query flags, see the c-ares flags section.

        Provides protocol-independent name resolution from an address to a host name and
        from a port number to the service name.

        Callback signature: ``callback(resolver, result, errorno)``


    .. py:method:: getaddrinfo(name, callback, [port, family, socktype, protocol, flags])

        :param callable callback: Callback to be called with the result of the query.
        
        :param string name: Name to query.

        :param int port: Port of the service to query.

        :param int socktype: Socket type, see Python ``socket`` module documentation for ``SOCK_*``.

        :param int protocol: Query protocol (UDP, TCP), see Python ``socket`` module documentation for ``IPPROTO_*``.

        :param int flags: Query flags, see Python ``socket`` module documentation for ``AI_*`` flags.

        Provides protocol-independent translation from a host name to an address.

        Callback signature: ``callback(resolver, result, errorno)``


    .. py:method:: query(query_type, name, callback)

        :param int query_type: Type of query to perform.

        :param string name: Name to query.

        :param callable callback: Callback to be called with the result of the query.
        
        Do a DNS query of the specified type. Available types: ``QUERY_TYPE_A``, ``QUERY_TYPE_AAAA``, ``QUERY_TYPE_CNAME``,
        ``QUERY_TYPE_MX``, ``QUERY_TYPE_NAPTR``, ``QUERY_TYPE_NS``, ``QUERY_TYPE_SRV``, ``QUERY_TYPE_TXT``.

        Callback signature: ``callback(resolver, result, errorno)``

    .. py:method:: cancel()

        Cancel any pending query on this resolver. All callbacks will be called with ARES_ECANCELLED errorno.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: servers

        List of nameservers to use for DNS queries.

    .. py:attribute:: data

        Any Python object attached to this handle.


c-ares library constants

.. py:data:: pyuv.dns.ARES_NI_NOFQDN
.. py:data:: pyuv.dns.ARES_NI_NUMERICHOST
.. py:data:: pyuv.dns.ARES_NI_NAMEREQD
.. py:data:: pyuv.dns.ARES_NI_NUMERICSERV
.. py:data:: pyuv.dns.ARES_NI_DGRAM
.. py:data:: pyuv.dns.ARES_NI_TCP
.. py:data:: pyuv.dns.ARES_NI_UDP
.. py:data:: pyuv.dns.ARES_NI_SCTP
.. py:data:: pyuv.dns.ARES_NI_DCCP
.. py:data:: pyuv.dns.ARES_NI_NUMERICSCOPE
.. py:data:: pyuv.dns.ARES_NI_LOOKUPHOST
.. py:data:: pyuv.dns.ARES_NI_LOOKUPSERVICE
.. py:data:: pyuv.dns.ARES_NI_IDN
.. py:data:: pyuv.dns.ARES_NI_IDN_ALLOW_UNASSIGNED
.. py:data:: pyuv.dns.ARES_NI_IDN_USE_STD3_ASCII_RULES

.. seealso::
    `c-ares documentation <http://linux.die.net/man/3/ares_getnameinfo>`_


