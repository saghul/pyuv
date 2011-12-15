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

    .. py:method:: run
        
        Run the event loop. This method will block until there is no active
        handle running on the loop.

    .. py:method:: poll
        
        Poll the event loop for events without blocking.

    .. py:method:: ref
    .. py:method:: unref

        Manually manage event loop reference count. This doesn't have anything to
        do with Python reference counting. The ``run`` funtion will not return until
        the loop's reference count reaches zero.

        This functions may be used to prevent the loop from being alive only if
        certain handles are active, such as :py:class:`Timer` handles. In that case
        ``unref`` should be called after the Timer was started.

    .. py:method:: now
    .. py:method:: update_time

        Manage event loop time. ``now`` will return the current event loop time in
        nanoseconds. It expresses the time when the event loop began to process
        events.

        After an operation which blocks the event loop for a long time the event loop
        may have lost track of time. In that case ``update_time`` should be called
        to query the kernel for the time.

        This are advanced functions not be used in standard applications.

    .. py:attribute:: data

        Any Python object attached to this loop.

    .. py:attribute:: counters

        *Read only*

        Dictionary containing the loop counters. Loop counters maintain a count on the
        number of different handles, requests, etc that were ever run by the loop.

