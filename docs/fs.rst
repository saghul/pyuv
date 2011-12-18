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

    Callback signature: ``callback(loop, data, result, errorno, stat_result)``


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

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.mkdir(loop, path, callback, [data])

    :param string path: Directory to create.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create the specified directory.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.rmdir(loop, path, callback, [data])

    :param string path: Directory to remove.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Remove the specified directory.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.rename(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: New name for the file.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Rename file.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.chmod(loop, path, mode, callback, [data])

    :param string path: File which persmissions will be changed.

    :param int mode: File permissions (ex. 0755)

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Remove the specified directory.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.fchmod(loop, fd, mode, callback, [data])

    Same as :py:func:`pyuv.fs.chmod` but using a file-descriptor instead of the path.


.. py:function:: pyuv.fs.link(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: Name for the hard-link.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create a hard-link.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.symlink(loop, path, new_path, callback, [data])

    :param string path: Original file.

    :param string new_path: Name for the symlink.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Create a symlink.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.readlink(loop, path, callback, [data])

    :param string path: Link file to process.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Read link file and return the original file path.

    Callback signature: ``callback(loop, data, result, errorno, path)``


.. py:function:: pyuv.fs.chown(loop, path, uid, gid, callback, [data])

    :param string path: File which persmissions will be changed.

    :param int uid: User ID.

    :param int gid: Group ID.

    :param loop: loop object where this function runs.

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    Changes ownership of a file.

    Callback signature: ``callback(loop, data, result, errorno)``


.. py:function:: pyuv.fs.fchown(loop, fd, mode, callback, [data])

    Same as :py:func:`pyuv.fs.chown` but using a file-descriptor instead of the path.


