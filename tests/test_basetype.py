
import socket
import unittest

from common import TestCase
import pyuv


class TestBasetype(TestCase):

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
        self._inheritance_test(pyuv.Timer, self.loop)

    def test_inherit_tcp(self):
        self._inheritance_test(pyuv.TCP, self.loop)

    def test_inherit_udp(self):
        self._inheritance_test(pyuv.UDP, self.loop)

    def test_inherit_poll(self):
        sock = socket.socket()
        self._inheritance_test(pyuv.Poll, self.loop, sock.fileno())
        sock.close()

    def test_inherit_pipe(self):
        self._inheritance_test(pyuv.Pipe, self.loop)

    def test_inherit_process(self):
        self._inheritance_test(pyuv.Process, self.loop)

    def test_inherit_async(self):
        callback = lambda handle: handle
        self._inheritance_test(pyuv.Async, self.loop, callback)

    def test_inherit_prepare(self):
        self._inheritance_test(pyuv.Prepare, self.loop)

    def test_inherit_idle(self):
        self._inheritance_test(pyuv.Idle, self.loop)

    def test_inherit_check(self):
        self._inheritance_test(pyuv.Check, self.loop)

    def test_inherit_signal(self):
        self._inheritance_test(pyuv.Signal, self.loop)

    def test_inherit_fs_fspoll(self):
        self._inheritance_test(pyuv.fs.FSPoll, self.loop)

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
    unittest.main(verbosity=2)
