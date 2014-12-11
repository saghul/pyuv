.. _fs:


.. currentmodule:: pyuv


========================================================
:py:mod:`pyuv.fs` --- Asynchronous filesystem operations
========================================================

This module provides asynchronous file system operations. All functions return an instance
of `FSRequest`, which has 3 public members:

* path: the path affecting the operation
* error: the error code if the operation failed, 0 if it succeeded
* result: for those operations returning results, it will be stored on this member.

These members will be populated before calling the callback, which has the following
signature: ``callback(loop, req)``

.. note::
    All functions in the fs module except for the `FSEvent` and `FSPoll` classes support both
    synchronous and asynchronous modes. If you want to run it synchronous don't
    pass any callable as the `callback` argument, else it will run asynchronously. If the async
    form is used, then a `FSRequest` is returned when calling the functions, which has a
    `cancel()` method that can be called in order to cancel the request, in case it hasn't run
    yet.

.. note::
    All functions that take a file descriptor argument must get the file descriptor
    resulting of a pyuv.fs.open call on Windows, else the operation will fail. This
    limitation doesn't apply to Unix systems.


.. py:function:: pyuv.fs.stat(loop, path, [callback])

    :param string path: File to stat.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    stat syscall.


.. py:function:: pyuv.fs.lstat(path, loop, [callback])

    Same as :py:func:`pyuv.fs.stat` but it also follows symlinks.


.. py:function:: pyuv.fs.fstat(fd, loop, [callback])

    Same as :py:func:`pyuv.fs.stat` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.unlink(loop, path, [callback])

    :param string path: File to unlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified file.


.. py:function:: pyuv.fs.mkdir(loop, path, [callback])

    :param string path: Directory to create.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Create the specified directory.


.. py:function:: pyuv.fs.rmdir(loop, path, [callback])

    :param string path: Directory to remove.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified directory.


.. py:function:: pyuv.fs.rename(loop, path, new_path, [callback])

    :param string path: Original file.

    :param string new_path: New name for the file.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Rename file.


.. py:function:: pyuv.fs.chmod(loop, path, mode, [callback])

    :param string path: File which permissions will be changed.

    :param int mode: File permissions (ex. 0755)

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Remove the specified directory.


.. py:function:: pyuv.fs.fchmod(loop, fd, mode, [callback])

    Same as :py:func:`pyuv.fs.chmod` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.link(loop, path, new_path, [callback])

    :param string path: Original file.

    :param string new_path: Name for the hard-link.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Create a hard-link.


.. py:function:: pyuv.fs.symlink(loop, path, new_path, flags, [callback])

    :param string path: Original file.

    :param string new_path: Name for the symlink.

    :param loop: loop object where this function runs.

    :param int flags: flags to be used on Windows platform. If ``UV_FS_SYMLINK_DIR`` is specified the symlink
        will be created to a directory. If ``UV_FS_SYMLINK_JUNCTION`` a junction point will be created instead
        of a symlink.

    :param callable callback: Function that will be called with the result of the function.

    Create a symlink.


.. py:function:: pyuv.fs.readlink(loop, path, [callback])

    :param string path: Link file to process.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Read link file and return the original file path.


.. py:function:: pyuv.fs.chown(loop, path, uid, gid, [callback])

    :param string path: File which persmissions will be changed.

    :param int uid: User ID.

    :param int gid: Group ID.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Changes ownership of a file.


.. py:function:: pyuv.fs.fchown(loop, fd, mode, [callback])

    Same as :py:func:`pyuv.fs.chown` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.open(loop, path, flags, mode, [callback])

    :param string path: File to open.

    :param int flags: Flags for opening the file. Check `os.O_` constants.

    :param int mode: Mode for opening the file. Check `stat.S_` constants.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Open file.


.. py:function:: pyuv.fs.close(loop, fd, [callback])

    :param int fd: File-descriptor to close.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Close file.


.. py:function:: pyuv.fs.read(loop, fd, length, offset, [callback])

    :param int fd: File-descriptor to read from.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Read from file.


.. py:function:: pyuv.fs.write(loop, fd, write_data, offset, [callback])

    :param int fd: File-descriptor to read from.

    :param string write_data: Data to be written.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Write to file.


.. py:function:: pyuv.fs.fsync(loop, fd, [callback])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Sync all changes made to file.


.. py:function:: pyuv.fs.fdatasync(loop, fd, [callback])

    :param int fd: File-descriptor to sync.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Sync data changes made to file.


.. py:function:: pyuv.fs.ftruncate(loop, fd, offset, [callback])

    :param int fd: File-descriptor to truncate.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Truncate the contents of a file to the specified offset.


.. py:function:: pyuv.fs.scandir(loop, path, flags, [callback])

    :param string path: Directory to list.

    :param int flags: Unused.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    List files from a directory.


.. py:function:: pyuv.fs.sendfile(loop, out_fd, in_fd, in_offset, length, [callback])

    :param int in_fd: File-descriptor to read from.

    :param int in_fd: File-descriptor to write to.

    :param int length: Amount of bytes to be read.

    :param int offset: File offset.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Send a regular file to a stream socket.


.. py:function:: pyuv.fs.utime(loop, path, atime, mtime, [callback])

    :param string path: Directory to list.

    :param double atime: New accessed time.

    :param double mtime: New modified time.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    Update file times.


.. py:function:: pyuv.fs.futime(loop, fd, atime, mtime, [callback])

    Same as :py:func:`pyuv.fs.utime` but using a file-descriptor instead of the path.


.. py:class:: pyuv.fs.FSEvent(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`FSEvent.loop`).

    `FSEvent` handles monitor a given path for changes.

    .. py:method:: start(path, flags, callback)

        :param string path: Path to monitor for changes.

        :param int flags: Flags which control what events are watched for. Not used at the moment.

        :param callable callback: Function that will be called when the given path changes any of its
            attributes.

        Start the ``FSEvent`` handle.

        Callback signature: ``callback(fsevent_handle, filename, events, error)``.

    .. py:method:: stop

        Stop the ``FSEvent`` handle.

    .. py:attribute:: filename

        *Read only*

        Filename being monitored.


.. py:class:: pyuv.fs.FSPoll(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`FSPoll.loop`).

    ``FSPoll`` handles monitor a given path for changes by using stat syscalls.

    .. py:method:: start(path, interval, callback)

        :param string path: Path to monitor for changes.

        :param float interval: How often to poll for events (in seconds).

        :param callable callback: Function that will be called when the given path changes any of its
            attributes.

        Start the ``FSPoll`` handle.

        Callback signature: ``callback(fspoll_handle, prev_stat, curr_stat, error)``.

    .. py:method:: stop

        Stop the ``FSPoll`` handle.


Module constants

.. py:data:: pyuv.fs.UV_FS_SYMLINK_DIR
.. py:data:: pyuv.fs.UV_FS_SYMLINK_JUNCTION
.. py:data:: pyuv.fs.UV_RENAME
.. py:data:: pyuv.fs.UV_CHANGE
.. py:data:: pyuv.fs.UV_FS_EVENT_WATCH_ENTRY
.. py:data:: pyuv.fs.UV_FS_EVENT_STAT

