
import unittest

from common import TestCase
import pyuv


class CheckTest(TestCase):

    def test_check1(self):
        self.check_cb_called = 0
        def check_cb(check):
            self.check_cb_called += 1
            check.stop()
            check.close()
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        check = pyuv.Check(self.loop)
        check.start(check_cb)
        timer = pyuv.Timer(self.loop)
        timer.start(timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.check_cb_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
