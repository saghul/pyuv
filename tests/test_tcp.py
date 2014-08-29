# coding=utf8

import os
import socket
import sys
import unittest

from common import linesep, platform_skip, TestCase
import pyuv


TEST_PORT = 1234

class TCPErrorTest(TestCase):

    def on_client_connect_error(self, client, error):
        self.assertNotEqual(error, None)
        client.close()

    def test_client1(self):
        client = pyuv.TCP(self.loop)
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connect_error)
        self.loop.run()

    def test_client2(self):
        client = pyuv.TCP(self.loop)
        self.assertFalse(client.readable)
        self.assertFalse(client.writable)
        client.close()
        self.loop.run()

    def test_open(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client = pyuv.TCP(self.loop)
        client.open(sock.fileno())
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connect_error)
        self.loop.run()
        sock.close()

    def test_raise(self):
        tcp = pyuv.TCP(self.loop)
        self.assertRaises(pyuv.error.TCPError, tcp.write, b"PING")
        tcp.close()
        self.loop.run()


class TCPTest(TestCase):

    def setUp(self):
        super(TCPTest, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(self.loop)
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
        client.start_read(self.on_client_read)
        self.assertTrue(client.readable)
        self.assertTrue(client.writable)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PING"+linesep)
        client.close()

    def test_tcp1(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()

    def test_tcp_bind(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("", TEST_PORT))
        self.server.close()
        self.loop.run()


class TCPTest2(TestCase):

    def setUp(self):
        super(TCPTest2, self).setUp()
        self.server = None
        self.client = None
        self.write_cb_count = 0

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(self.loop)
        server.accept(client)
        for x in range(1024):
            client.write(b"PING"*1000, self.on_client_write)
        client.close()
        server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.assertTrue(client.readable)
        self.assertTrue(client.writable)

    def on_client_write(self, handle, error):
        self.write_cb_count += 1

    def test_tcp_write_cancel(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()
        self.assertEqual(self.write_cb_count, 1024)


class TCPTest3(TestCase):

    def setUp(self):
        super(TCPTest3, self).setUp()
        self.server = None
        self.client = None

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        connection = pyuv.TCP(self.loop)
        server.accept(connection)
        while connection.write_queue_size == 0:
            connection.write(b"PING"*1000)
        connection.close(self.on_connection_close)

    def on_connection_close(self, connection):
        self.client.close()
        self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.assertTrue(client.readable)
        self.assertTrue(client.writable)

    def test_tcp_write_saturate(self):
        if 'TRAVIS' in os.environ:
            self.skipTest("Test disabled on Travis")
            return
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestMemoryview(TestCase):

    def setUp(self):
        super(TCPTestMemoryview, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        data = memoryview(b"PING")
        client.write(data)

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
        self.assertEqual(data, b"PING")
        client.close()

    def test_tcp_memoryview(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestNull(TestCase):

    def setUp(self):
        super(TCPTestNull, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(self.loop)
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
        self.assertEqual(data, b"PIN\x00G"+linesep)
        client.close()

    def test_tcp_null(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestList(TestCase):

    def setUp(self):
        super(TCPTestList, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING1", b"PING2", b"PING3", b"PING4", b"PING5", b"PING6", b"PING7", b"PING8", b"PING9", b"PING10", b"PING11", b"PING12"])

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
        self.assertEqual(data, b"PING1PING2PING3PING4PING5PING6PING7PING8PING9PING10PING11PING12")
        client.close()

    def test_tcp_list(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestListNull(TestCase):

    def setUp(self):
        super(TCPTestListNull, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write([b"PING1", b"PING2", b"PING3", b"PING4", b"PING5", b"PING\x00\xFF6"])

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
        self.assertEqual(data, b"PING1PING2PING3PING4PING5PING\x00\xFF6")
        client.close()

    def test_tcp_list_null(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestInvalidData(TestCase):

    def setUp(self):
        super(TCPTestInvalidData, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)

        self.assertRaises(TypeError, client.write, 1234)
        self.assertRaises(TypeError, client.write, object())
        self.assertRaises(TypeError, client.write, u"PING")
        self.assertRaises(ValueError, client.write, [])
        self.assertRaises(ValueError, client.write, ())
        self.assertRaises(TypeError, client.write, list(range(5)))
        self.assertRaises(TypeError, client.write, [b"PING", 123, u"PING"])

        client.close()
        self.client_connections.remove(client)
        self.server.close()

    def on_client_connection_read(self, client, data, error):
        client.close()
        self.client_connections.remove(client)
        self.server.close()
        self.fail('Expected write to fail.' % data)
        return

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertEqual(data, None)
        client.close()

    def test_invalid_data(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPShutdownTest(TestCase):

    def setUp(self):
        super(TCPShutdownTest, self).setUp()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+linesep)

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close(self.on_close)
            self.client_connections.remove(client)
            self.server.close(self.on_close)

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
        self.assertEqual(data, b"PING"+linesep)
        client.shutdown(self.on_client_shutdown)

    def test_tcp_shutdown(self):
        self.shutdown_cb_called = 0
        self.close_cb_called = 0
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()
        self.assertEqual(self.shutdown_cb_called, 1)
        self.assertEqual(self.close_cb_called, 3)


class TCPFlagsTest(TestCase):

    def test_tcp_flags(self):
        tcp = pyuv.TCP(self.loop)
        tcp.nodelay(True)
        tcp.keepalive(True, 60)
        tcp.simultaneous_accepts(True)
        tcp.close()
        self.loop.run()


@platform_skip(["win32"])
class TCPTryTest(TestCase):

    def setUp(self):
        super(TCPTryTest, self).setUp()
        self.server = None
        self.client = None
        self.connections = None

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.connection = client
        client.start_read(self.on_client_connection_read)
        while True:
            try:
                r = client.try_write(b"x")
            except pyuv.error.TCPError:
                continue
            if r != 0:
                break

    def on_client_connection_read(self, client, data, error):
        if data is None:
            client.close()
            self.connection = None
            self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        client.close()

    def test_tcp1_try(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestFileno(TestCase):

    def check_fileno(self, handle):
        fd = handle.fileno()
        self.assertIsInstance(fd, int)
        self.assertGreaterEqual(fd, 0)

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        self.check_fileno(server)
        client = pyuv.TCP(self.loop)
        server.accept(client)
        self.check_fileno(client)
        client.close()
        server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.check_fileno(client)
        client.close()

    def test_fileno(self):
        server = pyuv.TCP(self.loop)
        server.bind(("0.0.0.0", TEST_PORT))
        self.check_fileno(server)
        server.listen(self.on_connection)
        client = pyuv.TCP(self.loop)
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.check_fileno(client)
        self.loop.run()


if __name__ == '__main__':
    unittest.main(verbosity=2)
