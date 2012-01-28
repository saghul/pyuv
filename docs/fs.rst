.. _fs:


.. currentmodule:: pyuv


========================================================
:py:mod:`pyuv.fs` --- Asynchronous filesystem operations
========================================================


.. note::
    All functions in the fs module except for the `FSEvent` class support both
    synchronous and asyncronous modes. If you want to run it synchronous don't
    pass any callable as the `callback` argument, else it will run asynchronously.


.. py:function:: pyuv.fs.stat(loop, path, [callback])

    :param string path: File to stat.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    stat syscall.

    Callback signature: ``callback(loop, path, stat_result, errorno)``


.. py:function:: pyuv.fs.lstat(path, loop, [callback])

    Same as :py:func:`pyuv.fs.stat` but it also follows symlinks.


.. py:function:: pyuv.fs.fstat(fd, loop, [callback])

    Same as :py:func:`pyuv.fs.stat` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.unlink(loop, path, [callback])

    :param string path: File to unlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.mkdir(loop, path, [callback])

    :param string path: Directory to create.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Create the specified directory.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.rmdir(loop, path, [callback])

    :param string path: Directory to remove.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified directory.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.rename(loop, path, new_path, [callback])

    :param string path: Original file.

    :param string new_path: New name for the file.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Rename file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.chmod(loop, path, mode, [callback])

    :param string path: File which persmissions will be changed.

    :param int mode: File permissions (ex. 0755)

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified directory.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.fchmod(loop, fd, mode, [callback])

    Same as :py:func:`pyuv.fs.chmod` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.link(loop, path, new_path, [callback])

    :param string path: Original file.

    :param string new_path: Name for the hard-link.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Create a hard-link.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.symlink(loop, path, new_path, [callback])

    :param string path: Original file.

    :param string new_path: Name for the symlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Create a symlink.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.readlink(loop, path, [callback])

    :param string path: Link file to process.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Read link file and return the original file path.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.chown(loop, path, uid, gid, [callback])

    :param string path: File which persmissions will be changed.

    :param int uid: User ID.

    :param int gid: Group ID.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Changes ownership of a file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.fchown(loop, fd, mode, [callback])

    Same as :py:func:`pyuv.fs.chown` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.open(loop, path, flags, mode, [callback])

    :param string path: File to open.

    :param int flags: Flags for opening the file. Check `os.O_` constants.

    :param int mode: Mode for opening the file. Check `stat.S_` constants.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Open file.

    Callback signature: ``callback(loop, path, fd, errorno)``


.. py:function:: pyuv.fs.close(loop, fd, [callback])

    :param int fd: File-descriptor to close.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Close file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.read(loop, fd, length, offset, [callback])

    :param int fd: File-descriptor to read from.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Read from file.

    Callback signature: ``callback(loop, path, read_data, errorno)``


.. py:function:: pyuv.fs.write(loop, fd, write_data, offset, [callback])

    :param int fd: File-descriptor to read from.

    :param string write_data: Data to be written.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Write to file.

    Callback signature: ``callback(loop, path, bytes_written, errorno)``


.. py:function:: pyuv.fs.fsync(loop, fd, [callback])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Sync all changes made to file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.fdatasync(loop, fd, [callback])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Sync data changes made to file.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.ftruncate(loop, fd, offset, [callback])

    :param int fd: File-descriptor to truncate.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Truncate the contents of a file to the specified offset.

    Callback signature: ``callback(loop, path, errorno)``


.. py:function:: pyuv.fs.readdir(loop, path, flags, [callback])

    :param string path: Directory to list.

    :param int flags: Unused.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    List files from a directory.

    Callback signature: ``callback(loop, path, files, errorno)``


.. py:function:: pyuv.fs.sendfile(loop, out_fd, in_fd, in_offset, length, [callback])

    :param int in_fd: File-descriptor to read from.

    :param int in_fd: File-descriptor to write to.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Send a regular file to a stream socket.

    Callback signature: ``callback(loop, path, bytes_written, errorno)``


.. py:function:: pyuv.fs.utime(loop, path, atime, mtime, [callback])

    :param string path: Directory to list.

    :param double atime: New accessed time.

    :param double mtime: New modified time.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Update file times.

    Callback signature: ``callback(loop, path, files, errorno)``


.. py:function:: pyuv.fs.futime(loop, fd, atime, mtime, [callback])

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


