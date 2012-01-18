
import sys

from common import unittest2
import common
import pyuv


class ProcessTest(unittest2.TestCase):

    def test_process_basic(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        def proc_close_cb(proc):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.assertEqual(exit_status, 0)
            self.exit_cb_called += 1
            proc.close(proc_close_cb)
        loop = pyuv.Loop.default_loop()
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_basic.py"], exit_callback=proc_exit_cb)
        else:
            proc.spawn(file="./proc_basic.py", exit_callback=proc_exit_cb)
        pid = proc.pid
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 1)
        self.assertNotEqual(pid, None)

    def test_process_cwd(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        def proc_close_cb(proc):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.assertEqual(exit_status, 0)
            self.exit_cb_called += 1
            proc.close(proc_close_cb)
        loop = pyuv.Loop.default_loop()
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_basic.py"], exit_callback=proc_exit_cb, cwd=".")
        else:
            proc.spawn(file="./proc_basic.py", exit_callback=proc_exit_cb, cwd=".")
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 1)

    def test_process_stdout(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        self.received_output = None
        def handle_close_cb(handle):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.assertEqual(exit_status, 0)
            self.exit_cb_called += 1
            proc.close(handle_close_cb)
        def stdout_read_cb(handle, data, error):
            self.received_output = data.strip()
            handle.close(handle_close_cb)
        loop = pyuv.Loop.default_loop()
        stdout_pipe = pyuv.Pipe(loop)
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_stdout.py"], exit_callback=proc_exit_cb, stdout=stdout_pipe)
        else:
            proc.spawn(file="./proc_stdout.py", exit_callback=proc_exit_cb, stdout=stdout_pipe)
        stdout_pipe.start_read(stdout_read_cb)
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 2)
        self.assertEqual(self.received_output, b"TEST")

    def test_process_args(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        self.received_output = None
        def handle_close_cb(handle):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.assertEqual(exit_status, 0)
            self.exit_cb_called += 1
            proc.close(handle_close_cb)
        def stdout_read_cb(handle, data, error):
            self.received_output = data.strip()
            handle.close(handle_close_cb)
        loop = pyuv.Loop.default_loop()
        stdout_pipe = pyuv.Pipe(loop)
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_args_stdout.py", b"TEST"], exit_callback=proc_exit_cb, stdout=stdout_pipe)
        else:
            proc.spawn(file="./proc_args_stdout.py", args=[b"TEST"], exit_callback=proc_exit_cb, stdout=stdout_pipe)
        stdout_pipe.start_read(stdout_read_cb)
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 2)
        self.assertEqual(self.received_output, b"TEST")

    def test_process_env(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        self.received_output = None
        def handle_close_cb(handle):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.assertEqual(exit_status, 0)
            self.exit_cb_called += 1
            proc.close(handle_close_cb)
        def stdout_read_cb(handle, data, error):
            self.received_output = data.strip()
            handle.close(handle_close_cb)
        loop = pyuv.Loop.default_loop()
        stdout_pipe = pyuv.Pipe(loop)
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_env_stdout.py"], env={b"TEST": b"TEST"}, exit_callback=proc_exit_cb, stdout=stdout_pipe)
        else:
            proc.spawn(file="./proc_env_stdout.py", env={b"TEST": b"TEST"}, exit_callback=proc_exit_cb, stdout=stdout_pipe)
        stdout_pipe.start_read(stdout_read_cb)
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 2)
        self.assertEqual(self.received_output, b"TEST")

    def test_process_stdin(self):
        if sys.platform == 'win32':
            # TODO: fix
            return
        self.exit_cb_called = 0
        self.close_cb_called = 0
        self.received_output = None
        self.exit_status = -1
        self.term_signal = 0
        def handle_close_cb(handle):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.exit_cb_called += 1
            self.exit_status = exit_status
            self.term_signal = term_signal
            proc.close(handle_close_cb)
        def stdout_read_cb(handle, data, error):
            if data:
                self.received_output = data.strip()
            handle.close(handle_close_cb)
        def stdin_write_cb(handle, error):
            handle.close(handle_close_cb)
        loop = pyuv.Loop.default_loop()
        stdin_pipe = pyuv.Pipe(loop)
        stdout_pipe = pyuv.Pipe(loop)
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=["/c", "proc_stdin_stdout.py"], exit_callback=proc_exit_cb, stdin=stdin_pipe, stdout=stdout_pipe)
        else:
            proc.spawn(file="./proc_stdin_stdout.py", exit_callback=proc_exit_cb, stdin=stdin_pipe, stdout=stdout_pipe)
        stdout_pipe.start_read(stdout_read_cb)
        stdin_pipe.write(b"TEST"+common.linesep, stdin_write_cb)
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 3)
        self.assertEqual(self.received_output, b"TEST")

    def test_process_kill(self):
        self.exit_cb_called = 0
        self.close_cb_called = 0
        self.exit_status = -1
        self.term_signal = 0
        def handle_close_cb(proc):
            self.close_cb_called +=1
        def proc_exit_cb(proc, exit_status, term_signal):
            self.exit_cb_called += 1
            self.exit_status = exit_status
            self.term_signal = term_signal
            proc.close(handle_close_cb)
        def timer_cb(timer):
            timer.close(handle_close_cb)
            proc.kill(15)
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)
        timer.start(timer_cb, 0.1, 0)
        proc = pyuv.Process(loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", b"proc_infinite.py"], exit_callback=proc_exit_cb)
        else:
            proc.spawn(file="./proc_infinite.py", exit_callback=proc_exit_cb)
        loop.run()
        self.assertEqual(self.exit_cb_called, 1)
        self.assertEqual(self.close_cb_called, 2)
        if sys.platform == 'win32':
            self.assertEqual(self.exit_status, 1)
        else:
            self.assertEqual(self.exit_status, 0)
        self.assertEqual(self.term_signal, 15)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

