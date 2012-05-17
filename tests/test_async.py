
import threading

from common import unittest2
import pyuv


class AsyncTest(unittest2.TestCase):

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
        self.loop = pyuv.Loop.default_loop()
        self.async = pyuv.Async(self.loop, async_cb)
        self.prepare = pyuv.Prepare(self.loop)
        self.prepare.start(prepare_cb)
        self.lock = threading.Lock()
        self.loop.run()
        self.assertEqual(self.async_cb_called, 3)
        self.assertEqual(self.prepare_cb_called, 1)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

