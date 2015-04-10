
import os
import shutil
import stat
import unittest

from common import TestCase
import pyuv


# Make stat return integers
os.stat_float_times(False)
pyuv.fs.stat_float_times(False)


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_FILE2 = 'test_file_1234_2'
TEST_LINK = 'test_file_1234_link'
TEST_DIR = 'test-dir'
TEST_DIR2 = 'test-dir_2'
BAD_DIR = 'test-dir-bad'
MAX_INT32_VALUE = 2 ** 31 - 1
OFFSET_VALUE = MAX_INT32_VALUE if not os.name == 'nt' else 2 ** 8 - 1


class FileTestCase(TestCase):

    TEST_FILE_CONTENT = 'test'

    def setUp(self):
        super(FileTestCase, self).setUp()
        with open(TEST_FILE, 'w') as f:
            f.write(self.TEST_FILE_CONTENT)

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        super(FileTestCase, self).tearDown()


class FSTestRequestDict(FileTestCase):

    def stat_cb(self, req):
        self.errorno = req.error
        self.assertEqual(req.test, 'test123')

    def test_request_dict(self):
        self.errorno = None
        req = pyuv.fs.stat(self.loop, TEST_FILE, self.stat_cb)
        req.test = 'test123'
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestStat(FileTestCase):

    def stat_cb(self, req):
        self.errorno = req.error

    def test_stat_error(self):
        self.errorno = None
        pyuv.fs.stat(self.loop, BAD_FILE, self.stat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_stat(self):
        self.errorno = None
        pyuv.fs.stat(self.loop, TEST_FILE, self.stat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def test_stat_sync(self):
        self.stat_data = pyuv.fs.stat(self.loop, TEST_FILE)
        self.assertNotEqual(self.stat_data, None)

    def test_stat_error_sync(self):
        try:
            pyuv.fs.stat(self.loop, BAD_FILE)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestLstat(FileTestCase):

    def setUp(self):
        super(FSTestLstat, self).setUp()
        try:
            pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)
        except pyuv.error.FSError as e:
            if e.args[0] == pyuv.errno.UV_EPERM:
                raise unittest.SkipTest("Symlinks not permitted")

    def tearDown(self):
        try:
            os.remove(TEST_LINK)
        except OSError:
            pass
        super(FSTestLstat, self).tearDown()

    def stat_cb(self, req):
        self.errorno = req.error

    def test_lstat(self):
        self.errorno = None
        pyuv.fs.lstat(self.loop, TEST_LINK, self.stat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestFstat(FileTestCase):

    def fstat_cb(self, req):
        self.errorno = req.error

    def test_fstat(self):
        self.errorno = None
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.fstat(self.loop, fd, self.fstat_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(self.errorno, None)

    def test_fstat_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        self.stat_data = pyuv.fs.fstat(self.loop, fd)
        pyuv.fs.close(self.loop, fd)
        self.assertNotEqual(self.stat_data, None)


class FSTestUnlink(FileTestCase):

    def bad_unlink_cb(self, req):
        self.errorno = req.error

    def test_bad_unlink(self):
        self.errorno = None
        pyuv.fs.unlink(self.loop, BAD_FILE, self.bad_unlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def unlink_cb(self, req):
        self.errorno = req.error

    def test_unlink(self):
        self.errorno = None
        pyuv.fs.unlink(self.loop, TEST_FILE, self.unlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def test_unlink_sync(self):
        pyuv.fs.unlink(self.loop, TEST_FILE)

    def test_unlink_error_sync(self):
        try:
            pyuv.fs.unlink(self.loop, BAD_FILE)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestMkdir(TestCase):

    def setUp(self):
        super(FSTestMkdir, self).setUp()
        os.mkdir(BAD_DIR, 0o755)

    def tearDown(self):
        os.rmdir(BAD_DIR)
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass
        super(FSTestMkdir, self).tearDown()

    def mkdir_cb(self, req):
        self.errorno = req.error

    def test_bad_mkdir(self):
        self.errorno = None
        pyuv.fs.mkdir(self.loop, BAD_DIR, 0o755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_EEXIST)

    def test_mkdir(self):
        self.errorno = None
        pyuv.fs.mkdir(self.loop, TEST_DIR, 0o755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(os.path.isdir(TEST_DIR))

    def test_mkdir_sync(self):
        pyuv.fs.mkdir(self.loop, TEST_DIR, 0o755)
        self.assertTrue(os.path.isdir(TEST_DIR))

    def test_mkdir_error_sync(self):
        try:
            pyuv.fs.mkdir(self.loop, BAD_DIR, 0o755)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_EEXIST)


class FSTestRmdir(TestCase):

    def setUp(self):
        super(FSTestRmdir, self).setUp()
        os.mkdir(TEST_DIR, 0o755)

    def tearDown(self):
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass
        super(FSTestRmdir, self).tearDown()

    def rmdir_cb(self, req):
        self.errorno = req.error

    def test_bad_rmdir(self):
        self.errorno = None
        pyuv.fs.rmdir(self.loop, BAD_DIR, self.rmdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_rmdir(self):
        self.errorno = None
        pyuv.fs.rmdir(self.loop, TEST_DIR, self.rmdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertFalse(os.path.isdir(TEST_DIR))

    def test_rmdir_sync(self):
        pyuv.fs.rmdir(self.loop, TEST_DIR)
        self.assertFalse(os.path.isdir(TEST_DIR))

    def test_rmdir_error_sync(self):
        try:
            pyuv.fs.rmdir(self.loop, BAD_DIR)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestRename(FileTestCase):

    def tearDown(self):
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass
        super(FSTestRename, self).tearDown()

    def rename_cb(self, req):
        self.errorno = req.error

    def test_rename(self):
        self.errorno = None
        pyuv.fs.rename(self.loop, TEST_FILE, TEST_FILE2, self.rename_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertFalse(os.path.exists(TEST_FILE))
        self.assertTrue(os.path.exists(TEST_FILE2))

    def test_rename_sync(self):
        pyuv.fs.rename(self.loop, TEST_FILE, TEST_FILE2)
        self.assertFalse(os.path.exists(TEST_FILE))
        self.assertTrue(os.path.exists(TEST_FILE2))


class FSTestChmod(FileTestCase):

    def chmod_cb(self, req):
        self.errorno = req.error

    def test_chmod(self):
        self.errorno = None
        pyuv.fs.chmod(self.loop, TEST_FILE, 0o777, self.chmod_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))

    def test_chmod_sync(self):
        pyuv.fs.chmod(self.loop, TEST_FILE, 0o777)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))


class FSTestFchmod(FileTestCase):

    def fchmod_cb(self, req):
        self.errorno = req.error

    def test_fchmod(self):
        self.errorno = None
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD | stat.S_IWRITE)
        pyuv.fs.fchmod(self.loop, fd, 0o777, self.fchmod_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(self.errorno, None)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))

    def test_fchmod_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD | stat.S_IWRITE)
        pyuv.fs.fchmod(self.loop, fd, 0o777)
        pyuv.fs.close(self.loop, fd)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))


