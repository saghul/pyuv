
import os
import common
import unittest

import pyuv


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_LINK = 'test_file_1234_link'

class FSTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')
        os.symlink(TEST_FILE, TEST_LINK)

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def stat_error_cb(self, loop, data, result, errorno, stat_result):
        self.assertEqual(result, -1)
        self.assertNotEqual(errorno, 0)

    def test_stat_error(self):
        pyuv.fs.stat(self.loop, BAD_FILE, self.stat_error_cb)
        self.loop.run()

    def stat_cb(self, loop, data, result, errorno, stat_result):
        self.assertEqual(result, 0)
        self.assertEqual(errorno, 0)

    def test_stat(self):
        pyuv.fs.stat(self.loop, TEST_FILE, self.stat_cb)
        self.loop.run()

    def lstat_cb(self, loop, data, result, errorno, stat_result):
        self.assertEqual(result, 0)
        self.assertEqual(errorno, 0)

    def test_lstat(self):
        pyuv.fs.lstat(self.loop, TEST_LINK, self.lstat_cb)
        self.loop.run()


if __name__ == '__main__':
    unittest.main()

