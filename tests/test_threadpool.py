
import common
import random
import time
import unittest

import pyuv


class ThreadPoolTest(common.UVTestCase):

    def setUp(self):
        self.pool_cb_called = 0
        self.loop = pyuv.Loop.default_loop()

    def run_in_pool(self):
        self.pool_cb_called += 1
        time.sleep(random.random())

    def test_threadpool1(self):
        pyuv.ThreadPool.run(self.loop, self.run_in_pool)
        pyuv.ThreadPool.run(self.loop, self.run_in_pool)
        pyuv.ThreadPool.run(self.loop, self.run_in_pool)
        self.loop.run()
        self.assertEqual(self.pool_cb_called, 3)


if __name__ == '__main__':
    unittest.main()

