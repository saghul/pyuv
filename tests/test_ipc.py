
from common import unittest2, platform_skip
from functools import partial

import pyuv
import random
import sys

TEST_PORT = random.randint(2000, 3000)
TEST_PORT2 = random.randint(2000, 3000)
TEST_ADDR = ("127.0.0.1", TEST_PORT)
TEST_ADDR2 = ("127.0.0.1", TEST_PORT2)


if sys.platform == 'win32':
    TEST_PIPE = '\\\\.\\pipe\\test-pipe'
else:
    TEST_PIPE = 'test-pipe'


class IPCTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def proc_exit_cb(self, proc, exit_status, term_signal):
        proc.close()

    def on_client_connection(self, client, error):
        client.close()
        self.connections.remove(client)

    def make_many_connections(self):
        for i in range(100):
            conn = pyuv.TCP(self.loop)
            self.connections.append(conn)
            conn.connect(TEST_ADDR, self.on_client_connection)

    def on_ipc_connection(self, handle, error):
        if self.local_conn_accepted:
            return
        conn = pyuv.TCP(self.loop)
        self.tcp_server.accept(conn)
        conn.close()
        self.tcp_server.close()
        self.local_conn_accepted = True

    def on_channel_read(self, handle, data, pending, error):
        if self.tcp_server is None:
            self.assertEqual(pending, pyuv.UV_TCP)
            self.tcp_server = pyuv.TCP(self.loop)
            self.channel.accept(self.tcp_server)
            self.tcp_server.listen(self.on_ipc_connection, 12)
            self.assertEqual(data.strip(), b"hello")
            self.channel.write(b"world")
            self.make_many_connections()
        else:
            if data.strip() == b"accepted_connection":
                self.assertEqual(pending, pyuv.UV_UNKNOWN_HANDLE)
                self.channel.close()

    def _do_test(self, test_type):
        self.connections = []
        self.local_conn_accepted = False
        self.tcp_server = None
        self.channel = pyuv.Pipe(self.loop, True)
        stdio = [pyuv.StdIO(stream=self.channel, flags=pyuv.UV_CREATE_PIPE|pyuv.UV_READABLE_PIPE|pyuv.UV_WRITABLE_PIPE)]
        proc = pyuv.Process(self.loop)
        proc.spawn(file=sys.executable, args=["proc_ipc.py", test_type, str(TEST_PORT)], exit_callback=self.proc_exit_cb, stdio=stdio)
        self.channel.start_read2(self.on_channel_read)
        self.loop.run()

    def test_ipc1(self):
        self._do_test("listen_before_write")

    def test_ipc2(self):
        self._do_test("listen_after_write")


class IPCSendRecvTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def proc_exit_cb(self, proc, exit_status, term_signal):
        proc.close()

    def on_channel_read(self, expected_type, handle, data, pending, error):
        self.assertEqual(pending, expected_type)
        if pending == pyuv.UV_NAMED_PIPE:
            recv_handle = pyuv.Pipe(self.loop)
        elif pending == pyuv.UV_TCP:
            recv_handle = pyuv.TCP(self.loop)
        elif pending == pyuv.UV_UDP:
            recv_handle = pyuv.UDP(self.loop)
        self.channel.accept(recv_handle)
        self.channel.close()
        self.send_pipe.close()
        recv_handle.close()

    def _do_test(self):
        self.channel = pyuv.Pipe(self.loop, True)
        stdio = [pyuv.StdIO(stream=self.channel, flags=pyuv.UV_CREATE_PIPE|pyuv.UV_READABLE_PIPE|pyuv.UV_WRITABLE_PIPE)]
        proc = pyuv.Process(self.loop)
        proc.spawn(file=sys.executable, args=["proc_ipc_echo.py"], exit_callback=self.proc_exit_cb, stdio=stdio)
        self.channel.write2(b".", self.send_handle)
        self.channel.start_read2(functools.partial(self.on_channel_read, self.send_handle_type))
        self.loop.run()

    @platform_skip(["win32"])
    def test_ipc_send_recv_pipe(self):
        # Handle that will be sent to the process and back
        self.send_handle = pyuv.Pipe(self.loop, True)
        self.send_handle.bind(TEST_PIPE+'2')
        self.send_handle_type = pyuv.UV_NAMED_PIPE

    def test_ipc_send_recv_tcp(self):
        # Handle that will be sent to the process and back
        self.send_handle = pyuv.TCP(self.loop)
        self.send_handle.bind(TEST_ADDR2)
        self.send_handle_type = pyuv.UV_TCP

    @platform_skip(["win32"])
    def test_ipc_send_recv_udp(self):
        # Handle that will be sent to the process and back
        self.send_handle = pyuv.UDP(self.loop)
        self.send_handle.bind(TEST_ADDR2)
        self.send_handle_type = pyuv.UV_UDP


if __name__ == '__main__':
    unittest2.main(verbosity=2)

