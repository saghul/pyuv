
from common import unittest2

import pyuv
import socket


class UtilTest(unittest2.TestCase):

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
        loop = pyuv.Loop.default_loop()
        pyuv.util.getaddrinfo(loop, getaddrinfo_cb, 'localhost', 80, socket.AF_INET)
        loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

