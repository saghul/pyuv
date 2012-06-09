
import functools
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

    def after_work_cb(self, result, error):
        self.assertEqual(error, None)
        self.pool_after_work_cb_called += 1

    def test_threadpool1(self):
        self.pool.queue_work(self.run_in_pool, self.after_work_cb)
        args = (1, 2, 3)
        kw = {}
        self.pool.queue_work(functools.partial(self.run_in_pool, *args, **kw), self.after_work_cb)
        args = ()
        kw = {'test': 1}
        self.pool.queue_work(functools.partial(self.run_in_pool, *args, **kw), self.after_work_cb)
        self.loop.run()
        self.assertEqual(self.pool_cb_called, 3)
        self.assertEqual(self.pool_after_work_cb_called, 3)

    def raise_in_pool(self, *args, **kw):
        1/0

    def after_work_cb2(self, result, error):
        self.assertEqual(error[0], ZeroDivisionError)

    def test_threadpool2(self):
        self.pool.queue_work(self.raise_in_pool, self.after_work_cb2)
        self.loop.run()


class ThreadPoolMultiLoopTest(unittest2.TestCase):

    def setUp(self):
        self.pool_cb_called = 0
        self.lock = threading.Lock()

    def run_in_pool(self):
        with self.lock:
            self.pool_cb_called += 1
            time.sleep(0.2)

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
        [t.join(5) for t in (t1, t2, t3)]
        self.assertEqual(self.pool_cb_called, 3)


class ThreadPoolThreadCount(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.pool = pyuv.ThreadPool(self.loop)

    def run_in_pool(self):
        time.sleep(0.1)

    def test_threadpool_thread_count(self):
        self.pool.set_parallel_threads(10)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.pool.queue_work(self.run_in_pool)
        self.loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

