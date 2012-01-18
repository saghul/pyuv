
import sys

from common import unittest2, platform_skip
import common
import pyuv

if sys.platform == 'win32':
    TEST_PIPE = '\\\\.\\pipe\\test-pipe'
else:
    TEST_PIPE = 'test-pipe'

BAD_PIPE = '/pipe/that/does/not/exist'


class PipeErrorTest(unittest2.TestCase):

    def on_client_connect_error(self, client_pipe, error):
        self.assertNotEqual(error, None)
        client_pipe.close()

    def test_client1(self):
        loop = pyuv.Loop.default_loop()
        client = pyuv.Pipe(loop)
        client.connect(BAD_PIPE, self.on_client_connect_error)
        loop.run()


class PipeTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+common.linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, b"PING")
        client.close()

    def test_pipe1(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


class PipeTestNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PIN\x00G"+common.linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, b"PIN\x00G")
        client.close()

    def test_pipe_null(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


@platform_skip(["win32"])
class PipeTestList(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING", common.linesep])

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, b"PING")
        client.close()

    def test_pipe_list(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


@platform_skip(["win32"])
class PipeTestListNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING", b"\x00", common.linesep])

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, b"PING\x00")
        client.close()

    def test_pipe_list_null(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


class PipeShutdownTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+common.linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close(self.on_close)
            self.client_connections.remove(client)
            self.server.close(self.on_close)
            return

    def on_client_connection(self, client, error):
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_close(self, handle):
        self.close_cb_called += 1

    def on_client_shutdown(self, client, error):
        self.shutdown_cb_called += 1
        client.close(self.on_close)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, b"PING")
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


if __name__ == '__main__':
    unittest2.main(verbosity=2)

