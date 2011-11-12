
import common
import unittest

import pyuv


class MultiHandleTest(common.UVTestCase):

    def test_multihandle1(self):
        self.prepare_cb_called = 0
        def prepare_cb(prepare, data):
            self.prepare_cb_called += 1
            prepare.stop()
            prepare.close()
        self.idle_cb_called = 0
        def idle_cb(idle, data):
            self.idle_cb_called += 1
            idle.stop()
            idle.close()
        self.check_cb_called = 0
        def check_cb(check, data):
            self.check_cb_called += 1
            check.stop()
            check.close()
        self.timer_cb_called = 0
        def timer_cb(timer, data):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        loop = pyuv.Loop.default_loop()
        prepare = pyuv.Prepare(loop)
        prepare.start(prepare_cb)
        idle = pyuv.Idle(loop)
        idle.start(idle_cb)
        check = pyuv.Check(loop)
        check.start(check_cb)
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 1, 0)
        loop.run()
        self.assertEqual(self.prepare_cb_called, 1)
        self.assertEqual(self.idle_cb_called, 1)
        self.assertEqual(self.check_cb_called, 1)


if __name__ == '__main__':
    unittest.main()

