
import socket

from common import unittest2
import pyuv


class DNSTest(unittest2.TestCase):

    def getaddrinfo_cb(self, resolver, status, result):
        self.assertEqual(status, 0)

    def test_getaddrinfo(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.getaddrinfo(self.getaddrinfo_cb, 'localhost', 80, socket.AF_INET)
        loop.run()

    def gethostbyaddr_cb(self, resolver, status, timeouts, name, aliases, result):
        self.assertEqual(status, 0)

    def test_gethostbyaddr(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.gethostbyaddr(self.gethostbyaddr_cb, '127.0.0.1')
        loop.run()

    def gethostbyname_cb(self, resolver, status, timeouts, name, aliases, result):
        self.assertEqual(status, 0)

    def test_gethostbyname(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.gethostbyname(self.gethostbyname_cb, 'localhost', socket.AF_INET)
        loop.run()

    def getnameinfo_cb(self, resolver, status, timeouts, node, service):
        self.assertEqual(status, 0)

    def test_getnameinfo(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.getnameinfo(self.getnameinfo_cb, '127.0.0.1', 0, pyuv.dns.ARES_NI_LOOKUPHOST)
        loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

