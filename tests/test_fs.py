
import os
import common
import unittest

import pyuv


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_LINK = 'test_file_1234_link'
TEST_DIR = 'test-dir'
BAD_DIR = 'test-dir-bad'

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
        self.result = result
        self.errorno = errorno

    def test_stat_error(self):
        self.result = None
        self.errorno = None
        pyuv.fs.stat(self.loop, BAD_FILE, self.stat_error_cb)
        self.loop.run()
        self.assertEqual(self.result, -1)
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def stat_cb(self, loop, data, result, errorno, stat_result):
        self.result = result
        self.errorno = errorno

    def test_stat(self):
        self.result = None
        self.errorno = None
        pyuv.fs.stat(self.loop, TEST_FILE, self.stat_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)

    def lstat_cb(self, loop, data, result, errorno, stat_result):
        self.result = result
        self.errorno = errorno

    def test_lstat(self):
        self.result = None
        self.errorno = None
        pyuv.fs.lstat(self.loop, TEST_LINK, self.lstat_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)


class FSTestUnlink(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def bad_unlink_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_bad_unlink(self):
        self.result = None
        self.errorno = None
        pyuv.fs.unlink(self.loop, BAD_FILE, self.bad_unlink_cb)
        self.loop.run()
        self.assertEqual(self.result, -1)
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def unlink_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_unlink(self):
        self.result = None
        self.errorno = None
        pyuv.fs.unlink(self.loop, TEST_FILE, self.unlink_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)


class FSTestMkdir(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(BAD_DIR, 0755)

    def tearDown(self):
        os.rmdir(BAD_DIR)
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass

    def mkdir_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_bad_mkdir(self):
        self.result = None
        self.errorno = None
        pyuv.fs.mkdir(self.loop, BAD_DIR, 0755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.result, -1)
        self.assertEqual(self.errorno, pyuv.errno.UV_EEXIST)

    def test_mkdir(self):
        self.result = None
        self.errorno = None
        pyuv.fs.mkdir(self.loop, TEST_DIR, 0755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)
        self.assertTrue(os.path.isdir(TEST_DIR))


if __name__ == '__main__':
    unittest.main()

