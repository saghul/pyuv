
import os
import sys

from common import unittest2, platform_skip
import common
import pyuv


TEST_PORT = 12345
TEST_PORT2 = 12346
MULTICAST_ADDRESS = "239.255.0.1"

class UDPTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PING")
        self.server.send(b"PONG"+common.linesep, (ip, port))

    def on_client_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(b"PING"+common.linesep, ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.set_broadcast(True) # for coverage
        try:
            self.server.set_ttl(10) # for coverage
        except pyuv.error.UDPError:
            # This function is not implemented on Windows
            pass
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


class UDPTestNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PIN\x00G")
        self.server.send(b"PONG"+common.linesep, (ip, port))

    def on_client_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(b"PIN\x00G"+common.linesep, ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong_null(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


class UDPTestList(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PING")
        self.server.send([b"PONG", common.linesep], (ip, port))

    def on_client_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send([b"PING", common.linesep], ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong_list(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


class UDPTestListNull(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PIN\x00G")
        self.server.send([b"PONG", common.linesep], (ip, port))

    def on_client_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        data = data.strip()
        self.assertEquals(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send([b"PIN\x00G", common.linesep], ("127.0.0.1", TEST_PORT))
        timer.close(self.on_close)

    def test_udp_pingpong_list_null(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


class UDPTestInvalidData(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, data):
        ip, port = ip_port
        self.client.close(self.on_close)
        self.server.close(self.on_close)
        self.fail("Expected send to fail.")

    def timer_cb(self, timer):
        if sys.version_info >= (3, 0):
            data = 'Unicode'
        else:
            data = unicode('Unicode')
        self.assertRaises(TypeError, self.client.send, data+os.linesep, ("127.0.0.1", TEST_PORT))
        self.assertRaises(TypeError, self.client.send, 1, ("127.0.0.1", TEST_PORT))

        self.client.close(self.on_close)
        self.server.close(self.on_close)
        timer.close(self.on_close)

    def test_udp_invalid_data(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.start_recv(self.on_server_recv)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.1, 0)
        self.loop.run()
        self.assertEqual(self.on_close_called, 3)


@platform_skip(["win32"])
class UDPTestMulticast(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.server = None
        self.client = None
        self.received_data = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_client_recv(self, handle, ip_port, data, error):
        ip, port = ip_port
        self.received_data = data.strip()
        self.client.set_membership(MULTICAST_ADDRESS, pyuv.UV_LEAVE_GROUP)
        self.client.close(self.on_close)

    def on_server_send(self, handle, error):
        handle.close(self.on_close)

    def test_udp_multicast(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self.client = pyuv.UDP(self.loop)
        self.client.bind((MULTICAST_ADDRESS, TEST_PORT))
        self.client.set_membership(MULTICAST_ADDRESS, pyuv.UV_JOIN_GROUP)
        self.client.set_multicast_ttl(10)
        self.client.start_recv(self.on_client_recv)
        self.server.send(b"PING", (MULTICAST_ADDRESS, TEST_PORT), self.on_server_send)
        self.loop.run()
        self.assertEqual(self.on_close_called, 2)
        self.assertEquals(self.received_data, b"PING")

    def test_udp_multicast_loop(self):
        self.on_close_called = 0
        self.client = pyuv.UDP(self.loop)
        self.client.bind((MULTICAST_ADDRESS, TEST_PORT))
        self.client.set_membership(MULTICAST_ADDRESS, pyuv.UV_JOIN_GROUP)
        self.client.set_multicast_loop(True)
        self.client.start_recv(self.on_client_recv)
        self.client.send(b"PING", (MULTICAST_ADDRESS, TEST_PORT))
        self.loop.run()
        self.assertEqual(self.on_close_called, 1)
        self.assertEquals(self.received_data, b"PING")


if __name__ == '__main__':
    unittest2.main(verbosity=2)

