
import threading
import time
import unittest

from common import TestCase
import pyuv


class AsyncTest(TestCase):

    def test_async1(self):
        self.async_cb_called = 0
        self.prepare_cb_called = 0
        def async_cb(async):
            with self.lock:
                self.async_cb_called += 1
                n = self.async_cb_called
            if n == 3:
                self.async.close()
                self.prepare.close()
        def prepare_cb(prepare):
            if self.prepare_cb_called:
                return
            self.prepare_cb_called += 1
            self.thread = threading.Thread(target=thread_cb)
            self.thread.start()
        def thread_cb():
            while True:
                with self.lock:
                    n = self.async_cb_called
                    if n == 3:
                        break
                    self.async.send()
        self.async = pyuv.Async(self.loop, async_cb)
        self.prepare = pyuv.Prepare(self.loop)
        self.prepare.start(prepare_cb)
        self.lock = threading.Lock()
        self.loop.run()
        self.assertEqual(self.async_cb_called, 3)
        self.assertEqual(self.prepare_cb_called, 1)

    def test_async2(self):
        self.prepare_cb_called = 0
        self.check_cb_called = 0
        def prepare_cb(prepare):
            self.prepare_cb_called += 1
            self.thread = threading.Thread(target=thread_cb)
            self.thread.start()
        def check_cb(check):
            self.check_cb_called += 1
            self.loop.stop()
        def thread_cb():
            time.sleep(0.01)
            self.async.send()
        self.async = pyuv.Async(self.loop)
        self.prepare = pyuv.Prepare(self.loop)
        self.prepare.start(prepare_cb)
        self.check = pyuv.Check(self.loop)
        self.check.start(check_cb)
        self.loop.run()
        self.assertEqual(self.prepare_cb_called, 1)
        self.assertEqual(self.check_cb_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
