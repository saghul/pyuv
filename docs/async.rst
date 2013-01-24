.. _async:


.. currentmodule:: pyuv


==================================
:py:class:`Async` --- Async handle
==================================


.. py:class:: Async(loop, callback)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Async.loop`).

    :param callable callback: Function that will be called after the ``Async`` handle fires. It will
        be called in the event loop.

    Calling event loop related functions from an outside thread is not safe in general.
    This is actually the only handle which is thread safe. The ``Async`` handle may
    be used to pass control from an outside thread to the event loop, as it will allow
    the calling thread to schedule a callback which will be called in the event loop
    thread.


    .. py:method:: send()

        Start the ``Async`` handle. The callback will be called *at least* once.

        Callback signature: ``callback(async_handle)``

