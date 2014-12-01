
import socket
import unittest

from common import TestCase
import pyuv


class DnsTest(TestCase):

    def test_getaddrinfo(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.dns.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', 80, socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_none(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 0)
        pyuv.dns.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', None, socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_service(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.dns.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', 'http', socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_service_bytes(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.dns.getaddrinfo(self.loop, getaddrinfo_cb, b'localhost', b'http', socket.AF_INET)
        self.loop.run()

    def test_getnameinfo_ipv4(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.dns.getnameinfo(self.loop, cb, ('127.0.0.1', 80))
        self.loop.run()

    def test_getnameinfo_ipv6(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
            pyuv.dns.getnameinfo(self.loop, cb, ('::1', 80))
        self.loop.run()


if __name__ == '__main__':
    unittest.main(verbosity=2)
