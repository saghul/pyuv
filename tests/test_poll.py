
import common
import unittest

import pyuv


class PollTest(common.UVTestCase):

    def test_poll1(self):
        self.idle_cb_called = 0
        def idle_cb(idle):
            self.idle_cb_called += 1
        loop = pyuv.Loop.default_loop()
        idle = pyuv.Idle(loop)
        idle.start(idle_cb)
        for i in xrange(500):
            loop.poll()
        idle.close()
        self.assertEqual(self.idle_cb_called, 500)


if __name__ == '__main__':
    unittest.main()

