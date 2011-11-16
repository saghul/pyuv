.. _signal:


.. currentmodule:: pyuv


=============================================
:py:class:`Signal` --- Signal handling helper
=============================================


.. py:class:: Signal(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Signal.loop`).

    Currently the ``Signal`` handle is implemented with a :py:class:`Prepare` handle.
    Before the event loop blocks for I/O it calls `PyErr_CheckSignals` so that signals
    registered with the standard library `signal` module are fired.

    This implementation detail may change in the future.

    .. py:method:: start()

        Start the ``Signal`` handle.

    .. py:method:: stop

        Stop the ``Signal`` handle.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Signal``
            handle is closed.

        Close the ``Signal`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(signal_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.