class FSTestLink(FileTestCase):

    def tearDown(self):
        os.remove(TEST_LINK)
        super(FSTestLink, self).tearDown()

    def link_cb(self, req):
        self.errorno = req.error

    def test_link(self):
        self.errorno = None
        pyuv.fs.link(self.loop, TEST_FILE, TEST_LINK, self.link_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(os.stat(TEST_FILE).st_ino, os.stat(TEST_LINK).st_ino)

    def test_link_sync(self):
        pyuv.fs.link(self.loop, TEST_FILE, TEST_LINK)
        self.assertEqual(os.stat(TEST_FILE).st_ino, os.stat(TEST_LINK).st_ino)


class FSTestSymlink(FileTestCase):

    def tearDown(self):
        try:
            os.remove(TEST_LINK)
        except OSError:
            pass
        super(FSTestSymlink, self).tearDown()

    def symlink_cb(self, req):
        self.errorno = req.error

    def test_symlink(self):
        self.errorno = None
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0, self.symlink_cb)
        self.loop.run()
        if self.errorno == pyuv.errno.UV_EPERM:
            raise unittest.SkipTest("Symlinks not permitted")
        self.assertEqual(self.errorno, None)
        self.assertTrue(os.stat(TEST_LINK).st_mode & stat.S_IFLNK)

    def test_symlink_sync(self):
        try:
            pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)
        except pyuv.error.FSError as e:
            if e.args[0] == pyuv.errno.UV_EPERM:
                raise unittest.SkipTest("Symlinks not permitted")
        self.assertTrue(os.stat(TEST_LINK).st_mode & stat.S_IFLNK)


