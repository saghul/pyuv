
import sys
import unittest

from common import platform_skip, TestCase
import pyuv


@platform_skip(["win32"])
class TYTest(TestCase):

    def test_tty1(self):
        tty = pyuv.TTY(self.loop, sys.stdin.fileno(), True)
        w, h = tty.get_winsize()
        self.assertNotEqual((w, h), (None, None))

        tty.close()

        self.loop.run()
        tty.reset_mode()


if __name__ == '__main__':
    unittest.main(verbosity=2)
