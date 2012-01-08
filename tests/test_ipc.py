
import common
import pyuv


TEST_PORT = 1234

class IPCTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()

    def proc_exit_cb(self, proc, exit_status, term_signal):
        proc.close()

    def on_client_connection(self, client, error):
        client.close()
        self.connections.remove(client)

    def make_many_connections(self):
        for i in xrange(100):
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
            self.assertEqual(data.strip(), "hello")
            self.channel.write("world")
            self.make_many_connections()
        else:
            if data.strip() == "accepted_connection":
                self.assertEqual(pending, pyuv.UV_UNKNOWN_HANDLE)
                self.channel.close()

    def test_ipc1(self):
        self.connections = []
        self.local_conn_accepted = False
        self.tcp_server = None
        self.channel = pyuv.Pipe(self.loop, True)
        proc = pyuv.Process(self.loop)
        proc.spawn(file="./proc_ipc.py", args=["listen_before_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        self.channel.start_read2(self.on_channel_read)
        self.loop.run()

    def test_ipc2(self):
        self.connections = []
        self.local_conn_accepted = False
        self.tcp_server = None
        self.channel = pyuv.Pipe(self.loop, True)
        proc = pyuv.Process(self.loop)
        proc.spawn(file="./proc_ipc.py", args=["listen_after_write"], exit_callback=self.proc_exit_cb, stdin=self.channel)
        self.channel.start_read2(self.on_channel_read)
        self.loop.run()


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

