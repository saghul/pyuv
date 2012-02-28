
import threading
import time

from common import unittest2
import pyuv


class ThreadPoolTest(unittest2.TestCase):

    def setUp(self):
        self.pool_cb_called = 0
        self.pool_after_work_cb_called = 0
        self.loop = pyuv.Loop.default_loop()
        self.pool = pyuv.ThreadPool(self.loop)

    def run_in_pool(self, *args, **kw):
        self.pool_cb_called += 1
        time.sleep(0.1)

    def after_work_cb(self, result):
        self.pool_after_work_cb_called += 1

    def test_threadpool1(self):
        self.pool.queue_work(self.run_in_pool, self.after_work_cb)
        self.pool.queue_work(self.run_in_pool, self.after_work_cb, (1, 2, 3), {})
        self.pool.queue_work(self.run_in_pool, self.after_work_cb, (), {'test': 1})
        self.loop.run()
        self.assertEqual(self.pool_cb_called, 3)
        self.assertEqual(self.pool_after_work_cb_called, 3)


class ThreadPoolMultiLoopTest(unittest2.TestCase):

    def setUp(self):
        self.pool_cb_called = 0

    def run_in_pool(self):
        self.pool_cb_called += 1
        time.sleep(0.1)

    def run_loop(self):
        loop = pyuv.Loop()
        pool = pyuv.ThreadPool(loop)
        pool.queue_work(self.run_in_pool)
        loop.run()

    def test_threadpool_multiple_loops(self):
        t1 = threading.Thread(target=self.run_loop)
        t2 = threading.Thread(target=self.run_loop)
        t3 = threading.Thread(target=self.run_loop)
        [t.start() for t in (t1, t2, t3)]
        [t.join() for t in (t1, t2, t3)]
        self.assertEqual(self.pool_cb_called, 3)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

