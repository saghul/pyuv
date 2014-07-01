
from common import unittest2, TestCase

import pyuv
import socket


class UtilTest(TestCase):

    def test_hrtime(self):
        r = pyuv.util.hrtime()
        self.assertTrue(r)

    def test_freemem(self):
        r = pyuv.util.get_free_memory()
        self.assertTrue(r)

    def test_totalmem(self):
        r = pyuv.util.get_total_memory()
        self.assertTrue(r)

    def test_loadavg(self):
        r = pyuv.util.loadavg()
        self.assertTrue(r)

    def test_uptime(self):
        r = pyuv.util.uptime()
        self.assertTrue(r)

    def test_resident_set_memory(self):
        r = pyuv.util.resident_set_memory()
        self.assertTrue(r)

    def test_interface_addresses(self):
        r = pyuv.util.interface_addresses()
        self.assertTrue(r)

    def test_cpu_info(self):
        r = pyuv.util.cpu_info()
        self.assertTrue(r)

    def test_getaddrinfo(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.util.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', 80, socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_none(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 0)
        pyuv.util.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', None, socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_service(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.util.getaddrinfo(self.loop, getaddrinfo_cb, 'localhost', 'http', socket.AF_INET)
        self.loop.run()

    def test_getaddrinfo_service_bytes(self):
        def getaddrinfo_cb(result, errorno):
            self.assertEqual(errorno, None)
            self.assertEqual(result[0][4][1], 80)
        pyuv.util.getaddrinfo(self.loop, getaddrinfo_cb, b'localhost', b'http', socket.AF_INET)
        self.loop.run()

    def test_getnameinfo_ipv4(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
        pyuv.util.getnameinfo(self.loop, cb, ('127.0.0.1', 80))
        self.loop.run()

    def test_getnameinfo_ipv6(self):
        def cb(result, errorno):
            self.assertEqual(errorno, None)
            pyuv.util.getnameinfo(self.loop, cb, ('::1', 80))
        self.loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)
