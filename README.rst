================================
pyuv: Python interface for libuv
================================

pyuv is a Python module which provides an interface to libuv.
`libuv <http://github.com/joyent/libuv>`_ is a high performance
asynchronous networking library, used as the platform layer for
`NodeJS <http://nodejs.org>`_.

libuv is written and maintained by Joyent Inc. and contributors.
It’s built on top of epoll/kequeue/event ports/etc on Unix and
IOCP on Windows systems providing a consistent API on top of them.

pyuv's features:

- Non-blocking TCP sockets
- Non-blocking named pipes
- UDP support (including multicast)
- Timers
- Child process spawning
- Asynchronous DNS resolution (getaddrinfo)
- Asynchronous file system APIs
- High resolution time
- System memory information
- System CPUs information
- Network interfaces information
- Thread pool scheduling
- ANSI escape code controlled TTY
- File system events (inotify style and stat based)
- IPC and TCP socket sharing between processes
- Arbitrary file descriptor polling
- Thread synchronization primitives


CI status
=========

Stable branch (v0.10):

.. image:: https://secure.travis-ci.org/saghul/pyuv.png?branch=v0.10
    :target: http://travis-ci.org/saghul/pyuv

Master:

.. image:: https://secure.travis-ci.org/saghul/pyuv.png?branch=master
    :target: http://travis-ci.org/saghul/pyuv


Versioning
==========

pyuv follows the versioning scheme used by Node and libuv, that is, odd numbered releases are
considered *unstable* and even numbered releases *stable*. This doesn't necessarily mean that an
odd numbered release is supposed to crash, but the API may change between stable releases.

The so called *unstable* releases will not be released on PyPI, as that would confuse users. PyPI
will always contain the latest *stable* build. All versions (both stable and unstable) are downloadable
from `the GitHub tags page <https://github.com/saghul/pyuv/tags>`_.


Documentation
=============

http://readthedocs.org/docs/pyuv/


Installing
==========

pyuv can be installed via pip as follows:

::

    pip install pyuv

If you'd like to use the `development version <https://github.com/saghul/pyuv/zipball/master#egg=pyuv-dev>`_ use the following command:

::

    pip install pyuv==dev


Building
========

Get the source:

::

    git clone https://github.com/saghul/pyuv


Linux:

::

    ./build_inplace

Mac OSX:

::

    (XCode needs to be installed)
    export ARCHFLAGS="-arch x86_64"
    ./build_inplace

Microsoft Windows (with Visual Studio 2008):

::

    ./build_inplace

Microsoft Windows (with MinGW, not recommended):

::

    (MinGW and MSYS need to be installed)
    ./build_inplace --compiler=mingw32


Running the test suite
======================

There are several ways of running the test ruite:

- Run the test with the current Python interpreter:

  From the toplevel directory, run: ``nosetests -v``

- Use Tox to run the test suite in several virtualenvs with several interpreters

  From the toplevel directory, run: ``tox -e py26,py27,py32`` this will run the test suite
  on Python 2.6, 2.7 and 3.2 (you'll need to have them installed beforehand)


Author
======

Saúl Ibarra Corretgé <saghul@gmail.com>


License
=======

Unless stated otherwise on-file pyuv uses the MIT license, check LICENSE file.


Python versions
===============

Python >= 2.6 is supported. Yes, that includes Python 3 :-)


Contributing
============

If you'd like to contribute, fork the project, make a patch and send a pull
request. Have a look at the surrounding code and please, make yours look
alike :-)

