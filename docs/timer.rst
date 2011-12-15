.. _timer:


.. currentmodule:: pyuv


==================================
:py:class:`Timer` --- Timer handle
==================================


.. py:class:: Timer(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Timer.loop`).

    A ``Timer`` handle will run the suplied callback after the specified amount of seconds. 


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

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Timer``
            handle is closed.

        Close the ``Timer`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(timer_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.

    .. py:attribute:: repeat

        Set the repeat value. Note that if the repeat value is set from a timer callback it does
        not immediately take effect. If the timer was non-repeating before, it will have been stopped.
        If it was repeating, then the old repeat value will have been used to schedule the next timeout.     


