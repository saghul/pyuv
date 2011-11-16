.. currentmodule:: pyuv


################################
Welcome to PyUV's documentation!
################################

Python interface for libuv.

libuv is a high performance asynchronous networking library used as the platform layer
for NodeJS. It's built on top of liev and libeio on Unix and IOCP on Windows systems
providing a consistent API on top of them.

Features:
#########

 * Non-blocking TCP sockets
 * Non-blocking named pipes
 * UDP
 * Timers
 * Child process spawning
 * Asynchronous DNS via c-ares or ``uv_getaddrinfo``.
 * Asynchronous file system APIs ``uv_fs_*``
 * High resolution time ``uv_hrtime``
 * Current executable path look up ``uv_exepath``
 * Thread pool scheduling ``uv_queue_work``
 * ANSI escape code controlled TTY ``uv_tty_t``
 * File system events Currently supports inotify, ``ReadDirectoryChangesW`` and kqueue. Event ports in the near future. ``uv_fs_event_t``
 * IPC and socket sharing between processes ``uv_write2``

libuv is written and maintained by Joyent Inc. and contributors.

.. seealso::
    `libuv's source code <http://github.com/joyent/libuv>`_.

.. note::
    puyv does not yet wrap all functionality of libuv.

Contents
########

.. toctree::
    :maxdepth: 3
    :titlesonly:

    pyuv


Examples
########

.. toctree::
    :maxdepth: 1

    examples


ToDo
########

.. toctree::
    :maxdepth: 1

    TODO


Indices and tables
##################

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

