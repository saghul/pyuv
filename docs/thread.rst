.. _thread:


.. currentmodule:: pyuv


===========================================================
:py:mod:`pyuv.thread` --- Thread synchronization primitives
===========================================================


.. py:class:: pyuv.thread.Barrier(count)

    :param int count: Initialize the barrier for the given amount of threads

    .. py:method:: wait

        Synchronize all the participating threads at the barrier.


.. py:class:: pyuv.thread.Mutex

    .. py:method:: lock

        Lock this mutex.

    .. py:method:: unlock

        Unlock this mutex.

    .. py:method:: trylock

        Try to lock the mutex. If the lock could be acquired True is returned, False otherwise.


.. py:class:: pyuv.thread.RWLock

    .. py:method:: rdlock

        Lock this rwlock for reading.

    .. py:method:: rdunlock

        Unlock this rwlock for reading.

    .. py:method:: tryrdlock

        Try to lock the rwlock for reading. If the lock could be acquired True is returned, False otherwise.

    .. py:method:: wrlock

        Lock this rwlock for writing.

    .. py:method:: wrunlock

        Unlock this rwlock for writing.

    .. py:method:: trywrlock

        Try to lock the rwlock for writing. If the lock could be acquired True is returned, False otherwise.


.. py:class:: pyuv.thread.Condition(lock)

    :param Mutex lock: Lock to be used by this condition.

    .. py:method:: signal

        Unblock at least one of the threads that are blocked on this condition.

    .. py:method:: broadcast

        Unblock all threads blocked on this condition.

    .. py:method:: wait

        Block on this condition variable, the mutex lock must be held.

    .. py:method:: timedwait(timeout)

        :param double timeout: Time to wait until condition is met before giving up.

        Wait for the condition to be met, give up after the specified timeout.


.. py:class:: pyuv.thread.Semaphore(count=1)

    :param int count: Initialize the semaphore with the given counter value.

    .. py:method:: post

        Increment (unlock) the semaphore.

    .. py:method:: wait

        Decrement (lock) the semaphore.

    .. py:method:: trywait

        Try to decrement (lock) the semaphore. If the counter could be decremented True is returned, False otherwise.


