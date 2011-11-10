
import common
import threading
import unittest

import pyuv


class AsyncTest(common.UVTestCase):

    def test_async1(self):
        self.async_cb_called = 0
        def async_cb(async, data):
            self.async_cb_called += 1
            async.close()
        loop = pyuv.Loop.default_loop()
        thread = threading.Thread(target=loop.run)
        async = pyuv.Async(loop)
        thread.start()
        async.send(async_cb)
        thread.join()
        self.assertEqual(self.async_cb_called, 1)


if __name__ == '__main__':
    unittest.main()

