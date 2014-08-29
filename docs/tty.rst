.. _tty:


.. currentmodule:: pyuv


==========================================
:py:class:`TTY` --- TTY controlling handle
==========================================


.. py:class:: TTY(loop, fd, readable)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`TTY.loop`).
    :param int fd: File descriptor to be opened as a TTY.
    :param bool readable: Specifies if the given fd is readable.

    The ``TTY`` handle provides asynchronous stdin / stdout functionality.

    .. py:method:: shutdown([callback])

        :param callable callback: Callback to be called after shutdown has been performed.

        Shutdown the outgoing (write) direction of the ``TTY`` connection.

        Callback signature: ``callback(tty_handle)``.

    .. py:method:: write(data, [callback])

        :param object data: Data to be written on the ``TTY`` connection. It can be any Python object conforming
            to the buffer interface or a sequence of such objects.

        :param callable callback: Callback to be called after the write operation
            has been performed.

        Write data on the ``TTY`` connection.

        Callback signature: ``callback(tcp_handle, error)``.

    .. py:method:: try_write(data)

        :param object data: Data to be written on the ``TTY`` connection. It can be any Python object conforming
            to the buffer interface.

        Try to write data on the ``TTY`` connection. It will raise an exception if data cannot be written immediately
        or a number indicating the amount of data written.

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

    .. py:method:: fileno

        Return the internal file descriptor (or HANDLE in Windows) used by the
        ``TTY`` handle.

        .. warning::
            libuv expects you not to modify the file descriptor in any way, and
            if you do, things will very likely break.

    .. py:classmethod:: reset_mode

        Reset TTY settings. To be called when program exits.

    .. py:attribute:: write_queue_size

        *Read only*

        Returns the size of the write queue.

    .. py:attribute:: readable

        *Read only*

        Indicates if this handle is readable.

    .. py:attribute:: writable

        *Read only*

        Indicates if this handle is writable.

