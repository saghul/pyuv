.. _util:


.. currentmodule:: pyuv


==============================================
:py:mod:`pyuv.util` --- Miscelaneous utilities
==============================================


.. py:function:: pyuv.util.hrtime

    Get the high-resolution system time. It's given in nanoseconds, even if the
    system doesn't support nanosecond precision.

.. py:function:: pyuv.util.get_free_memory

    Get the system free memory (in bytes).

.. py:function:: pyuv.util.get_total_memory

    Get the system total memory (in bytes).

.. py:function:: pyuv.util.loadavg

    Get the system load average.

.. py:function:: pyuv.util.uptime

    Get the current uptime.

.. py:function:: pyuv.util.resident_set_size

    Get the current resident memory size.

.. py:function:: pyuv.util.interface_addresses

    Get interface addresses information.

.. py:function:: pyuv.util.cpu_info

    Get CPUs information.

.. py:function:: pyuv.util.set_process_title(title)

    :param string title: Desired process title.

    Set current process title.

.. py:function:: pyuv.util.get_process_title()

    Get current process title.


