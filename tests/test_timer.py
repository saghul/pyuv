
import unittest
import warnings

from common import TestCase
import pyuv


class TimerTest(TestCase):

    def test_timer1(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        timer = pyuv.Timer(self.loop)
        timer.start(timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 1)

    def test_timer_never(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        timer = pyuv.Timer(self.loop)
        timer.start(timer_cb, 0.1, 0)
        timer.close()
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 0)

    def test_timer_ref1(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        self.timer = pyuv.Timer(self.loop)
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 0)

    def test_timer_ref2(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.stop()
            timer.close()
        self.timer = pyuv.Timer(self.loop)
        self.timer.start(timer_cb, 0.1, 0)
        self.timer.ref = False
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 0)
        self.timer.ref = True
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 1)

    def test_timer_noref(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.close()
        t = pyuv.Timer(self.loop)
        t.start(timer_cb, 0.1, 0)
        t = None
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 1)

    def test_timer_submillisecond(self):
        self.timer_cb_called = 0
        def timer_cb(timer):
            self.timer_cb_called += 1
            timer.close()
        t = pyuv.Timer(self.loop)
        with warnings.catch_warnings(record=True) as w:
            # Cause all warnings to always be triggered
            warnings.simplefilter('always')

            t.start(timer_cb, 0.00001, 0)

            self.assertEqual(len(w), 1)
            self.assertTrue(issubclass(w[0].category, RuntimeWarning))
        self.loop.run()
        self.assertEqual(self.timer_cb_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
