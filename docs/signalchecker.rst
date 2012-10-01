.. _signalchecker:


.. currentmodule:: pyuv


========================================================
:py:class:`SignalChecker` --- Helper for signal handling
========================================================


.. py:class:: SignalChecker(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`SignalChecker.loop`).

    The ``SignalChecker`` handle is implemented with a :py:class:`Prepare` handle.
    Before the event loop blocks for I/O it calls `PyErr_CheckSignals` so that signals
    registered with the standard library `signal` module are fired.

    This handle is not related with the ``Signal`` handle, it doesn't handle signals, it just instructs
    the Python interpreter to run signal handlers **if and only if** they have been registered with the
    `signal` module.

    .. py:method:: start()

        Start the ``SignalChecker`` handle.

    .. py:method:: stop

        Stop the ``SignalChecker`` handle.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``SignalChecker``
            handle is closed.

        Close the ``SignalChecker`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(signalchecker_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.

    .. py:attribute:: closed

        *Read only*

        Indicates if this handle is closing or already closed.


