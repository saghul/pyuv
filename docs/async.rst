.. _async:


.. currentmodule:: pyuv


==================================
:py:class:`Async` --- Async handle
==================================


.. py:class:: Async(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Async.loop`).

    Calling event loop related functions from an outside thread is not safe in general.
    This is actually the only handle which is thread safe. The ``Async`` handle may
    be used to pass control from an outside thread to the event loop, as it will allow
    the calling thread to schedulle a callback which will be called in the event loop
    thread.


    .. py:method:: send(callback)

        :param callable callback: Function that will be called after the ``Async``
            handle fires. It will be called in the event loop.

        Start the ``Async`` handle. The callback will be called *at least* once.

        Callback signature: ``callback(async_handle)``

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Async``
            handle is closed.

        Close the ``Async`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(async_handle)``

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.


