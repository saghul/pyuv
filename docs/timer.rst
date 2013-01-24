.. _timer:


.. currentmodule:: pyuv


==================================
:py:class:`Timer` --- Timer handle
==================================


.. py:class:: Timer(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Timer.loop`).

    A ``Timer`` handle will run the supplied callback after the specified amount of seconds. 

    .. py:method:: start(callback, timeout, repeat)

        :param callable callback: Function that will be called when the ``Timer``
            handle is run by the event loop.

        :param float timeout: The ``Timer`` will start after the specified amount of time.

        :param float repeat: The ``Timer`` will run again after the specified amount of time.

        Start the ``Timer`` handle.

        Callback signature: ``callback(timer_handle)``.

    .. py:method:: stop

        Stop the ``Timer`` handle.

    .. py:method:: again

        Stop the ``Timer``, and if it is repeating restart it using the repeat value as the timeout.

    .. py:attribute:: repeat

        Get/set the repeat value. Note that if the repeat value is set from a timer callback it does
        not immediately take effect. If the timer was non-repeating before, it will have been stopped.
        If it was repeating, then the old repeat value will have been used to schedule the next timeout.

