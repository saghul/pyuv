
import unittest

from common import TestCase
import pyuv


class IdleTest(TestCase):

    def test_idle1(self):
        self.idle_cb_called = 0
        def idle_cb(idle):
            self.idle_cb_called += 1
            idle.stop()
            idle.close()
        idle = pyuv.Idle(self.loop)
        idle.start(idle_cb)
        self.loop.run()
        self.assertEqual(self.idle_cb_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
