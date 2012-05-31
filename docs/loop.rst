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

    .. py:method:: run_once

        Run a single loop iteration. Returns true if there are any pending events to process,
        false otherwise.

    .. py:method:: now
    .. py:method:: update_time

        Manage event loop time. ``now`` will return the current event loop time in
        nanoseconds. It expresses the time when the event loop began to process
        events.

        After an operation which blocks the event loop for a long time the event loop
        may have lost track of time. In that case ``update_time`` should be called
        to query the kernel for the time.

        This are advanced functions not be used in standard applications.

    .. py:method:: walk(callback)

        :param callable callback: Function that will be called for each handle in the loop.

        Call the given callback for every handle in the loop. Useful for debugging and cleanup
        purposes.

        Callback signature: ``callback(handle)``.

    .. py:attribute:: active_handles

        *Read only*

        Number of currently active handles.

    .. py:attribute:: counters

        *Read only*

        Named tuple containing the loop counters. Loop counters maintain a count on the
        number of different handles, requests, etc that were ever run by the loop.

