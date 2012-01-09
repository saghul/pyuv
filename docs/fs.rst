.. _fs:


.. currentmodule:: pyuv


========================================================
:py:mod:`pyuv.fs` --- Asynchronous filesystem operations
========================================================


.. py:function:: pyuv.fs.stat(loop, path, callback, [data])

    :param string path: File to stat.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    stat syscall.

    Callback signature: ``callback(loop, data, path, stat_result, errorno)``


.. py:function:: pyuv.fs.lstat(path, loop, callback, [data])

    Same as :py:func:`pyuv.fs.stat` but it also follows symlinks.


.. py:function:: pyuv.fs.fstat(fd, loop, callback, [data])

    Same as :py:func:`pyuv.fs.stat` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.unlink(loop, path, callback, [data])

    :param string path: File to unlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Remove the specified file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.mkdir(loop, path, callback, [data])

    :param string path: Directory to create.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create the specified directory.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.rmdir(loop, path, callback, [data])

    :param string path: Directory to remove.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Remove the specified directory.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.rename(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: New name for the file.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Rename file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.chmod(loop, path, mode, callback, [data])

    :param string path: File which persmissions will be changed.

    :param int mode: File permissions (ex. 0755)

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Remove the specified directory.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.fchmod(loop, fd, mode, callback, [data])

    Same as :py:func:`pyuv.fs.chmod` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.link(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: Name for the hard-link.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create a hard-link.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.symlink(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: Name for the symlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create a symlink.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.readlink(loop, path, callback, [data])

    :param string path: Link file to process.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Read link file and return the original file path.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.chown(loop, path, uid, gid, callback, [data])

    :param string path: File which persmissions will be changed.

    :param int uid: User ID.

    :param int gid: Group ID.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Changes ownership of a file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.fchown(loop, fd, mode, callback, [data])

    Same as :py:func:`pyuv.fs.chown` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.open(loop, path, flags, mode, callback, [data])

    :param string path: File to open.

    :param int flags: Flags for opening the file. Check `os.O_` constants.

    :param int mode: Mode for opening the file. Check `stat.S_` constants.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Open file.

    Callback signature: ``callback(loop, data, path, fd, errorno)``


.. py:function:: pyuv.fs.close(loop, fd, callback, [data])

    :param int fd: File-descriptor to close.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Close file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.read(loop, fd, length, offset, callback, [data])

    :param int fd: File-descriptor to read from.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Read from file.

    Callback signature: ``callback(loop, data, path, read_data, errorno)``


.. py:function:: pyuv.fs.write(loop, fd, write_data, offset, callback, [data])

    :param int fd: File-descriptor to read from.

    :param string write_data: Data to be written.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Write to file.

    Callback signature: ``callback(loop, data, path, bytes_written, errorno)``


.. py:function:: pyuv.fs.fsync(loop, fd, callback, [data])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Sync all changes made to file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.fdatasync(loop, fd, callback, [data])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Sync data changes made to file.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.ftruncate(loop, fd, offset, callback, [data])

    :param int fd: File-descriptor to truncate.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Truncate the contents of a file to the specified offset.

    Callback signature: ``callback(loop, data, path, errorno)``


.. py:function:: pyuv.fs.readdir(loop, path, flags, callback, [data])

    :param string path: Directory to list.

    :param int flags: Unused.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    List files from a directory.

    Callback signature: ``callback(loop, data, path, files, errorno)``


.. py:function:: pyuv.fs.sendfile(loop, out_fd, in_fd, in_offset, length, callback, [data])

    :param int in_fd: File-descriptor to read from.

    :param int in_fd: File-descriptor to write to.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Send a regular file to a stream socket.

    Callback signature: ``callback(loop, data, path, bytes_written, errorno)``


.. py:function:: pyuv.fs.utime(loop, path, atime, mtime, callback, [data])

    :param string path: Directory to list.

    :param double atime: New accessed time.

    :param double mtime: New modified time.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Update file times.

    Callback signature: ``callback(loop, data, path, files, errorno)``


.. py:function:: pyuv.fs.futime(loop, fd, atime, mtime, callback, [data])

    Same as :py:func:`pyuv.fs.utime` but using a file-descriptor instead of the path.



.. py:class:: pyuv.fs.FSEvent(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`FSEvent.loop`).

    ``FSEvent`` handles monitor a given path for changes.

    .. py:method:: start(path, callback, flags)

        :param string path: Path to monitor for events.

        :param callable callback: Function that will be called when an event occurs in the
            given path.

        :param int flags: Flags which control what events are watched for. Not used at the moment.

        Start the ``FSEvent`` handle.

        Callback signature: ``callback(fsevent_handle, filename, events, error)``.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``FSEvent``
            handle is closed.

        Close the ``FSEvent`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(fsevent_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.


Module constants

.. py:data:: pyuv.fs.UV_FS_SYMLINK_DIR
.. py:data:: pyuv.fs.UV_RENAME
.. py:data:: pyuv.fs.UV_CHANGE
.. py:data:: pyuv.fs.UV_FS_EVENT_WATCH_ENTRY
.. py:data:: pyuv.fs.UV_FS_EVENT_STAT


