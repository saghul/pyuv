.. _fs:


.. currentmodule:: pyuv


========================================================
:py:mod:`pyuv.fs` --- Asynchronous filesystem operations
========================================================


.. py:function:: pyuv.fs.stat(loop, path, callback, [data])

    :param string path: File to stat.

    :param loop: loop object where this function runs (accessible through :py:attr:`Prepare.loop`).

    :param callable callback: Function that will be called with the result of the function.

    :param object data: Any Python object, it will be passed to the callback function.

    stat syscall.

    Callback signature: ``callback(loop, data, result, errorno, stat_result)``


.. py:function:: pyuv.fs.lstat(path, loop, callback, [data])

    Same as :py:func:`pyuv.fs.stat` but it also follows symlinks.

