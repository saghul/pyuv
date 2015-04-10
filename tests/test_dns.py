
import socket
import unittest

from common import TestCase
import pyuv


class DnsTest(TestCase):

    def test_getaddrinfo(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.dns.getaddrinfo(self.loop, 'localhost', 80, socket.AF_INET, callback=getaddrinfo_cb)
        self.loop.run()

    def test_getaddrinfo_sync(self):
        res = pyuv.dns.getaddrinfo(self.loop, 'localhost', 80, socket.AF_INET)
        self.loop.run()
        self.assertNotEqual(res, None)

    def test_getaddrinfo_sync_fail(self):
        self.assertRaises(pyuv.error.UVError, pyuv.dns.getaddrinfo, self.loop, 'lala.lala.lala', 80, socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_none(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 0)
        pyuv.dns.getaddrinfo(self.loop, 'localhost', None, socket.AF_INET, callback=getaddrinfo_cb)
        self.loop.run()

    def test_getaddrinfo_service(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.dns.getaddrinfo(self.loop, 'localhost', 'http', socket.AF_INET, callback=getaddrinfo_cb)
        self.loop.run()

    def test_getaddrinfo_service_bytes(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.dns.getaddrinfo(self.loop, b'localhost', b'http', socket.AF_INET, callback=getaddrinfo_cb)
        self.loop.run()

    def test_getnameinfo_ipv4(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.dns.getnameinfo(self.loop, ('127.0.0.1', 80), callback=cb)
        self.loop.run()

    def test_getnameinfo_ipv6(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
            pyuv.dns.getnameinfo(self.loop, ('::1', 80), callback=cb)
        self.loop.run()

    def test_getnameinfo_sync(self):
        result = pyuv.dns.getnameinfo(self.loop, ('127.0.0.1', 80))
        self.loop.run()
        self.assertNotEqual(result, None)


if __name__ == '__main__':
    unittest.main(verbosity=2)
