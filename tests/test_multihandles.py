
import unittest

from common import TestCase
import pyuv


class MultiHandleTest(TestCase):

    def test_multihandle1(self):
        self.close_cb_called = 0
        self.prepare_cb_called = 0
        def close_cb(handle):
            self.close_cb_called += 1
        def prepare_cb(prepare):
            self.prepare_cb_called += 1
            prepare.stop()
            prepare.close(close_cb)
        self.idle_cb_called = 0
        def idle_cb(idle):
            self.idle_cb_called += 1
            idle.stop()
            idle.close(close_cb)
        self.check_cb_called = 0
        def check_cb(check):
            self.check_cb_called += 1
            check.stop()
            check.close(close_cb)
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close(close_cb)
        prepare = pyuv.Prepare(self.loop)
        prepare.start(prepare_cb)
        idle = pyuv.Idle(self.loop)
        idle.start(idle_cb)
        check = pyuv.Check(self.loop)
        check.start(check_cb)
        timer = pyuv.Timer(self.loop)
        timer.start(timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.prepare_cb_called, 1)
        self.assertEqual(self.idle_cb_called, 1)
        self.assertEqual(self.check_cb_called, 1)
        self.assertEqual(self.close_cb_called, 4)


if __name__ == '__main__':
    unittest.main(verbosity=2)
