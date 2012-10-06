.. _signalchecker:


.. currentmodule:: pyuv


========================================================
:py:class:`SignalChecker` --- Helper for signal handling
========================================================


.. py:class:: SignalChecker(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`SignalChecker.loop`).

    The ``SignalChecker`` class is a helper object to allow signals registered with the `signal` module
    to be caught. It's not a real handle, and it is implemented with a :py:class:`Prepare` and a `Check` handle.
    Before the event loop blocks for I/O it calls `PyErr_CheckSignals` so that signals
    registered with the standard library `signal` module are fired.

    This handle is not related with the ``Signal`` handle, it doesn't handle signals, it just instructs
    the Python interpreter to run signal handlers **if and only if** they have been registered with the
    `signal` module.

    .. py:method:: start()

        Start the ``SignalChecker``.

    .. py:method:: stop

        Stop the ``SignalChecker``.

    .. py:method:: close()

        Close the ``SignalChecker`` handles.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where the handles run.

    .. py:attribute:: active

        *Read only*

        Indicates if the handles are active.

    .. py:attribute:: closed

        *Read only*

        Indicates if the handles are closing or already closed.


