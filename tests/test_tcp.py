
import os

import common
import pyuv


TEST_PORT = 1234

class TCPErrorTest(common.UVTestCase):

    def on_client_connect_error(self, client, error):
        self.assertNotEqual(error, None)
        client.close()

    def test_client1(self):
        loop = pyuv.Loop.default_loop()
        client = pyuv.TCP(loop)
        client.connect(("127.0.0.1", TEST_PORT), self.on_client_connect_error)
        loop.run()


class TCPTest(common.UVTestCase):

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
        client.write("PING"+os.linesep)

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
        self.assertEquals(data, "PING")
        client.close()

    def test_tcp1(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestNull(common.UVTestCase):

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
        client.write("PIN\x00G"+os.linesep)

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
        self.assertEquals(data, "PIN\x00G")
        client.close()

    def test_tcp_null(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestList(common.UVTestCase):

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
        client.write(["PING1", "PING2", "PING3", "PING4", "PING5", "PING6", os.linesep])

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
        self.assertEquals(data, "PING1PING2PING3PING4PING5PING6")
        client.close()

    def test_tcp_list(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestListNull(common.UVTestCase):

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
        client.write(["PING1", "PING2", "PING3", "PING4", "PING5", "PING\x00\xFF6", os.linesep])

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
        self.assertEquals(data, "PING1PING2PING3PING4PING5PING\x00\xFF6")
        client.close()

    def test_tcp_list_null(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


class TCPTestInvalidData(common.UVTestCase):

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

        self.assertRaises(TypeError, client.write, u'Unicode Data')
        self.assertRaises(TypeError, client.write, 1)

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


class TCPShutdownTest(common.UVTestCase):

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
        client.write("PING"+os.linesep)

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
        self.assertEquals(data, "PING")
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


class TCPFlagsTest(common.UVTestCase):

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
    import unittest
    tests = unittest.TestSuite(common.suites)
    unittest.TextTestRunner(verbosity=2).run(tests)