class FSTestReadlink(FileTestCase):

    def setUp(self):
        super(FSTestReadlink, self).setUp()
        try:
            pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)
        except pyuv.error.FSError as e:
            if e.args[0] == pyuv.errno.UV_EPERM:
                raise unittest.SkipTest("Symlinks not permitted")

    def tearDown(self):
        try:
            os.remove(TEST_LINK)
        except OSError:
            pass
        super(FSTestReadlink, self).tearDown()

    def readlink_cb(self, req):
        self.errorno = req.error
        self.link_path = req.result

    def test_readlink(self):
        self.errorno = None
        self.link_path = None
        pyuv.fs.readlink(self.loop, TEST_LINK, self.readlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(self.link_path, TEST_FILE)

    def test_readlink_sync(self):
        self.link_path = pyuv.fs.readlink(self.loop, TEST_LINK)
        self.assertEqual(self.link_path, TEST_FILE)


class FSTestChown(FileTestCase):

    def chown_cb(self, req):
        self.errorno = req.error

    def test_chown(self):
        self.errorno = None
        pyuv.fs.chown(self.loop, TEST_FILE, -1, -1, self.chown_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def test_chown_sync(self):
        pyuv.fs.chown(self.loop, TEST_FILE, -1, -1)


class FSTestFchown(FileTestCase):

    def fchown_cb(self, req):
        self.errorno = req.error

    def test_fchown(self):
        self.errorno = None
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD | stat.S_IWRITE)
        pyuv.fs.fchown(self.loop, fd, -1, -1, self.fchown_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(self.errorno, None)

    def test_fchown_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD | stat.S_IWRITE)
        pyuv.fs.fchown(self.loop, fd, -1, -1)
        pyuv.fs.close(self.loop, fd)


class FSTestOpen(TestCase):

    def setUp(self):
        super(FSTestOpen, self).setUp()
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        super(FSTestOpen, self).tearDown()

    def close_cb(self, req):
        self.errorno = req.error

    def open_cb(self, req):
        fd = req.result
        self.assertNotEqual(fd, None)
        self.assertEqual(req.error, None)
        pyuv.fs.close(self.loop, fd, self.close_cb)

    def test_open_create(self):
        self.errorno = None
        pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE, self.open_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def open_noent_cb(self, req):
        self.fd = req.result
        self.errorno = req.error

    def test_open_noent(self):
        self.fd = None
        self.errorno = None
        pyuv.fs.open(self.loop, BAD_FILE, os.O_RDONLY, 0, self.open_noent_cb)
        self.loop.run()
        self.assertEqual(self.fd, None)
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_open_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT|os.O_TRUNC, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.close(self.loop, fd)

    def test_open_error_sync(self):
        try:
            pyuv.fs.open(self.loop, BAD_FILE, os.O_RDONLY, 0)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestRead(FileTestCase):

    TEST_FILE_CONTENT = 'test1234567890'

    def read_cb(self, req):
        self.errorno = req.error
        self.data = req.result
        pyuv.fs.close(self.loop, self.fd)

    def test_read(self):
        self.data = None
        self.errorno = None
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDONLY, stat.S_IREAD)
        pyuv.fs.read(self.loop, self.fd, 4, -1, self.read_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(self.data, b'test')

    def test_read_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDONLY, stat.S_IREAD)
        self.data = pyuv.fs.read(self.loop, fd, 4, -1)
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(self.data, b'test')

    def test_read_offset(self):
        with open(TEST_FILE, 'w') as f:
            f.seek(OFFSET_VALUE)
            f.write('test1234567890')
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDONLY, stat.S_IREAD)
        data = pyuv.fs.read(self.loop, fd, 4, OFFSET_VALUE + 4)
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(data, b'1234')


