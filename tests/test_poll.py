
from common import unittest2
import pyuv


class PollTest(unittest2.TestCase):

    def test_poll1(self):
        self.cb_called = 0
        def prepare_cb(handle):
            handle.close()
            self.cb_called += 1
        loop = pyuv.Loop.default_loop()
        for i in range(500):
            prepare = pyuv.Prepare(loop)
            prepare.start(prepare_cb)
            loop.run_once()
        self.assertEqual(self.cb_called, 500)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

