===============================
pyuv: Python interface to libuv
===============================

pyuv is a Python module which provides an interface to libuv.
libuv (http://github.com/joyent/libuv) is a high performance
asynchronous networking library, used as the platform layer for
NodeJS (http://nodejs.org).

libuv is written and maintained by Joyent Inc. and contributors.
It’s built on top of libev and libeio on Unix and IOCP on Windows systems
providing a consistent API on top of them.

pyuv's features:

- Non-blocking TCP sockets
- Non-blocking named pipes
- UDP support (including multicast)
- Timers
- Child process spawning
- Asynchronous DNS resolver
- Asynchronous file system APIs
- High resolution time
- System memory information
- System CPUs information
- Network interfaces information
- Thread pool scheduling
- ANSI escape code controlled TTY
- File system events (inotify style)
- IPC and TCP socket sharing between processes


Documentation
=============

http://readthedocs.org/docs/pyuv/


Building
========

Linux:

::

    ./build_inplace

Mac OSX:

::

    (XCode needs to be installed)
    export CC="gcc -isysroot /Developer/SDKs/MacOSX10.6.sdk"
    export ARCHFLAGS="-arch x86_64"
    ./build_inplace

Microsoft Windows:

::

    1. for MinGW: (MinGW and MSYS need to be installed)  ./build_inplace
    2. for Visual Studio: python setup.py buld install


Running the test suite
======================

There are several ways of running the test ruite:

- Running individual tests:

 Go inside the tests/ directory and run the test, for example: `python test_tcp.py`

- Run the test with the current Python interpreter:

 From the toplevel directory, run: `nosetests -v -w tests/`

- Use Tox to run the test suite in several virtualenvs with several interpreters

 From the toplevel directory, run: `tox -e py26,py27,py32` this will run the test suite
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

