
import os
import shutil
import stat

from common import unittest2
import pyuv


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_FILE2 = 'test_file_1234_2'
TEST_LINK = 'test_file_1234_link'
TEST_DIR = 'test-dir'
TEST_DIR2 = 'test-dir_2'
BAD_DIR = 'test-dir-bad'


class FSTestStat(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def stat_cb(self, loop, path, stat_result, errorno):
        self.errorno = errorno

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


class FSTestLstat(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def stat_cb(self, loop, path, stat_result, errorno):
        self.errorno = errorno

    def test_lstat(self):
        self.errorno = None
        pyuv.fs.lstat(self.loop, TEST_LINK, self.stat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestFstat(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def fstat_cb(self, loop, path, stat_data, errorno):
        self.errorno = errorno

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


class FSTestUnlink(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def bad_unlink_cb(self, loop, path, errorno):
        self.errorno = errorno

    def test_bad_unlink(self):
        self.errorno = None
        pyuv.fs.unlink(self.loop, BAD_FILE, self.bad_unlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def unlink_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestMkdir(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(BAD_DIR, 0o755)

    def tearDown(self):
        os.rmdir(BAD_DIR)
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass

    def mkdir_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestRmdir(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0o755)

    def tearDown(self):
        try:
            os.rmdir(TEST_DIR)
        except OSError:
            pass

    def rmdir_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestRename(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass

    def rename_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestChmod(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def chmod_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestFchmod(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def fchmod_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestLink(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def link_cb(self, loop, path, errorno):
        self.errorno = errorno

    def test_link(self):
        self.errorno = None
        pyuv.fs.link(self.loop, TEST_FILE, TEST_LINK, self.link_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(os.stat(TEST_FILE).st_ino, os.stat(TEST_LINK).st_ino)

    def test_link_sync(self):
        pyuv.fs.link(self.loop, TEST_FILE, TEST_LINK)
        self.assertEqual(os.stat(TEST_FILE).st_ino, os.stat(TEST_LINK).st_ino)


class FSTestSymlink(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)
        try:
            os.remove(TEST_LINK)
        except OSError:
            pass

    def symlink_cb(self, loop, path, errorno):
        self.errorno = errorno

    def test_symlink(self):
        self.errorno = None
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0, self.symlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(os.stat(TEST_LINK).st_mode & stat.S_IFLNK)

    def test_symlink_sync(self):
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)
        self.assertTrue(os.stat(TEST_LINK).st_mode & stat.S_IFLNK)


class FSTestReadlink(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0)

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def readlink_cb(self, loop, path, errorno):
        self.errorno = errorno
        self.link_path = path

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


class FSTestChown(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def chown_cb(self, loop, path, errorno):
        self.errorno = errorno

    def test_chown(self):
        self.errorno = None
        pyuv.fs.chown(self.loop, TEST_FILE, -1, -1, self.chown_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def test_chown_sync(self):
        pyuv.fs.chown(self.loop, TEST_FILE, -1, -1)


class FSTestFchown(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def fchown_cb(self, loop, path, errorno):
        self.errorno = errorno

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


class FSTestOpen(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def close_cb(self, loop, path, errorno):
        self.errorno = errorno

    def open_cb(self, loop, path, fd, errorno):
        self.assertNotEqual(fd, None)
        self.assertEqual(errorno, None)
        pyuv.fs.close(self.loop, fd, self.close_cb)

    def test_open_create(self):
        self.errorno = None
        pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE, self.open_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def open_noent_cb(self, loop, path, fd, errorno):
        self.fd = fd
        self.errorno = errorno

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


class FSTestRead(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test1234567890')

    def tearDown(self):
        os.remove(TEST_FILE)

    def read_cb(self, loop, path, read_data, errorno):
        self.errorno = errorno
        self.data = read_data
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


class FSTestWrite(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT|os.O_TRUNC, stat.S_IWRITE|stat.S_IREAD)

    def tearDown(self):
        os.remove(TEST_FILE)

    def write_cb(self, loop, path, bytes_written, errorno):
        pyuv.fs.close(self.loop, self.fd)
        self.bytes_written = bytes_written
        self.errorno = errorno

    def test_write(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.fd, "TEST", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 4)
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")

    def test_write_null(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.fd, "TES\x00T", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 5)
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TES\x00T")

    def test_write_sync(self):
        self.bytes_written = pyuv.fs.write(self.loop, self.fd, "TEST", -1)
        pyuv.fs.close(self.loop, self.fd)
        self.assertEqual(self.bytes_written, 4)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")


class FSTestFsync(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def write_cb(self, loop, path, bytes_written, errorno):
        self.assertEqual(bytes_written, 4)
        self.assertEqual(errorno, None)
        pyuv.fs.fdatasync(self.loop, self.fd, self.fdatasync_cb)

    def fdatasync_cb(self, loop, path, errorno):
        self.assertEqual(errorno, None)
        pyuv.fs.fsync(self.loop, self.fd, self.fsync_cb)

    def fsync_cb(self, loop, path, errorno):
        self.assertEqual(errorno, None)

    def test_fsync(self):
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR|os.O_CREAT|os.O_TRUNC, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.write(self.loop, self.fd, "TEST", -1, self.write_cb)
        self.loop.run()
        pyuv.fs.close(self.loop, self.fd)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")

    def test_fsync_sync(self):
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR|os.O_CREAT|os.O_TRUNC, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.write(self.loop, self.fd, "TEST", -1)
        pyuv.fs.fdatasync(self.loop, self.fd)
        pyuv.fs.fsync(self.loop, self.fd)
        pyuv.fs.close(self.loop, self.fd)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")


class FSTestFtruncate(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("test-data")

    def tearDown(self):
        os.remove(TEST_FILE)

    def ftruncate_cb(self, loop, path, errorno):
        self.errorno = errorno
        pyuv.fs.close(self.loop, self.fd)

    def test_ftruncate1(self):
        self.errorno = None
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, self.fd, 4, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "test")

    def test_ftruncate2(self):
        self.errorno = None
        self.fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, self.fd, 0, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "")

    def test_ftruncate_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        pyuv.fs.ftruncate(self.loop, fd, 0)
        pyuv.fs.close(self.loop, fd)
        self.assertEqual(open(TEST_FILE, 'r').read(), "")


class FSTestReaddir(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0o755)
        os.mkdir(os.path.join(TEST_DIR, TEST_DIR2), 0o755)
        with open(os.path.join(TEST_DIR, TEST_FILE), 'w') as f:
            f.write('test')
        with open(os.path.join(TEST_DIR, TEST_FILE2), 'w') as f:
            f.write('test')

    def tearDown(self):
        shutil.rmtree(TEST_DIR)

    def readdir_cb(self, loop, path, files, errorno):
        self.errorno = errorno
        self.files = files

    def test_bad_readdir(self):
        self.errorno = None
        self.files = None
        pyuv.fs.readdir(self.loop, BAD_DIR, 0, self.readdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def test_readdir(self):
        self.errorno = None
        self.files = None
        pyuv.fs.readdir(self.loop, TEST_DIR, 0, self.readdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(TEST_FILE in self.files)
        self.assertTrue(TEST_FILE2 in self.files)
        self.assertTrue(TEST_DIR2 in self.files)

    def test_readdir_sync(self):
        self.files = pyuv.fs.readdir(self.loop, TEST_DIR, 0)
        self.assertNotEqual(self.files, None)
        self.assertTrue(TEST_FILE in self.files)
        self.assertTrue(TEST_FILE2 in self.files)
        self.assertTrue(TEST_DIR2 in self.files)

    def test_readdir_error_sync(self):
        try:
            pyuv.fs.readdir(self.loop, BAD_DIR, 0)
        except pyuv.error.FSError as e:
            self.errorno = e.args[0]
        else:
            self.errorno = None
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestSendfile(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("begin\n")
            os.lseek(f.fileno(), 65536, os.SEEK_CUR)
            f.write("end\n")
            f.flush()

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_FILE2)

    def sendfile_cb(self, loop, path, bytes_written, errorno):
        self.bytes_written = bytes_written
        self.errorno = errorno

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
        self.assertEqual(open(TEST_FILE, 'r').read(), open(TEST_FILE2, 'r').read())

    def test_sendfile_sync(self):
        fd = pyuv.fs.open(self.loop, TEST_FILE, os.O_RDWR, stat.S_IREAD|stat.S_IWRITE)
        fd2 = pyuv.fs.open(self.loop, TEST_FILE2, os.O_RDWR|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE)
        self.bytes_written = pyuv.fs.sendfile(self.loop, fd2, fd, 0, 131072)
        pyuv.fs.close(self.loop, fd)
        pyuv.fs.close(self.loop, fd2)
        self.assertEqual(open(TEST_FILE, 'r').read(), open(TEST_FILE2, 'r').read())


class FSTestUtime(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("test")
        self.fd = None

    def tearDown(self):
        os.remove(TEST_FILE)

    def utime_cb(self, loop, path, errorno):
        self.errorno = errorno
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


class FSEventTestBasic(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("test")

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass

    def on_fsevent_cb(self, handle, filename, events, errorno):
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
        fs_event.start(TEST_FILE, self.on_fsevent_cb, 0)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_RENAME)


class FSEventTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0o755)
        with open(os.path.join(TEST_DIR, TEST_FILE), 'w') as f:
            f.write("test")
        with open(TEST_FILE, 'w') as f:
            f.write("test")

    def tearDown(self):
        shutil.rmtree(TEST_DIR)
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass
        try:
            os.remove(TEST_FILE2)
        except OSError:
            pass

    def on_fsevent_cb(self, handle, filename, events, errorno):
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
        fs_event.start(TEST_DIR, self.on_fsevent_cb, 0)
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
        fs_event.start(os.path.join(TEST_DIR, TEST_FILE), self.on_fsevent_cb, 0)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb3, 1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_CHANGE)


class FSPollTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

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
        self.fs_poll.start(TEST_FILE, self.on_fspoll, 0.1)
        self.loop.run()
        self.assertEqual(self.poll_cb_called, 5)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

