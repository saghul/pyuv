
import socket
import sys
import unittest

from common import linesep, platform_skip, TestCase
import pyuv


TEST_PORT = 12345
TEST_PORT2 = 12346
MULTICAST_ADDRESS = "239.255.0.1"


class UDPTest(TestCase):

    def setUp(self):
        super(UDPTest, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PING")
        self.server.send((ip, port), b"PONG"+linesep)

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(("127.0.0.1", TEST_PORT), b"PING"+linesep)
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


class UDPEmptyDatagramTest(TestCase):

    def setUp(self):
        super(UDPEmptyDatagramTest, self).setUp()
        self.server = None
        self.client = None
        self.on_close_called = 0

    def on_close(self, handle):
        self.on_close_called += 1

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        self.assertEqual(error, None)
        self.assertEqual(data, b"")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def test_udp_empty_datagram(self):
        self.server = pyuv.UDP(self.loop)
        self.client = pyuv.UDP(self.loop)
        self.client.bind(("0.0.0.0", TEST_PORT2))
        self.client.start_recv(self.on_client_recv)
        self.server.send(("127.0.0.1", TEST_PORT2), b"")
        self.loop.run()
        self.assertEqual(self.on_close_called, 2)


class UDPTestNull(TestCase):

    def setUp(self):
        super(UDPTestNull, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PIN\x00G")
        self.server.send((ip, port), b"PONG"+linesep)

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(("127.0.0.1", TEST_PORT), b"PIN\x00G"+linesep)
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


class UDPTestList(TestCase):

    def setUp(self):
        super(UDPTestList, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PING")
        self.server.send((ip, port), [b"PONG", linesep])

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(("127.0.0.1", TEST_PORT), [b"PING", linesep])
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


class UDPTestListNull(TestCase):

    def setUp(self):
        super(UDPTestListNull, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PIN\x00G")
        self.server.send((ip, port), [b"PONG", linesep])

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PONG")
        self.client.close(self.on_close)
        self.server.close(self.on_close)

    def timer_cb(self, timer):
        self.client.send(("127.0.0.1", TEST_PORT), [b"PIN\x00G", linesep])
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


class UDPTestInvalidData(TestCase):

    def setUp(self):
        super(UDPTestInvalidData, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        self.client.close(self.on_close)
        self.server.close(self.on_close)
        self.fail("Expected send to fail.")

    def timer_cb(self, timer):
        self.assertRaises(TypeError, self.client.send, ("127.0.0.1", TEST_PORT), object())
        self.assertRaises(TypeError, self.client.send, ("127.0.0.1", TEST_PORT), 1)
        self.assertRaises(TypeError, self.client.send, ("127.0.0.1", TEST_PORT), object())
        self.assertRaises(TypeError, self.client.send, ("127.0.0.1", TEST_PORT), 1)

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


if sys.platform == 'win32':
    def interface_addresses():
        for family, _, _, _, sockaddr in socket.getaddrinfo('', None):
            if family == socket.AF_INET:
                yield sockaddr[0]
else:
    def interface_addresses():
        yield MULTICAST_ADDRESS


class UDPTestMulticast(TestCase):

    def setUp(self):
        super(UDPTestMulticast, self).setUp()
        self.server = None
        self.clients = None
        self.received_data = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_client_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        self.received_data = data.strip()

        for client in self.clients:
            client.set_membership(MULTICAST_ADDRESS, pyuv.UV_LEAVE_GROUP)
            client.close(self.on_close)

    def on_server_send(self, handle, error):
        handle.close(self.on_close)

    def _create_clients(self, loop=False):
        self.clients = list()
        for addr in interface_addresses():
            client = pyuv.UDP(self.loop)
            client.bind((addr, TEST_PORT))
            client.set_membership(MULTICAST_ADDRESS, pyuv.UV_JOIN_GROUP)
            if loop:
                client.set_multicast_loop(True)
            else:
                client.set_multicast_ttl(10)
            client.start_recv(self.on_client_recv)
            self.clients.append(client)

    def test_udp_multicast(self):
        self.on_close_called = 0
        self.server = pyuv.UDP(self.loop)
        self._create_clients()
        self.server.send((MULTICAST_ADDRESS, TEST_PORT), b"PING", self.on_server_send)
        self.loop.run()
        self.assertEqual(self.on_close_called, 1 + len(self.clients))
        self.assertEqual(self.received_data, b"PING")

    @platform_skip(["darwin"])
    def test_udp_multicast_loop(self):
        self.on_close_called = 0
        self._create_clients(True)
        for client in self.clients:
            client.send((MULTICAST_ADDRESS, TEST_PORT), b"PING")
        self.loop.run()
        self.assertEqual(self.on_close_called, len(self.clients))
        self.assertEqual(self.received_data, b"PING")


class UDPTestBigDatagram(TestCase):

    def send_cb(self, handle, error):
        self.handle.close()
        self.errorno = error

    def test_udp_big_datagram(self):
        self.errorno = None
        self.handle = pyuv.UDP(self.loop)
        data = b"X"*65536
        self.handle.send(("127.0.0.1", TEST_PORT), data, self.send_cb)
        self.loop.run()
        self.assertEqual(self.errorno, pyuv.errno.UV_EMSGSIZE)


class UDPTestOpen(TestCase):

    def test_udp_open(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        handle = pyuv.UDP(self.loop)
        handle.open(sock.fileno())
        try:
            handle.bind(("1.2.3.4", TEST_PORT))
        except pyuv.error.UDPError as e:
            self.assertEqual(e.args[0], pyuv.errno.UV_EADDRNOTAVAIL)
        self.loop.run()
        sock.close()


class UDPTestBind(TestCase):

    def test_udp_bind(self):
        handle = pyuv.UDP(self.loop)
        handle.bind(("", TEST_PORT))
        handle.close()
        self.loop.run()

    def test_udp_bind_reuse(self):
        handle = pyuv.UDP(self.loop)
        handle.bind(("", TEST_PORT), pyuv.UV_UDP_REUSEADDR)
        handle2 = pyuv.UDP(self.loop)
        handle2.bind(("", TEST_PORT), pyuv.UV_UDP_REUSEADDR)
        handle.close()
        handle2.close()
        self.loop.run()

    def test_udp_bind_noreuse(self):
        handle = pyuv.UDP(self.loop)
        handle.bind(("", TEST_PORT))
        handle2 = pyuv.UDP(self.loop)
        self.assertRaises(pyuv.error.UDPError, handle2.bind, ("", TEST_PORT))
        handle.close()
        handle2.close()
        self.loop.run()


class UDPTestFileno(TestCase):

    def check_fileno(self, handle):
        fd = handle.fileno()
        self.assertIsInstance(fd, int)
        self.assertGreaterEqual(fd, 0)

    def test_udp_fileno(self):
        server = pyuv.UDP(self.loop)
        server.bind(("0.0.0.0", TEST_PORT))
        self.check_fileno(server)
        server.close()


class UDPTestMulticastInterface(TestCase):

    def test_udp_multicast_interface(self):
        handle = pyuv.UDP(self.loop)
        handle.bind(("", TEST_PORT))
        handle.set_multicast_interface("0.0.0.0")
        handle.close()
        self.loop.run()


@platform_skip(["win32"])
class UDPTryTest(TestCase):

    def setUp(self):
        super(UDPTryTest, self).setUp()
        self.server = None
        self.client = None

    def on_close(self, handle):
        self.on_close_called += 1

    def on_server_recv(self, handle, ip_port, flags, data, error):
        self.assertEqual(flags, 0)
        ip, port = ip_port
        data = data.strip()
        self.assertEqual(data, b"PING")
        self.server.close(self.on_close)
        self.client.close(self.on_close)

    def timer_cb(self, timer):
        timer.close(self.on_close)
        while True:
            try:
                r = self.client.try_send(("127.0.0.1", TEST_PORT), b"PING")
            except pyuv.error.UDPError as e:
                self.assertEqual(e.args[0], pyuv.errno.UV_EAGAIN)
            else:
                self.assertEqual(r, 4)
                break

    def test_udp_try_send(self):
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
