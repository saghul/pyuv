.. _dns:


.. currentmodule:: pyuv


===============================================================
:py:mod:`puyv.dns` --- Asynchronous getaddrinfo and getnameinfo
===============================================================


.. py:function:: pyuv.util.getaddrinfo(loop, ... , callback=None)

    Equivalent of `socket.getaddrinfo`. When `callback` is not None,
    this function returns a `GAIRequest` object which has a `cancel()`
    method that can be called in order to cancel the request.

    Callback signature: ``callback(result, errorno)``.

    When `callback` is None, this function is synchronous.

.. py:function:: pyuv.util.getnameinfo(loop, ... , callback=None)

    Equivalent of `socket.getnameinfo`. When `callback` is not None,
    this function returns a `GNIRequest` object which has a `cancel()`
    method that can be called in order to cancel the request.

    Callback signature: ``callback(result, errorno)``.

    When `callback` is None, this function is synchronous.

.. note::
    libuv used to bundle c-ares in the past, so the c-ares bindings
    are now also `a separated project <https://github.com/saghul/pycares>`_.
    The examples directory contains `an example <https://github.com/saghul/pycares/blob/master/examples/cares-resolver.py>`_
    on how to build a full DNS resolver using the ``Channel`` class provided by pycares
    together with pyuv.

