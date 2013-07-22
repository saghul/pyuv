# coding=utf8

import socket
import sys

from common import unittest2
import common
import pyuv

try:
    memoryview
except NameError:
    # Fix for Python 2.6
    memoryview = str


TEST_PORT = 1234

class TCPErrorTest(unittest2.TestCase):

    def on_client_connect_error(self, client, error):
        self.assertNotEqual(error, None)
        client.close()

    def test_client1(self):
        loop = pyuv.Loop.default_loop()
        client = pyuv.TCP(loop)
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connect_error)
        loop.run()

    def test_client2(self):
        loop = pyuv.Loop.default_loop()
        client = pyuv.TCP(loop)
        self.assertFalse(client.readable)
        self.assertFalse(client.writable)
        client.close()
        loop.run()

    def test_open(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        loop = pyuv.Loop.default_loop()
        client = pyuv.TCP(loop)
        client.open(sock.fileno())
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connect_error)
        loop.run()
        sock.close()

    def test_raise(self):
        loop = pyuv.Loop.default_loop()
        tcp = pyuv.TCP(loop)
        self.assertRaises(pyuv.error.TCPError, tcp.write, b"PING")
        tcp.close()
        loop.run()


class TCPTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
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
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)
        self.assertTrue(client.readable)
        self.assertTrue(client.writable)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PING"+common.linesep)
        client.close()

    def test_tcp1(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTest2(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.write_cb_count = 0

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
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


class TCPTest3(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        connection = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(connection)
        while connection.write_queue_size == 0:
            connection.write(b"PING"*1000)
        connection.close()
        self.client.close()
        self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        self.assertTrue(client.readable)
        self.assertTrue(client.writable)

    def test_tcp_write_saturate(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestUnicode(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        if sys.version_info >= (3, 0):
            data = 'PÏNG'
            exc_type = TypeError
        else:
            data = unicode('PÏNG', 'utf-8')
            exc_type = UnicodeEncodeError
        self.assertRaises(exc_type, client.write, data)
        client.close()
        self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        client.close()

    def test_tcp_unicode(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestMemoryview(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
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


class TCPTestNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
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
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PIN\x00G"+common.linesep)
        client.close()

    def test_tcp_null(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestList(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.writelines([b"PING1", b"PING2", b"PING3", b"PING4", b"PING5", b"PING6", b"PING7", b"PING8", b"PING9", b"PING10", b"PING11", b"PING12"])

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


class TCPTestListUnicode(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        if sys.version_info >= (3, 0):
            data = 'PÏNG'
            exc_type = TypeError
        else:
            data = unicode('PÏNG', 'utf-8')
            exc_type = UnicodeEncodeError
        self.assertRaises(exc_type, client.writelines, [data for x in range(100)])
        client.close()
        self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        client.close()

    def test_tcp_list_unicode(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestListNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        client.writelines([b"PING1", b"PING2", b"PING3", b"PING4", b"PING5", b"PING\x00\xFF6"])

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


class TCPTestInvalidData(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)

        self.assertRaises(TypeError, client.write, 1234)
        self.assertRaises(TypeError, client.write, object())
        self.assertRaises(TypeError, client.writelines, 1234)
        self.assertRaises(TypeError, client.writelines, object())

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


class TCPShutdownTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
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
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def on_close(self, handle):
        self.close_cb_called += 1

    def on_client_shutdown(self, client, error):
        self.shutdown_cb_called += 1
        client.close(self.on_close)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEqual(data, b"PING"+common.linesep)
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


class TCPFlagsTest(unittest2.TestCase):

    def test_tcp_flags(self):
        loop = pyuv.Loop.default_loop()
        tcp = pyuv.TCP(loop)
        tcp.nodelay(True)
        tcp.keepalive(True, 60)
        tcp.simultaneous_accepts(True)
        tcp.close()
        loop.run()
        self.assertTrue(True)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

