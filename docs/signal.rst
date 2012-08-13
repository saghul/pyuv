.. _signal:


.. currentmodule:: pyuv


====================================
:py:class:`Signal` --- Signal handle
====================================


.. py:class:: Signal(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Signal.loop`).

    ``Signal`` handles register for the specified signal and notify the use about the signal's
    occurrence through the specified callback.


    .. py:method:: start(callback, signum)

        :param callable callback: Function that will be called when the specified signal is received.

        :param int signum: Specific signal that this handle listens to.

        Start the ``Signal`` handle.

        Callback signature: ``callback(signal_handle, signal_num)``.

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

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.

    .. py:attribute:: closed

        *Read only*

        Indicates if this handle is closing or already closed.


