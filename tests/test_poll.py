
import common
import pyuv


class PollTest(common.UVTestCase):

    def test_poll1(self):
        self.cb_called = 0
        def prepare_cb(handle):
            self.cb_called += 1
        loop = pyuv.Loop.default_loop()
        prepare = pyuv.Prepare(loop)
        prepare.start(prepare_cb)
        for i in xrange(500):
            loop.poll()
        prepare.close()
        self.assertEqual(self.cb_called, 500)


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

