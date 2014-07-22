
import gc
import unittest
import weakref

from common import TestCase
import pyuv


class HandlesTest(TestCase):

    def test_handles(self):
        timer = pyuv.Timer(self.loop)
        self.assertTrue(timer in self.loop.handles)
        timer = None
        self.loop.run()
        self.assertFalse(self.loop.handles)


if __name__ == '__main__':
    unittest.main(verbosity=2)
