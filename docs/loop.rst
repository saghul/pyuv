.. _Loop:


.. currentmodule:: pyuv


===============================
:py:class:`Loop` --- Event loop
===============================


.. py:class:: Loop()

    Instantiate a new event loop. The instantiated loop is *not* the default
    event loop. In order to instantiate the default event loop ``default_loop``
    classmethod should be used.

    .. py:classmethod:: default_loop

        Create the *default* event loop. Most applications should use this event
        loop if only a single loop is needed.

    .. py:method:: run([mode])

        :param int mode: Specifies the mode in which the loop will run.
            It can take 3 different values:

            - ``UV_RUN_DEFAULT``: Default mode. Run the event loop until there are no
              active handles or requests.
            - ``UV_RUN_ONCE``: Run a single event loop iteration.
            - ``UV_RUN_NOWAIT``: Run a single event loop iteration, but don't block for io.

        Run the event loop. Returns True if there are pending operations and run should be called again
        or False otherwise.

    .. py:method:: stop

        Stops a running event loop. The action won't happen immediately, it will happen the next loop
        iteration, but if stop was called before blocking for i/o a 0 timeout poll will be performed,
        so the loop will not block for i/o on that iteration.

    .. py:method:: now
    .. py:method:: update_time

        Manage event loop time. ``now`` will return the current event loop time in
        milliseconds. It expresses the time when the event loop began to process
        events.

        After an operation which blocks the event loop for a long time the event loop
        may have lost track of time. In that case ``update_time`` should be called
        to query the kernel for the time.

        This are advanced functions not be used in standard applications.

    .. py:method:: queue_work(work_callback, [done_callback])

        :param callable work_callback: Function that will be called in the thread pool.

        :param callable done_callback: Function that will be called in the caller thread after
            the given function has run in the thread pool.

            Callback signature: ``done_callback(errorno)``. Errorno indicates if the request
            was cancelled (UV_ECANCELLED) or None, if it was actually executed.

        Run the given function in a thread from the internal thread pool. A `WorkRequest` object is
        returned, which has a `cancel()` method that can be called to avoid running the request, in case
        it didn't already run.

        Unix only: The size of the internal threadpool can be controlled with the `UV_THREADPOOL_SIZE`
        environment variable, which needs to be set before the first call to this function. The default
        size is 4 threads.

    .. py:method:: excepthook(type, value, traceback)

        This function prints out a given traceback and exception to sys.stderr.

        When an exception is raised and uncaught, the interpreter calls loop.excepthook with three
        arguments, the exception class, exception instance, and a traceback object. The handling of
        such top-level exceptions can be customized by assigning another three-argument function to
        loop.excepthook.

    .. py:method:: fileno

        Returns the file descriptor of the polling backend.

    .. py:method:: get_timeout

        Returns the poll timeout.

    .. py:attribute:: handles

        *Read only*

        List of handles in this loop.

    .. py:attribute:: alive

        *Read only*

        Checks whether the reference count, that is, the number of active handles or requests left in the event
        loop is non-zero aka if the loop is currently running.

