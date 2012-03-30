
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
        self.assertEquals(data, b"PING"+common.linesep)
        client.close()

    def test_tcp1(self):
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
        self.client_connections = []

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        if sys.version_info >= (3, 0):
            data = 'PING'
        else:
            data = unicode('PING')
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
        self.assertEquals(data, b"PIN\x00G"+common.linesep)
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
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEquals(data, b"PING1PING2PING3PING4PING5PING6PING7PING8PING9PING10PING11PING12")
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
        self.client_connections = []

    def on_connection(self, server, error):
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connections.append(client)
        client.start_read(self.on_client_connection_read)
        if sys.version_info >= (3, 0):
            data = 'PING'
        else:
            data = unicode('PING')
        client.writelines([data for x in range(100)])

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
        self.assertEquals(data, b"PING"*100)
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
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEquals(data, b"PING1PING2PING3PING4PING5PING\x00\xFF6")
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
        self.assertEquals(error, None)
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
        self.assertEquals(error, None)
        client.start_read(self.on_client_read)

    def on_close(self, handle):
        self.close_cb_called += 1

    def on_client_shutdown(self, client, error):
        self.shutdown_cb_called += 1
        client.close(self.on_close)

    def on_client_read(self, client, data, error):
        self.assertNotEqual(data, None)
        self.assertEquals(data, b"PING"+common.linesep)
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

