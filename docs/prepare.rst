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

