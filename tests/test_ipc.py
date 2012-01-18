
import sys

from common import unittest2, platform_skip
import pyuv

TEST_PORT = 1234

if sys.version_info > (3, 0):
    EXECUTABLE = sys.executable.encode()
else:
    EXECUTABLE = sys.executable


@platform_skip(["win32"])
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
            conn.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)

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

    def test_ipc1(self):
        self.connections = []
        self.local_conn_accepted = False
        self.tcp_server = None
        self.channel = pyuv.Pipe(self.loop, True)
        proc = pyuv.Process(self.loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", EXECUTABLE+ b" proc_ipc.py", b"listen_before_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        else:
            proc.spawn(file=sys.executable , args=[b"proc_ipc.py", b"listen_before_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        self.channel.start_read2(self.on_channel_read)
        self.loop.run()

    def test_ipc2(self):
        self.connections = []
        self.local_conn_accepted = False
        self.tcp_server = None
        self.channel = pyuv.Pipe(self.loop, True)
        proc = pyuv.Process(self.loop)
        if sys.platform == 'win32':
            proc.spawn(file="cmd.exe", args=[b"/c", EXECUTABLE+ b" proc_ipc.py", b"listen_after_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        else:
            proc.spawn(file=sys.executable, args=[b"proc_ipc.py", b"listen_after_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        self.channel.start_read2(self.on_channel_read)
        self.loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)
