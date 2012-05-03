.. _tty:


.. currentmodule:: pyuv


==========================================
:py:class:`TTY` --- TTY controlling handle
==========================================


.. py:class:: TTY(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`TTY.loop`).

    The ``TTY`` handle provides asynchronous stdin / stdout functionallity.

    .. py:method:: shutdown([callback])

        :param callable callback: Callback to be called after shutdown has been performed.

        Shutdown the outgoing (write) direction of the ``TTY`` connection.

        Callback signature: ``callback(tty_handle)``.

    .. py:method:: close([callback])

        :param callable callback: Callback to be called after the handle has been closed.

        Close the ``TTY`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(tty_handle)``.

    .. py:method:: write(data, [callback])

        :param object data: Data to be written on the ``TTY`` connection. It can be any Python object conforming
            to the buffer interface.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``TTY`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: writelines(seq, [callback])

        :param object seq: Data to be written on the ``TTY`` connection. It can be any iterable object and the same
            logic is applied for the contained elements as in the ``write`` method.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``TTY`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: start_read(callback)

        :param callable callback: Callback to be called when data is read.

        Start reading for incoming data.

        Callback signature: ``callback(status_handle, data)``.

    .. py:method:: stop_read

        Stop reading data.

    .. py:method:: set_mode(mode)

        :param int mode: TTY mode. 0 for normal, 1 for raw.

        Set TTY mode.

    .. py:method:: get_winsize

        Get terminal window size.

    .. py:classmethod:: reset_mode

        Reset TTY settings. To be called when program exits.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: readable

        *Read only*

        Indicates if this handle is readable.

    .. py:attribute:: writable

        *Read only*

        Indicates if this handle is writable.


