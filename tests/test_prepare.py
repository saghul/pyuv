
import common
import pyuv


class PrepareTest(common.UVTestCase):

    def test_prepare1(self):
        self.prepare_cb_called = 0
        def prepare_cb(prepare):
            self.prepare_cb_called += 1
            prepare.stop()
            prepare.close()
        loop = pyuv.Loop.default_loop()
        prepare = pyuv.Prepare(loop)
        prepare.start(prepare_cb)
        loop.run()
        self.assertEqual(self.prepare_cb_called, 1)


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

