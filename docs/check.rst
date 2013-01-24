.. _check:


.. currentmodule:: pyuv


==================================
:py:class:`Check` --- Check handle
==================================


.. py:class:: Check(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Check.loop`).

    ``Check`` handles are usually used together with :py:class:`Prepare` handles.
    They run just after the event loop comes back after being blocked for I/O. The
    callback will be called *once* each loop iteration, after I/O.


    .. py:method:: start(callback)

        :param callable callback: Function that will be called when the ``Check``
            handle is run by the event loop.

        Start the ``Check`` handle.

        Callback signature: ``callback(check_handle)``.

    .. py:method:: stop

        Stop the ``Check`` handle.

