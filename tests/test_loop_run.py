
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

    def test_stop(self):
        self.num_ticks = 10
        self.prepare_called = 0
        self.timer_called = 0
        def timer_cb(handle):
            self.timer_called += 1
            if self.timer_called == 1:
                self.loop.stop()
            elif self.timer_called == self.num_ticks:
                handle.close()
        def prepare_cb(handle):
            self.prepare_called += 1
            if self.prepare_called == self.num_ticks:
                handle.close()
        self.loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(self.loop)
        timer.start(timer_cb, 0.1, 0.1)
        prepare = pyuv.Prepare(self.loop)
        prepare.start(prepare_cb)
        self.loop.run(pyuv.UV_RUN_DEFAULT)
        self.assertEqual(self.timer_called, 1)
        self.assertTrue(self.prepare_called >= 2)
        self.loop.run(pyuv.UV_RUN_NOWAIT)
        self.assertTrue(self.prepare_called >= 3)
        self.loop.run(pyuv.UV_RUN_DEFAULT)
        self.assertEqual(self.timer_called, 10)
        self.assertEqual(self.prepare_called, 10)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

