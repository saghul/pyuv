
import os
import threading

import common
import pyuv


TEST_PIPE = 'test-pipe'
BAD_PIPE = '/pipe/that/does/not/exist'

class PipeErrorTest(common.UVTestCase):

    def on_client_connect_error(self, client_pipe, status):
        self.assertNotEqual(status, 0)
        client_pipe.close()

    def test_client1(self):
        loop = pyuv.Loop.default_loop()
        client = pyuv.Pipe(loop)
        client.connect(BAD_PIPE, self.on_client_connect_error)
        loop.run()


class PipeTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write("PIN\x00G"+os.linesep)

    def on_client_connection_read(self, client, data):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, status):
        self.assertEquals(status, 0)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, "PIN\x00G")
        client.close()

    def test_pipe1(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()

class PipeTestList(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(["PING", '\x00', os.linesep])

    def on_client_connection_read(self, client, data):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, status):
        self.assertEquals(status, 0)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, "PING\x00")
        client.close()

    def test_pipe1(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()

class PipeShutdownTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write("PING"+os.linesep)

    def on_client_connection_read(self, client, data):
        if data is None:
            client.close(self.on_close)
            self.client_connections.remove(client)
            self.server.close(self.on_close)
            return

    def on_client_connection(self, client, status):
        self.assertEquals(status, 0)
        client.start_read(self.on_client_read)

    def on_close(self, handle):
        self.close_cb_called += 1

    def on_client_shutdown(self, client):
        self.shutdown_cb_called += 1
        client.close(self.on_close)

    def on_client_read(self, client, data):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, "PING")
        client.shutdown(self.on_client_shutdown)

    def test_pipe1(self):
        self.shutdown_cb_called = 0
        self.close_cb_called = 0
        self.server = pyuv.Pipe(self.loop)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()
        self.assertEqual(self.shutdown_cb_called, 1)
        self.assertEqual(self.close_cb_called, 3)


class PipePairTest(common.UVTestCase):

    def on_pipe1_read(self, handle, data):
        self.data = data
        handle.close()

    def on_pipe2_read(self, handle, data):
        handle.close()

    def run_loop2(self):
        self.pipe2.write("PING")
        self.pipe2.start_read(self.on_pipe2_read)
        self.loop2.run()

    def test_pipe_pair(self):
        self.data = None
        self.loop1 = pyuv.Loop()
        self.loop2 = pyuv.Loop()
        self.pipe1 = pyuv.Pipe(self.loop1)
        self.pipe2 = pyuv.Pipe(self.loop2)
        pyuv.Pipe.pair(self.pipe1, self.pipe2)
        self.pipe1.start_read(self.on_pipe1_read)
        t = threading.Thread(target=self.run_loop2)
        t.start()
        self.loop1.run()
        t.join()
        self.assertEqual(self.data, "PING")


if __name__ == '__main__':
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

