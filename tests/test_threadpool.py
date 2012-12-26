
import functools
import sys
import threading
import time

from common import unittest2
import pyuv


class WorkItem(object):

    def __init__(self, func, *args, **kw):
        self._cb = functools.partial(func, *args, **kw)
        self.result = None
        self.exc_info = None

    def __call__(self):
        try:
            self._cb()
        except Exception:
            self.exc_info = sys.exc_info()
        finally:
            self._cb = None


class ThreadPoolTest(unittest2.TestCase):

    def setUp(self):
        self.pool_cb_called = 0
        self.pool_after_work_cb_called = 0
        self.loop = pyuv.Loop.default_loop()

    def run_in_pool(self, *args, **kw):
        self.pool_cb_called += 1
        time.sleep(0.1)

    def after_work_cb(self, error):
        self.assertEqual(error, None)
        self.pool_after_work_cb_called += 1

    def test_threadpool1(self):
        self.loop.queue_work(self.run_in_pool, self.after_work_cb)
        args = (1, 2, 3)
        kw = {}
        self.loop.queue_work(functools.partial(self.run_in_pool, *args, **kw), self.after_work_cb)
        args = ()
        kw = {'test': 1}
        self.loop.queue_work(functools.partial(self.run_in_pool, *args, **kw), self.after_work_cb)
        self.loop.run()
        self.assertEqual(self.pool_cb_called, 3)
        self.assertEqual(self.pool_after_work_cb_called, 3)

    def raise_in_pool(self, *args, **kw):
        1/0

    def after_work_cb2(self, error):
        self.assertEqual(error, None)
        self.assertEqual(self.work_item.exc_info[0], ZeroDivisionError)

    def test_threadpool_exc(self):
        self.work_item = WorkItem(self.raise_in_pool)
        self.loop.queue_work(self.work_item, self.after_work_cb2)
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
        loop.queue_work(self.run_in_pool)
        loop.run()

    def test_threadpool_multiple_loops(self):
        t1 = threading.Thread(target=self.run_loop)
        t2 = threading.Thread(target=self.run_loop)
        t3 = threading.Thread(target=self.run_loop)
        [t.start() for t in (t1, t2, t3)]
        [t.join(5) for t in (t1, t2, t3)]
        self.assertEqual(self.pool_cb_called, 3)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

