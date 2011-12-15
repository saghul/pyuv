.. _threadpool:


.. currentmodule:: pyuv


================================================================
:py:class:`ThreadPool` --- Thread pool for running blocking code
================================================================


.. py:class:: ThreadPool()

    Many times there are blocking functions which we can't avoid to call. In order to make
    them *feel* asynchronous they can be run in a different thread. It will appear that they
    don't block, but in reallity they do, but they don't hog the event loop because they run
    on a different thread.

    **NOTE:** ``ThreadPool`` cannot be instantiated, its only method is a classmethod. Any
    attempt of instantiating it will result in :py:exc:`RuntimeError`.

    .. py:classmethod:: run(loop, function, [args, kwargs])

        :param loop: loop where the result will be sent.

        :param callable function: Function that will be called in the thread pool.

        :param tuple args: Arguments to be passed to the function.

        :param dict kwargs: Keyword arguments to be passed to the function.

        Run the given function with the given arguments and keyword arguments in a thread
        from the `ThreadPool`.


