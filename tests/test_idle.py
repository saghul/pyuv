
import common
import unittest

import pyuv


class IdleTest(common.UVTestCase):

    def test_idle1(self):
        self.idle_cb_called = 0
        def idle_cb(idle, data):
            self.idle_cb_called += 1
            idle.stop()
            idle.close()
        loop = pyuv.Loop.default_loop()
        idle = pyuv.Idle(loop)
        idle.start(idle_cb)
        loop.run()
        self.assertEqual(self.idle_cb_called, 1)


if __name__ == '__main__':
    unittest.main()

