
from common import unittest2
import pyuv


class LoopRunTest(unittest2.TestCase):

    def test_run_once(self):
        self.cb_called = 0
        def prepare_cb(handle):
            handle.close()
            self.cb_called += 1
        loop = pyuv.Loop.default_loop()
        for i in range(500):
            prepare = pyuv.Prepare(loop)
            prepare.start(prepare_cb)
            loop.run(pyuv.UV_RUN_ONCE)
        self.assertEqual(self.cb_called, 500)

    def test_run_nowait(self):
        self.cb_called = 0
        def timer_cb(handle):
            handle.close()
            self.cb_called = 1
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 10, 10)
        loop.run(pyuv.UV_RUN_NOWAIT)
        self.assertEqual(self.cb_called, 0)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

