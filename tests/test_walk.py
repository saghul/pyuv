
import gc
import weakref

from common import unittest2, TestCase
import pyuv


class WalkTest(TestCase):

    def test_walk(self):
        def timer_cb(timer):
            handles = []
            def walk_cb(handle):
                handles.append(handle)
            self.loop.walk(walk_cb)
            self.assertTrue(timer in handles)
            timer.close()
        timer = pyuv.Timer(self.loop)
        w_timer = weakref.ref(timer)
        self.assertNotEqual(w_timer(), None)
        timer.start(timer_cb, 0.1, 0.0)
        self.loop.run()
        handles = []
        def walk_cb(handle):
            handles.append(handle)
        self.loop.walk(walk_cb)
        self.assertTrue(timer not in handles)
        handles = None
        timer = None
        gc.collect()
        self.assertEqual(w_timer(), None)

    def test_handles(self):
        timer = pyuv.Timer(self.loop)
        self.assertTrue(timer in self.loop.handles)
        timer = None
        self.loop.run()
        self.assertFalse(self.loop.handles)


if __name__ == '__main__':
    unittest2.main(verbosity=2)
