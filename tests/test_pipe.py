
import sys
import unittest

from common import linesep, platform_skip, TestCase
import pyuv

if sys.platform == 'win32':
    TEST_PIPE = '\\\\.\\pipe\\test-pipe'
else:
    TEST_PIPE = 'test-pipe'

BAD_PIPE = '/pipe/that/does/not/exist'


class PipeErrorTest(TestCase):

    def on_client_connect_error(self, client_pipe, error):
        self.assertNotEqual(error, None)
        client_pipe.close()

    def test_client1(self):
        client = pyuv.Pipe(self.loop)
        client.connect(BAD_PIPE, self.on_client_connect_error)
        self.loop.run()


class PipeBlockingTest(TestCase):

    def test_pipe_set_blocking(self):
        pipe = pyuv.Pipe(self.loop)
        pipe.bind(TEST_PIPE)
        pipe.set_blocking(True)
        pipe.set_blocking(False)
        pipe.close()
        self.loop.run()


class PipeTestCase(TestCase):

    def setUp(self):
        super(PipeTestCase, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []


class PipeTest(PipeTestCase):

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.assertEqual(client.getpeername(), TEST_PIPE)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEqual(data, b"PING")
        client.close()

    def test_pipe1(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.assertEqual(self.server.getsockname(), TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


class PipeTestNull(PipeTestCase):

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PIN\x00G"+linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEqual(data, b"PIN\x00G")
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
class PipeTestList(PipeTestCase):

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING", linesep])

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PING"+linesep)
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
class PipeTestListNull(PipeTestCase):

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING", b"\x00", linesep])

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PING\x00"+linesep)
        client.close()

    def test_pipe_list_null(self):
        self.server = pyuv.Pipe(self.loop)
        self.server.pending_instances(100)
        self.server.bind(TEST_PIPE)
        self.server.listen(self.on_connection)
        self.client = pyuv.Pipe(self.loop)
        self.client.connect(TEST_PIPE, self.on_client_connection)
        self.loop.run()


class PipeShutdownTest(PipeTestCase):

    def on_connection(self, server, error):
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close(self.on_close)
            self.client_connections.remove(client)
            self.server.close(self.on_close)
            return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_close(self, handle):
        self.close_cb_called += 1

    def on_client_shutdown(self, client, error):
        self.shutdown_cb_called += 1
        client.close(self.on_close)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEqual(data, b"PING")
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


class PipeTestFileno(TestCase):

    def check_fileno(self, handle):
        fd = handle.fileno()
        self.assertIsInstance(fd, int)
        self.assertGreaterEqual(fd, 0)

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        self.check_fileno(server)
        client = pyuv.Pipe(self.loop)
        server.accept(client)
        self.check_fileno(client)
        client.close()
        server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.check_fileno(client)
        client.close()

    def test_fileno(self):
        server = pyuv.Pipe(self.loop)
        server.bind(TEST_PIPE)
        self.check_fileno(server)
        server.listen(self.on_connection)
        client = pyuv.Pipe(self.loop)
        client.connect(TEST_PIPE, self.on_client_connection)
        self.check_fileno(client)
        self.loop.run()


@platform_skip(["win32"])
class PipeBytesBind(TestCase):

    def test_bind_bytes(self):
        p = pyuv.Pipe(self.loop)
        p.bind(b'test-pipe')
        p.close()
        self.loop.run()


if __name__ == '__main__':
    unittest.main(verbosity=2)
