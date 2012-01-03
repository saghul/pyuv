
import os
import signal
import threading

import common
import pyuv


class SignalTest(common.UVTestCase):

    def setUp(self):
        self.async_cb_called = 0
        self.signal_cb_called = 0
        self.async = None
        self.signal_h = None
        self.loop = pyuv.Loop.default_loop()
        signal.signal(signal.SIGUSR1, self.signal_cb)

    def signal_cb(self, sig, frame):
        self.signal_cb_called += 1
        self.async = pyuv.Async(self.loop)
        self.async.send(self.async_cb)

    def async_cb(self, async):
        self.async_cb_called += 1
        async.close()
        self.signal_h.close()

    def test_signal1(self):
        self.async_cb_called = 0
        def async_cb(async, data):
            self.async_cb_called += 1
            async.close()
        self.signal_h = pyuv.Signal(self.loop)
        self.signal_h.start()
        thread = threading.Thread(target=self.loop.run)
        thread.start()
        os.kill(os.getpid(), signal.SIGUSR1)
        thread.join()
        self.assertEqual(self.async_cb_called, 1)


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

