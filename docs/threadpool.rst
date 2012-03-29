.. _threadpool:


.. currentmodule:: pyuv


================================================================
:py:class:`ThreadPool` --- Thread pool for running blocking code
================================================================


.. py:class:: ThreadPool(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`TTY.loop`).

    Many times there are blocking functions which we can't avoid to call. In order to make
    them *feel* asynchronous they can be run in a different thread. It will appear that they
    don't block, but in reallity they do, but they don't hog the event loop because they run
    on a different thread.

    .. py:method:: queue_work(function, [after_function, args, kwargs])

        :param callable function: Function that will be called in the thread pool.

        :param callable after_function: Function that will be called in the caller thread after
            the given function has run in the thread pool. It will get a single argument, the result
            of the function or None in case an exception was produced.

        :param tuple args: Arguments to be passed to the function.

        :param dict kwargs: Keyword arguments to be passed to the function.

        Run the given function with the given arguments and keyword arguments in a thread
        from the `ThreadPool`.


