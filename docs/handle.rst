.. _handle:


.. currentmodule:: pyuv


========================================
:py:class:`Handle` --- Handle base class
========================================


.. py:class:: Handle

    `Handle` is an internal base class from which all handles inherit in pyuv. It
    provides all handles with a number of methods which are common for all.

    .. py:method:: ref/unref

        Reference/unreference this handle. If running the event loop in default mode (UV_RUN_DEFAULT)
        the loop will exit when there are no more ref'd active handles left. Calling ref on
        a handle will ensure that the loop is maintained alive while the handle is active. Likewise,
        if all handles are unref'd, the loop would finish enven if they were all active.

        This operations are idempotent, that is, you can call it multiple times with no difference in
        their effect.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the handle is closed.

        Close the handle. After a handle has been closed no other
        operations can be performed on it, they will raise `HandleClosedError`.

        Callback signature: ``callback(handle)``

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.

    .. py:attribute:: closed

        *Read only*

        Indicates if this handle is closing or already closed.

