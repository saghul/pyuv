.. currentmodule:: pyuv


################################
Welcome to PyUV's documentation!
################################

Python interface for libuv.

libuv is a high performance asynchronous networking library used as the platform layer
for NodeJS. It's built on top of liev and libeio on Unix and IOCP on Windows systems
providing a consistent API on top of them.

libuv is written and maintained by Joyent Inc. and contributors.


.. note::
    pyuv's source code is hosted `on GitHub <http://github.com/saghul/pyuv>`_

Features:
#########

 * Non-blocking TCP sockets
 * Non-blocking named pipes
 * UDP support
 * Timers
 * Child process spawning
 * Asynchronous DNS
 * Asynchronous file system APIs
 * Thread pool scheduling
 * ANSI escape code controlled TTY
 * File system events
 * IPC and TCP socket sharing between processes

.. seealso::
    `libuv's source code <http://github.com/joyent/libuv>`_

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
####

.. toctree::
    :maxdepth: 1

    TODO


Indices and tables
##################

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

