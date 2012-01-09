.. _error:


.. currentmodule:: pyuv


==============================================
:py:mod:`pyuv.error` --- Exception definitions
==============================================


This module contains the definition of the different exceptions that
are used throughout `puyv`.


.. py:exception:: UVError()

    Base exception class. Parent of all other exception classes.

.. py:exception:: AsyncError()

    Exception raised if an error is found when calling ``Async`` handle functions.

.. py:exception:: CheckError()

    Exception raised if an error is found when calling ``Check`` handle functions.

.. py:exception:: DNSError()

    Exception raised if an error is found when calling ``DNSResolver`` functions.

.. py:exception:: FSError()

    Exception raised if an error is found when calling funcions from the :py:mod:`fs` module.

.. py:exception:: FSEventError()

    Exception raised if an error is found when calling ``FSEvent`` handle functions.

.. py:exception:: IdleError()

    Exception raised if an error is found when calling ``Idle`` handle functions.

.. py:exception:: PipeError()

    Exception raised if an error is found when calling ``Pipe`` handle functions.

.. py:exception:: PrepareError()

    Exception raised if an error is found when calling ``Prepare`` handle functions.

.. py:exception:: SignalError()

    Exception raised if an error is found when calling ``Signal`` handle functions.

.. py:exception:: TCPError()

    Exception raised if an error is found when calling ``TCP`` handle functions.

.. py:exception:: ThreadPoolError()

    Exception raised if an error is found when running operatios in a ``ThreadPool``.

.. py:exception:: TimerError()

    Exception raised if an error is found when calling ``Timer`` handle functions.

.. py:exception:: UDPError()

    Exception raised if an error is found when calling ``UDP`` handle functions.

.. py:exception:: TTYError()

    Exception raised if an error is found when calling ``TTY`` handle functions.

.. py:exception:: ProcessError()

    Exception raised if an error is found when calling ``Process`` handle functions.


