.. _errno:


.. currentmodule:: pyuv


===================================================
:py:mod:`pyuv.errno` --- Error constant definitions
===================================================


This module contains the defined error constants from libuv and c-ares.

.. py:attribute:: pyuv.errno.errorcode

    Mapping (code, string) with libuv error codes.

.. py:attribute:: pyuv.errno.ares_errorcode

    Mapping (code, string) with c-ares error codes.

.. py:function:: pyuv.errno.strerror(errorno)

    :param int errorno: Error number.

    Get the string representation of the given error number.

.. py:function:: pyuv.errno.ares_strerror(errorno)

    :param int errorno: Error number.

    Get the string representation of the given c-ares error number.


