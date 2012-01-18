
from common import unittest2
import pyuv


class CheckTest(unittest2.TestCase):

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
        loop = pyuv.Loop.default_loop()
        check = pyuv.Check(loop)
        check.start(check_cb)
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        loop.run()
        self.assertEqual(self.check_cb_called, 1)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

