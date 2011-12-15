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

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Check``
            handle is closed.

        Close the ``Check`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(check_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.


