
from common import unittest2
import pyuv


class WalkTest(unittest2.TestCase):

    def test_walk(self):
        def timer_cb(timer):
            handles = []
            def walk_cb(handle):
                handles.append(handle)
            loop.walk(walk_cb)
            self.assertTrue(timer in handles)
            timer.close()
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0.0)
        loop.run()
        handles = []
        def walk_cb(handle):
            handles.append(handle)
        loop.walk(walk_cb)
        self.assertTrue(timer not in handles)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

