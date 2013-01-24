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

