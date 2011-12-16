
import os
import common
import unittest

import pyuv


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_FILE2 = 'test_file_1234_2'
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


class FSTestRmdir(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0755)

    def tearDown(self):
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass

    def rmdir_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_bad_rmdir(self):
        self.result = None
        self.errorno = None
        pyuv.fs.rmdir(self.loop, BAD_DIR, self.rmdir_cb)
        self.loop.run()
        self.assertEqual(self.result, -1)
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_rmdir(self):
        self.result = None
        self.errorno = None
        pyuv.fs.rmdir(self.loop, TEST_DIR, self.rmdir_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)
        self.assertFalse(os.path.isdir(TEST_DIR))


class FSTestRename(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
            os.remove(TEST_FILE2)
        except OSError:
            pass

    def rename_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_rename(self):
        self.result = None
        self.errorno = None
        pyuv.fs.rename(self.loop, TEST_FILE, TEST_FILE2, self.rename_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)
        self.assertFalse(os.path.exists(TEST_FILE))
        self.assertTrue(os.path.exists(TEST_FILE2))


class FSTestChmod(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def chmod_cb(self, loop, data, result, errorno):
        self.result = result
        self.errorno = errorno

    def test_chmod(self):
        self.result = None
        self.errorno = None
        pyuv.fs.chmod(self.loop, TEST_FILE, 0777, self.chmod_cb)
        self.loop.run()
        self.assertEqual(self.result, 0)
        self.assertEqual(self.errorno, 0)
        self.assertTrue(os.access(TEST_FILE, 0777))


if __name__ == '__main__':
    unittest.main()

