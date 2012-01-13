
import common
import pyuv


class PollTest(common.UVTestCase):

    def test_poll1(self):
        self.cb_called = 0
        def prepare_cb(handle):
            handle.close()
            self.cb_called += 1
        loop = pyuv.Loop.default_loop()
        for i in xrange(500):
            prepare = pyuv.Prepare(loop)
            prepare.start(prepare_cb)
            loop.poll()
        self.assertEqual(self.cb_called, 500)


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

