
import os
import common
import unittest

import pyuv


TEST_PORT = 1234

class TCPErrorTest(common.UVTestCase):

    def on_client_connect_error(self, client_pipe, status):
        self.assertNotEqual(status, 0)
        client_pipe.close()

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

    def on_connection(self, server):
        client = server.accept()
        self.client_connections.append(client)
        client.start_reading(self.on_client_connection_read)
        client.write("PING"+os.linesep)

    def on_client_connection_read(self, client, data):
        if data is None:
            client.close()
            self.client_connections.remove(client)
            self.server.close()
            return

    def on_client_connection(self, client, status):
        self.assertEquals(status, 0)
        client.start_reading(self.on_client_read)

    def on_client_read(self, client, data):
        self.assertNotEqual(data, None)
        data = data.strip()
        self.assertEquals(data, "PING")
        client.close()

    def test_pipe1(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.client = pyuv.TCP(self.loop)
        self.client.connect(("127.0.0.1", TEST_PORT), self.on_client_connection)
        self.loop.run()


if __name__ == '__main__':
    unittest.main()

