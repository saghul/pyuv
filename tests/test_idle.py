
from common import unittest2
import pyuv


class IdleTest(unittest2.TestCase):

    def test_idle1(self):
        self.idle_cb_called = 0
        def idle_cb(idle):
            self.idle_cb_called += 1
            idle.stop()
            idle.close()
        loop = pyuv.Loop.default_loop()
        idle = pyuv.Idle(loop)
        idle.start(idle_cb)
        loop.run()
        self.assertEqual(self.idle_cb_called, 1)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

