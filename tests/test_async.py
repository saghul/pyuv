
import threading

import pyuv
from common import unittest2


class AsyncTest(unittest2.TestCase):

    def test_async1(self):
        self.async_cb_called = 0
        self.close_cb_called = 0
        def close_cb(handle):
            self.close_cb_called += 1
        def async_cb(async):
            self.async_cb_called += 1
            async.close(close_cb)
        loop = pyuv.Loop.default_loop()
        thread = threading.Thread(target=loop.run)
        async = pyuv.Async(loop)
        thread.start()
        async.send(async_cb)
        thread.join()
        self.assertEqual(self.async_cb_called, 1)
        self.assertEqual(self.close_cb_called, 1)


if __name__ == '__main__':
    unittest2.main(verbosity=2)
