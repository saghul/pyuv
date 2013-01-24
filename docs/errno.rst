.. _errno:


.. currentmodule:: pyuv


===================================================
:py:mod:`pyuv.errno` --- Error constant definitions
===================================================


This module contains the defined error constants from libuv and c-ares.

**IMPORTANT:** The errno codes in pyuv don't necessarily match those in the
Python `errno` module.

.. py:attribute:: pyuv.errno.errorcode

    Mapping (code, string) with libuv error codes.

.. py:function:: pyuv.errno.strerror(errorno)

    :param int errorno: Error number.

    Get the string representation of the given error number.

