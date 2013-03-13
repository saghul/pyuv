
from common import unittest2

import pyuv
import socket


class TestBasetype(unittest2.TestCase):

    def _inheritance_test(self, base, *args, **kwargs):
        derived = type(base.__name__, (base,), {})
        assert derived is not base
        assert issubclass(derived, base)
        instance = derived(*args, **kwargs)
        assert isinstance(instance, base)
        assert isinstance(instance, derived)

    def test_inherit_loop(self):
        self._inheritance_test(pyuv.Loop)

    def test_inherit_timer(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Timer, loop)

    def test_inherit_tcp(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.TCP, loop)

    def test_inherit_udp(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.UDP, loop)

    def test_inherit_poll(self):
        loop = pyuv.Loop.default_loop()
        sock = socket.socket()
        self._inheritance_test(pyuv.Poll, loop, sock.fileno())
        sock.close()

    def test_inherit_pipe(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Pipe, loop)

    def test_inherit_process(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Process, loop)

    def test_inherit_async(self):
        loop = pyuv.Loop.default_loop()
        callback = lambda handle: handle
        self._inheritance_test(pyuv.Async, loop, callback)

    def test_inherit_prepare(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Prepare, loop)

    def test_inherit_idle(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Idle, loop)

    def test_inherit_check(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Check, loop)

    def test_inherit_signal(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.Signal, loop)

    def test_inherit_fs_fspoll(self):
        loop = pyuv.Loop.default_loop()
        self._inheritance_test(pyuv.fs.FSPoll, loop)

    def test_inherit_thread_barrier(self):
        self._inheritance_test(pyuv.thread.Barrier, 1)

    def test_inherit_thread_condition(self):
        self._inheritance_test(pyuv.thread.Condition)

    def test_inherit_thread_mutex(self):
        self._inheritance_test(pyuv.thread.Mutex)

    def test_inherit_thread_rwlock(self):
        self._inheritance_test(pyuv.thread.RWLock)

    def test_inherit_thread_semaphore(self):
        self._inheritance_test(pyuv.thread.Semaphore, 1)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