class FSTestWrite(TestCase):

    def setUp(self):
        super(FSTestWrite, self).setUp()
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT|os.O_TRUNC, stat.S_IWRITE|stat.S_IREAD)

    def tearDown(self):
        os.remove(TEST_FILE)
        super(FSTestWrite, self).tearDown()

    def write_cb(self, req):
        pyuv.fs.close(self.loop, self.fd)
        self.bytes_written = req.result
        self.errorno = req.error

    def test_write(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.fd, b"TEST", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 4)
        self.assertEqual(self.errorno, None)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "TEST")

    def test_write_null(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.fd, b"TES\x00T", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 5)
        self.assertEqual(self.errorno, None)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "TES\x00T")

    def test_write_sync(self):
        self.bytes_written = pyuv.fs.write(self.loop, self.fd, b"TEST", -1)
        pyuv.fs.close(self.loop, self.fd)
        self.assertEqual(self.bytes_written, 4)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "TEST")

    def test_write_offset(self):
        offset = OFFSET_VALUE + 4
        self.bytes_written = pyuv.fs.write(self.loop, self.fd, b"TEST", offset)
        pyuv.fs.close(self.loop, self.fd)
        with open(TEST_FILE, 'r') as fobj:
            fobj.seek(offset)
            self.assertEqual(fobj.read(), "TEST")


class FSTestFsync(TestCase):

    def write_cb(self, req):
        self.assertEqual(req.result, 4)
        self.assertEqual(req.error, None)
        pyuv.fs.fdatasync(self.loop, self.fd, self.fdatasync_cb)

    def fdatasync_cb(self, req):
        self.assertEqual(req.error, None)
        pyuv.fs.fsync(self.loop, self.fd, self.fsync_cb)

    def fsync_cb(self, req):
        self.assertEqual(req.error, None)

    def test_fsync(self):
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR|os.O_CREAT|os.O_TRUNC, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.write(self.loop, self.fd, b"TEST", -1, self.write_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, self.fd)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "TEST")

    def test_fsync_sync(self):
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR|os.O_CREAT|os.O_TRUNC, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.write(self.loop, self.fd, b"TEST", -1)
        pyuv.fs.fdatasync(self.loop, self.fd)
        pyuv.fs.fsync(self.loop, self.fd)
        pyuv.fs.close(self.loop, self.fd)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "TEST")


class FSTestFtruncate(FileTestCase):

    TEST_FILE_CONTENT = "test-data"

    def ftruncate_cb(self, req):
        self.errorno = req.error
        pyuv.fs.close(self.loop, self.fd)

    def test_ftruncate1(self):
        self.errorno = None
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, self.fd, 4, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "test")

    def test_ftruncate2(self):
        self.errorno = None
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, self.fd, 0, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "")

    def test_ftruncate_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, fd, 0)
        pyuv.fs.close(self.loop, fd)
        with open(TEST_FILE, 'r') as fobj:
            self.assertEqual(fobj.read(), "")


