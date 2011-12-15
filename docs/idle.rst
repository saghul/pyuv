.. _idle:


.. currentmodule:: pyuv


================================
:py:class:`Idle` --- Idle handle
================================


.. py:class:: Idle(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Idle.loop`).

    ``Idle`` handles run when the event loop is *idle*, that is, there are no
    other events pending to be run. It is usually used to defer operations to
    be run at a later loop iteration.


    .. py:method:: start(callback)

        :param callable callback: Function that will be called when the ``Idle``
            handle is run by the event loop.

        Start the ``Idle`` handle.

        Callback signature: ``callback(idle_handle)``.

    .. py:method:: stop

        Stop the ``Idle`` handle.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Idle``
            handle is closed.

        Close the ``Idle`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(idle_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.


