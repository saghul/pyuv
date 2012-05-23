.. _threadpool:


.. currentmodule:: pyuv


================================================================
:py:class:`ThreadPool` --- Thread pool for running blocking code
================================================================


.. py:class:: ThreadPool(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`ThreadPool.loop`).

    Many times there are blocking functions which we can't avoid to call. In order to make
    them *feel* asynchronous they can be run in a different thread. It will appear that they
    don't block, but in reallity they do, but they don't hog the event loop because they run
    on a different thread.

    .. py:method:: queue_work(work_callback, [after_work_callback])

        :param callable work_callback: Function that will be called in the thread pool.

        :param callable after_work_callback: Function that will be called in the caller thread after
            the given function has run in the thread pool.
        
            Callback signature: ``after_work_callback(result, exception)``.
            
        Run the given function in a thread from the `ThreadPool`.

    .. py:classmethod:: set_parallel_threads(numthreads)

        :param int numthreads: number of threads.

        Set the amount of parallel threads that the pool may have.

