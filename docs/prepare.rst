.. _prepare:


.. currentmodule:: pyuv


======================================
:py:class:`Prepare` --- Prepare handle
======================================


.. py:class:: Prepare(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Prepare.loop`).

    ``Prepare`` handles are usually used together with :py:class:`Check` handles.
    They run just before the event loop ia about to block for I/O. The callback will be 
    called *once* each loop iteration, before I/O.


    .. py:method:: start(callback)

        :param callable callback: Function that will be called when the ``Prepare``
            handle is run by the event loop.

        Start the ``Prepare`` handle.

        Callback signature: ``callback(prepare_handle)``.

    .. py:method:: stop

        Stop the ``Prepare`` handle.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Prepare``
            handle is closed.

        Close the ``Prepare`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(prepare_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.