class FSTestScandir(TestCase):

    def setUp(self):
        super(FSTestScandir, self).setUp()
        os.mkdir(TEST_DIR, 0o755)
        os.mkdir(os.path.join(TEST_DIR, TEST_DIR2), 0o755)
        with open(os.path.join(TEST_DIR, TEST_FILE), 'w') as f:
            f.write('test')
        with open(os.path.join(TEST_DIR, TEST_FILE2), 'w') as f:
            f.write('test')

    def tearDown(self):
        shutil.rmtree(TEST_DIR)
        super(FSTestScandir, self).tearDown()

    def scandir_cb(self, req):
        self.errorno = req.error
        self.files = req.result

    def test_bad_scandir(self):
        self.errorno = None
        self.files = None
        pyuv.fs.scandir(self.loop, BAD_DIR, 0, self.scandir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_scandir(self):
        self.errorno = None
        self.files = None
        pyuv.fs.scandir(self.loop, TEST_DIR, 0, self.scandir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(TEST_FILE in self.files)
        self.assertTrue(TEST_FILE2 in self.files)
        self.assertTrue(TEST_DIR2 in self.files)

    def test_scandir_sync(self):
        self.files = pyuv.fs.scandir(self.loop, TEST_DIR, 0)
        self.assertNotEqual(self.files, None)
        self.assertTrue(TEST_FILE in self.files)
        self.assertTrue(TEST_FILE2 in self.files)
        self.assertTrue(TEST_DIR2 in self.files)

    def test_scandir_error_sync(self):
        try:
            pyuv.fs.scandir(self.loop, BAD_DIR, 0)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestSendfile(TestCase):

    def setUp(self):
        super(FSTestSendfile, self).setUp()
        with open(TEST_FILE, 'w') as f:
            f.write("begin\n")
            os.lseek(f.fileno(), 65536, os.SEEK_CUR)
            f.write("end\n")
            f.flush()

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_FILE2)
        super(FSTestSendfile, self).tearDown()

    def sendfile_cb(self, req):
        self.bytes_written = req.result
        self.errorno = req.error

    def test_sendfile(self):
        self.result = None
        self.errorno = None
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        fd2 = pyuv.fs.open(self.loop, TEST_FILE2, os.O_RDWR|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.sendfile(self.loop, fd2, fd, 0, 131072, self.sendfile_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, fd)
        pyuv.fs.close(self.loop, fd2)
        self.assertEqual(self.errorno, None)
        with open(TEST_FILE, 'r') as fobj1:
            with open(TEST_FILE2, 'r') as fobj2:
                self.assertEqual(fobj1.read(), fobj2.read())

    def test_sendfile_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        fd2 = pyuv.fs.open(self.loop, TEST_FILE2, os.O_RDWR|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE)
        self.bytes_written = pyuv.fs.sendfile(self.loop, fd2, fd, 0, 131072)
        pyuv.fs.close(self.loop, fd)
        pyuv.fs.close(self.loop, fd2)
        with open(TEST_FILE, 'r') as fobj1:
            with open(TEST_FILE2, 'r') as fobj2:
                self.assertEqual(fobj1.read(), fobj2.read())

    def test_sendfile_offset(self):
        offset = OFFSET_VALUE + 1
        with open(TEST_FILE, 'w') as f:
            f.seek(offset)
            f.write("test")
            f.flush()
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        fd2 = pyuv.fs.open(self.loop, TEST_FILE2, os.O_RDWR|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE)
        self.bytes_written = pyuv.fs.sendfile(self.loop, fd2, fd, offset, 4)
        pyuv.fs.close(self.loop, fd)
        pyuv.fs.close(self.loop, fd2)
        with open(TEST_FILE, 'r') as fobj1:
            fobj1.seek(offset)
            with open(TEST_FILE2, 'r') as fobj2:
                self.assertEqual(fobj1.read(), fobj2.read())


class FSTestUtime(FileTestCase):

    def setUp(self):
        super(FSTestUtime, self).setUp()
        self.fd = None

    def utime_cb(self, req):
        self.errorno = req.error
        if self.fd is not None:
            pyuv.fs.close(self.loop, self.fd)

    def test_utime(self):
        self.errorno = None
        atime = mtime = 400497753
        pyuv.fs.utime(self.loop, TEST_FILE, atime, mtime, self.utime_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        s = os.stat(TEST_FILE)
        self.assertEqual(s.st_atime, atime)
        self.assertEqual(s.st_mtime, mtime)

    def test_futime(self):
        self.errorno = None
        atime = mtime = 400497753
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IWRITE|stat.S_IREAD)
        pyuv.fs.futime(self.loop, self.fd, atime, mtime, self.utime_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        s = os.stat(TEST_FILE)
        self.assertTrue(s.st_atime == atime and s.st_mtime == mtime)

    def test_utime_sync(self):
        atime = mtime = 400497753
        pyuv.fs.utime(self.loop, TEST_FILE, atime, mtime)
        s = os.stat(TEST_FILE)
        self.assertEqual(s.st_atime, atime)
        self.assertEqual(s.st_mtime, mtime)

    def test_futime_sync(self):
        atime = mtime = 400497753
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IWRITE|stat.S_IREAD)
        pyuv.fs.futime(self.loop, self.fd, atime, mtime)
        pyuv.fs.close(self.loop, self.fd)
        s = os.stat(TEST_FILE)
        self.assertEqual(s.st_atime, atime)
        self.assertEqual(s.st_mtime, mtime)


class FSTestAccess(TestCase):

    def setUp(self):
        super(FSTestAccess, self).setUp()
        with open(TEST_FILE, 'w') as f:
            f.write("test")

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        super(FSTestAccess, self).tearDown()

    def access_cb(self, req):
        self.errorno = req.error

    def test_bad_access(self):
        self.errorno = None
        pyuv.fs.access(self.loop, BAD_FILE, os.F_OK, self.access_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_access(self):
        self.errorno = None
        pyuv.fs.access(self.loop, TEST_FILE, os.F_OK, self.access_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def test_access_sync(self):
        pyuv.fs.access(self.loop, TEST_FILE, os.F_OK)

    def test_access_error_sync(self):
        try:
            pyuv.fs.access(self.loop, BAD_FILE, os.F_OK)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSEventTestBasic(FileTestCase):

    def tearDown(self):
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass
        super(FSEventTestBasic, self).tearDown()

    def on_fsevent_cb(self, handle, filename, events, errorno):
        handle.stop()
        handle.close()
        self.filename = filename
        self.events = events
        self.errorno = errorno

    def timer_cb(self, timer):
        timer.close()
        os.rename(TEST_FILE, TEST_FILE2)

    def test_fsevent_basic(self):
        self.errorno = None
        self.events = None
        self.filename = None
        fs_event = pyuv.fs.FSEvent(self.loop)
        fs_event.start(TEST_FILE, 0, self.on_fsevent_cb)
        self.assertEqual(fs_event.path, TEST_FILE)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename in (None, TEST_FILE, TEST_FILE2))
        self.assertTrue(self.events in (pyuv.fs.UV_CHANGE, pyuv.fs.UV_RENAME))


class FSEventTest(FileTestCase):

    def setUp(self):
        super(FSEventTest, self).setUp()
        os.mkdir(TEST_DIR, 0o755)
        with open(os.path.join(TEST_DIR, TEST_FILE), 'w') as f:
            f.write("test")

    def tearDown(self):
        shutil.rmtree(TEST_DIR)
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass
        super(FSEventTest, self).tearDown()

    def on_fsevent_cb(self, handle, filename, events, errorno):
        handle.stop()
        handle.close()
        self.filename = filename
        self.events = events
        self.errorno = errorno

    def timer_cb2(self, timer):
        timer.close()
        os.rename(os.path.join(TEST_DIR, TEST_FILE), os.path.join(TEST_DIR, TEST_FILE2))

    def test_fsevent_dir(self):
        self.errorno = None
        self.events = None
        self.filename = None
        fs_event = pyuv.fs.FSEvent(self.loop)
        fs_event.start(TEST_DIR, 0, self.on_fsevent_cb)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb2, 1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_RENAME)

    def timer_cb3(self, timer):
        timer.close()
        os.utime(os.path.join(TEST_DIR, TEST_FILE), None)

    def test_fsevent_nrefile(self):
        self.errorno = None
        self.events = None
        self.filename = None
        fs_event = pyuv.fs.FSEvent(self.loop)
        fs_event.start(os.path.join(TEST_DIR, TEST_FILE), 0, self.on_fsevent_cb)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb3, 1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_CHANGE)


class FSPollTest(TestCase):

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        super(FSPollTest, self).tearDown()

    def _touch_file(self):
        with open(TEST_FILE, 'w+') as f:
            self.count += 1
            for i in range(self.count+1):
                f.write('*')

    def on_timer(self, timer):
        self._touch_file()

    def on_fspoll(self, handle, prev_stat, curr_stat, error):
        if self.poll_cb_called == 0:
            self.assertEqual(error, pyuv.errno.UV_ENOENT)
            self._touch_file()
        elif self.poll_cb_called == 1:
            self.timer.start(self.on_timer, 0.02, 0.0)
        elif self.poll_cb_called == 2:
            self.timer.start(self.on_timer, 0.2, 0.0)
        elif self.poll_cb_called == 3:
            os.remove(TEST_FILE)
        elif self.poll_cb_called == 4:
            self.assertEqual(error, pyuv.errno.UV_ENOENT)
            self.fs_poll.close()
            self.timer.close()
        else:
            self.fail('This should not happen')
        self.poll_cb_called += 1

    def test_fspoll1(self):
        self.count = 0
        self.poll_cb_called = 0
        self.timer = pyuv.Timer(self.loop)
        self.fs_poll = pyuv.fs.FSPoll(self.loop)
        self.fs_poll.start(TEST_FILE, 0.1, self.on_fspoll)
        self.assertEqual(self.fs_poll.path, TEST_FILE)
        self.loop.run()
        self.assertEqual(self.poll_cb_called, 5)


if __name__ == '__main__':
    unittest.main(verbosity=2)
