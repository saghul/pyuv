
import os
import shutil
import stat

import common
import pyuv


BAD_FILE = 'test_file_bad'
TEST_FILE = 'test_file_1234'
TEST_FILE2 = 'test_file_1234_2'
TEST_LINK = 'test_file_1234_link'
TEST_DIR = 'test-dir'
TEST_DIR2 = 'test-dir_2'
BAD_DIR = 'test-dir-bad'

class FSTestStat(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')
        os.symlink(TEST_FILE, TEST_LINK)

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def stat_cb(self, loop, data, path, stat_result, errorno):
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

    def lstat_cb(self, loop, data, path, stat_result, errorno):
        self.errorno = errorno

    def test_lstat(self):
        self.errorno = None
        pyuv.fs.lstat(self.loop, TEST_LINK, self.lstat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestFstat(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')
        self.file.write('test')

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def fstat_cb(self, loop, data, path, stat_data, errorno):
        self.errorno = errorno

    def test_fstat(self):
        self.errorno = None
        pyuv.fs.fstat(self.loop, self.file.fileno(), self.fstat_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


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

    def bad_unlink_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_bad_unlink(self):
        self.errorno = None
        pyuv.fs.unlink(self.loop, BAD_FILE, self.bad_unlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)

    def unlink_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_unlink(self):
        self.errorno = None
        pyuv.fs.unlink(self.loop, TEST_FILE, self.unlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


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

    def mkdir_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_bad_mkdir(self):
        self.errorno = None
        pyuv.fs.mkdir(self.loop, BAD_DIR, 0755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_EEXIST)

    def test_mkdir(self):
        self.errorno = None
        pyuv.fs.mkdir(self.loop, TEST_DIR, 0755, self.mkdir_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
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

    def rmdir_cb(self, loop, data, path, errorno):
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


class FSTestRename(common.UVTestCase):

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

    def rename_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_rename(self):
        self.errorno = None
        pyuv.fs.rename(self.loop, TEST_FILE, TEST_FILE2, self.rename_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertFalse(os.path.exists(TEST_FILE))
        self.assertTrue(os.path.exists(TEST_FILE2))


class FSTestChmod(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def chmod_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_chmod(self):
        self.errorno = None
        pyuv.fs.chmod(self.loop, TEST_FILE, 0777, self.chmod_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))


class FSTestFchmod(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')
        self.file.write('test')

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def fchmod_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_fchmod(self):
        self.errorno = None
        pyuv.fs.fchmod(self.loop, self.file.fileno(), 0777, self.fchmod_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        mode = os.stat(TEST_FILE).st_mode
        self.assertTrue(bool(mode & stat.S_IRWXU) and bool(mode & stat.S_IRWXG) and bool(mode & stat.S_IRWXO))


class FSTestLink(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def link_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_link(self):
        self.errorno = None
        pyuv.fs.link(self.loop, TEST_FILE, TEST_LINK, self.link_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(os.stat(TEST_FILE).st_ino, os.stat(TEST_LINK).st_ino)


class FSTestSymlink(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def symlink_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_symlink(self):
        self.errorno = None
        pyuv.fs.symlink(self.loop, TEST_FILE, TEST_LINK, 0, self.symlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(os.path.islink(TEST_LINK))


class FSTestReadlink(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')
        os.symlink(TEST_FILE, TEST_LINK)

    def tearDown(self):
        os.remove(TEST_FILE)
        os.remove(TEST_LINK)

    def readlink_cb(self, loop, data, path, errorno):
        self.errorno = errorno
        self.link_path = path

    def test_readlink(self):
        self.errorno = None
        self.link_path = None
        pyuv.fs.readlink(self.loop, TEST_LINK, self.readlink_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(self.link_path, TEST_FILE)


class FSTestChown(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test')

    def tearDown(self):
        os.remove(TEST_FILE)

    def chown_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_chown(self):
        self.errorno = None
        pyuv.fs.chown(self.loop, TEST_FILE, -1, -1, self.chown_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestFchown(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')
        self.file.write('test')

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def fchown_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_fchown(self):
        self.errorno = None
        pyuv.fs.fchown(self.loop, self.file.fileno(), -1, -1, self.fchown_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)


class FSTestOpen(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def tearDown(self):
        try:
            os.remove(TEST_FILE)
        except OSError:
            pass

    def close_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def open_cb(self, loop, data, path, fd, errorno):
        self.assertNotEqual(fd, -1)
        self.assertEqual(errorno, None)
        pyuv.fs.close(self.loop, fd, self.close_cb)

    def test_open_create(self):
        self.errorno = None
        pyuv.fs.open(self.loop, TEST_FILE, os.O_WRONLY|os.O_CREAT, stat.S_IREAD|stat.S_IWRITE, self.open_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)

    def open_noent_cb(self, loop, data, path, fd, errorno):
        self.fd = fd
        self.errorno = errorno

    def test_open_noent(self):
        self.fd = None
        self.errorno = None
        pyuv.fs.open(self.loop, BAD_FILE, os.O_RDONLY, 0, self.open_noent_cb)
        self.loop.run()
        self.assertEqual(self.fd, -1)
        self.assertEqual(self.errorno, pyuv.errno.UV_ENOENT)


class FSTestRead(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write('test1234567890')
        self.file = open(TEST_FILE, 'r')

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def read_cb(self, loop, data, path, read_data, errorno):
        self.errorno = errorno
        self.data = read_data

    def test_read(self):
        self.data = None
        self.errorno = None
        pyuv.fs.read(self.loop, self.file.fileno(), 4, -1, self.read_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(self.data, 'test')


class FSTestWrite(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')

    def tearDown(self):
        os.remove(TEST_FILE)

    def write_cb(self, loop, data, path, bytes_written, errorno):
        self.file.close()
        self.bytes_written = bytes_written
        self.errorno = errorno

    def test_write(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.file.fileno(), "TEST", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 4)
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")

    def test_write_null(self):
        self.bytes_written = None
        self.errorno = None
        pyuv.fs.write(self.loop, self.file.fileno(), "TES\x00T", -1, self.write_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 5)
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "TES\x00T")

class FSTestFsync(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')

    def tearDown(self):
        os.remove(TEST_FILE)

    def write_cb(self, loop, data, path, bytes_written, errorno):
        self.assertEqual(bytes_written, 4)
        self.assertEqual(errorno, None)
        pyuv.fs.fdatasync(self.loop, self.file.fileno(), self.fdatasync_cb)

    def fdatasync_cb(self, loop, data, path, errorno):
        self.assertEqual(errorno, None)
        pyuv.fs.fsync(self.loop, self.file.fileno(), self.fsync_cb)

    def fsync_cb(self, loop, data, path, errorno):
        self.assertEqual(errorno, None)

    def test_fsync(self):
        pyuv.fs.write(self.loop, self.file.fileno(), "TEST", -1, self.write_cb)
        self.loop.run()
        self.file.close()
        self.assertEqual(open(TEST_FILE, 'r').read(), "TEST")


class FSTestFtruncate(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.file = open(TEST_FILE, 'w')
        self.file.write("test-data")
        self.file.flush()

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def ftruncate_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_ftruncate1(self):
        self.errorno = None
        pyuv.fs.ftruncate(self.loop, self.file.fileno(), 4, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "test")

    def test_ftruncate2(self):
        self.errorno = None
        pyuv.fs.ftruncate(self.loop, self.file.fileno(), 0, self.ftruncate_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), "")


class FSTestReaddir(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0755)
        os.mkdir(os.path.join(TEST_DIR, TEST_DIR2), 0755)
        with open(os.path.join(TEST_DIR, TEST_FILE), 'w') as f:
            f.write('test')
        with open(os.path.join(TEST_DIR, TEST_FILE2), 'w') as f:
            f.write('test')

    def tearDown(self):
        shutil.rmtree(TEST_DIR)

    def readdir_cb(self, loop, data, path, files, errorno):
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


class FSTestSendfile(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("begin\n")
            os.lseek(f.fileno(), 65536, os.SEEK_CUR)
            f.write("end\n")
            f.flush()
        self.file = open(TEST_FILE, 'r')
        self.new_file = open(TEST_FILE2, 'w')

    def tearDown(self):
        self.file.close()
        self.new_file.close()
        os.remove(TEST_FILE)
        os.remove(TEST_FILE2)

    def sendfile_cb(self, loop, data, path, bytes_written, errorno):
        self.bytes_written = bytes_written
        self.errorno = errorno

    def test_sendfile(self):
        self.result = None
        self.errorno = None
        pyuv.fs.sendfile(self.loop, self.new_file.fileno(), self.file.fileno(), 0, 131072, self.sendfile_cb)
        self.loop.run()
        self.assertEqual(self.bytes_written, 65546)
        self.assertEqual(self.errorno, None)
        self.assertEqual(open(TEST_FILE, 'r').read(), open(TEST_FILE2, 'r').read())


class FSTestUtime(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        with open(TEST_FILE, 'w') as f:
            f.write("test")
        self.file = open(TEST_FILE, 'r')

    def tearDown(self):
        self.file.close()
        os.remove(TEST_FILE)

    def utime_cb(self, loop, data, path, errorno):
        self.errorno = errorno

    def test_utime(self):
        self.errorno = None
        atime = mtime = 400497753
        pyuv.fs.utime(self.loop, TEST_FILE, atime, mtime, self.utime_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        s = os.stat(TEST_FILE)
        self.assertTrue(s.st_atime == atime and s.st_mtime == mtime)

    def test_futime(self):
        self.errorno = None
        atime = mtime = 400497753
        pyuv.fs.futime(self.loop, self.file.fileno(), atime, mtime, self.utime_cb)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        s = os.stat(TEST_FILE)
        self.assertTrue(s.st_atime == atime and s.st_mtime == mtime)


class FSEventTestBasic(common.UVTestCase):
    __disabled__ = ["linux"]

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
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_RENAME)


class FSEventTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        os.mkdir(TEST_DIR, 0755)
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
        timer.start(self.timer_cb2, 0.1, 0)
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
        timer.start(self.timer_cb3, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.errorno, None)
        self.assertTrue(self.filename == None or self.filename == TEST_FILE)
        self.assertTrue(self.events & pyuv.fs.UV_CHANGE)


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

