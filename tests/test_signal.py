
import os
import signal
import time
import threading

from common import unittest2, platform_skip
import pyuv


@platform_skip(["win32"])
class SignalTest(unittest2.TestCase):

    def signal_cb(self, handle, signum):
        self.assertEqual(signum, signal.SIGUSR1)
        self.signal_cb_called += 1
        self.async.send()

    def async_cb(self, async):
        self.async_cb_called += 1
        self.async.close()
        self.signal_h.close()

    def test_signal1(self):
        self.async_cb_called = 0
        self.signal_cb_called = 0
        self.loop = pyuv.Loop.default_loop()
        self.async = pyuv.Async(self.loop, self.async_cb)
        self.signal_h = pyuv.Signal(self.loop)
        self.signal_h.start(self.signal_cb, signal.SIGUSR1)
        thread = threading.Thread(target=self.loop.run)
        thread.start()
        os.kill(os.getpid(), signal.SIGUSR1)
        thread.join()
        self.assertEqual(self.async_cb_called, 1)
        self.assertEqual(self.signal_cb_called, 1)


@platform_skip(["win32"])
class MultiLoopSignalTest(unittest2.TestCase):

    def setUp(self):
        self.lock = threading.Lock()
        self.signal_cb_called = 0

    def signal_cb(self, handle, signum):
        self.assertEqual(signum, signal.SIGUSR1)
        with self.lock:
            self.signal_cb_called += 1
        handle.close()

    def run_loop(self):
        loop = pyuv.Loop()
        signal_h = pyuv.Signal(loop)
        signal_h.start(self.signal_cb, signal.SIGUSR1)
        loop.run()

    def test_multi_loop_signals(self):
        threads = [threading.Thread(target=self.run_loop) for x in range(25)]
        [t.start() for t in threads]
        # Wait until threads have started
        time.sleep(1)
        os.kill(os.getpid(), signal.SIGUSR1)
        [t.join() for t in threads]
        self.assertEqual(self.signal_cb_called, 25)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

