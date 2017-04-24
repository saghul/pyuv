.. _idle:


.. currentmodule:: pyuv


================================
:py:class:`Idle` --- Idle handle
================================


.. py:class:: Idle(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Idle.loop`).

    ``Idle`` handles will run the given callback *once per loop iteration*, right
    before the :py:class:`Prepare` handles.

    .. note::
        The notable difference with :py:class:`Prepare` handles is that
        when there are active :py:class:`Idle` handles, the loop will perform
        a zero timeout poll instead of blocking for I/O.

    .. warning::
        Despite the name, :py:class:`Idle` handles will get their callbacks
        called on **every** loop iteration, not when the loop is actually
        "idle".


    .. py:method:: start(callback)

        :param callable callback: Function that will be called when the ``Idle``
            handle is run by the event loop.

        Start the ``Idle`` handle.

        Callback signature: ``callback(idle_handle)``.

    .. py:method:: stop

        Stop the ``Idle`` handle.

