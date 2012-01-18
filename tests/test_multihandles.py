
from common import unittest2
import pyuv


class MultiHandleTest(unittest2.TestCase):

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
        loop = pyuv.Loop.default_loop()
        prepare = pyuv.Prepare(loop)
        prepare.start(prepare_cb)
        idle = pyuv.Idle(loop)
        idle.start(idle_cb)
        check = pyuv.Check(loop)
        check.start(check_cb)
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        loop.run()
        self.assertEqual(self.prepare_cb_called, 1)
        self.assertEqual(self.idle_cb_called, 1)
        self.assertEqual(self.check_cb_called, 1)
        self.assertEqual(self.close_cb_called, 4)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

