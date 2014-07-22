
import unittest

from common import TestCase
import pyuv


class PrepareTest(TestCase):

    def test_prepare1(self):
        self.prepare_cb_called = 0
        def prepare_cb(prepare):
            self.prepare_cb_called += 1
            prepare.stop()
            prepare.close()
        prepare = pyuv.Prepare(self.loop)
        prepare.start(prepare_cb)
        self.loop.run()
        self.assertEqual(self.prepare_cb_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
