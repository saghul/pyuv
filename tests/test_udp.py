
import os
import common
import unittest

import pyuv


class UDPTest(common.UVTestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_server_read(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PING")
        self.server.write("PONG"+os.linesep, (ip, port))

    def on_client_read(self, handle, (ip, port), data):
        data = data.strip()
        self.assertEquals(data, "PONG")
        self.client.close()
        self.server.close()

    def timer_cb(self, timer, data):
        self.client.write("PING"+os.linesep, ("127.0.0.1", common.TEST_PORT))
        timer.close()

    def test_udp_pingpong(self):
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", common.TEST_PORT))
        self.server.start_read(self.on_server_read)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", common.TEST_PORT2))
        self.client.start_read(self.on_client_read)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 1, 0)
        self.loop.run()


if __name__ == '__main__':
    unittest.main()

