
import os
import common
import unittest

import pyuv


TEST_PORT = 12345
TEST_PORT2 = 12346

class UDPTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PING")
        self.server.send("PONG"+os.linesep, (ip, port))

    def on_client_recv(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer, data):
        self.client.send("PING"+os.linesep, ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


class UDPTestList(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PING")
        self.server.send(["PONG", os.linesep], (ip, port))

    def on_client_recv(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer, data):
        self.client.send(["PING", os.linesep], ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


if __name__ == '__main__':
    unittest.main()

