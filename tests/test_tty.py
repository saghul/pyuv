
import sys

import common
import pyuv
from common import unittest2, platform_skip


@platform_skip(["win32"])
class TYTest(common.UVTestCase):
    __disabled__ = ['win32']

    def test_tty1(self):
        loop = pyuv.Loop.default_loop()
        tty = pyuv.TTY(loop, sys.stdin.fileno())
        w, h = tty.get_winsize()
        self.assertNotEqual((w, h), (None, None))
        self.assertTrue(pyuv.TTY.isatty(sys.stdin.fileno()))

        tty.close()

        loop.run()
        tty.reset_mode()


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

