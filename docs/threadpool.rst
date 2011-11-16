.. _threadpool:


.. currentmodule:: pyuv


================================================================
:py:class:`ThreadPool` --- Thread pool for running blocking code
================================================================


.. py:class:: ThreadPool(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this pool runs (accessible through :py:attr:`ThreadPool.loop`).

    Many times there are blocking functions which we can't avoid to call. In order to make
    them *feel* asynchronous they can be run in a different thread. It will appear that they
    don't block, but in reallity they do, but they don't hog the event loop because they run
    on a different thread.

    **NOTE:** Due to a limitation in libuv only one ``ThreadPool`` may be instantiated
    for the moment. Any attempt of instantiating another one will result in
    :py:exc:`RuntimeError`.

    .. py:method:: run(function, [args, kwargs])

        :param callable function: Function that will be called in the thread pool.

        :param tuple args: Arguments to be passed to the function.

        :param dict kwargs: Keyword arguments to be passed to the function.

        Run the given function with the given arguments and keyword arguments in a thread
        from the `ThreadPool`.


