
from common import unittest2
import pyuv


class TimerTest(unittest2.TestCase):

    def test_timer1(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        loop.run()
        self.assertEqual(self.timer_cb_called, 1)

    def test_timer_never(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        timer.close()
        loop.run()
        self.assertEqual(self.timer_cb_called, 0)

    def test_timer_ref1(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        loop.unref()
        loop.run()
        # When timer is destroyed it will unref the loop again, so loop.run() would block next time
        loop.ref()
        self.assertEqual(self.timer_cb_called, 0)

    def test_timer_ref2(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        loop.unref()
        loop.run()
        # When timer is destroyed it will unref the loop again, so loop.run() would block next time
        loop.ref()
        self.assertEqual(self.timer_cb_called, 0)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

