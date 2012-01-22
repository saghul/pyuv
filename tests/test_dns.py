
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

    def query_a_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_a(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_a('google.com', self.query_a_cb)
        loop.run()

    def query_aaaa_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_aaaa(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_aaaa('ipv6.google.com', self.query_aaaa_cb)
        loop.run()

    def query_cname_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_cname(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_cname('www.google.com', self.query_cname_cb)
        loop.run()

    def query_mx_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_mx(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_mx('google.com', self.query_mx_cb)
        loop.run()

    def query_ns_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_ns(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_ns('google.com', self.query_ns_cb)
        loop.run()

    def query_txt_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_txt(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_txt('google.com', self.query_txt_cb)
        loop.run()

    def query_srv_cb(self, resolver, status, result):
        self.assertEqual(status, 0)
        self.assertTrue(result)

    def test_query_srv(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_srv('_xmpp-server._tcp.google.com', self.query_srv_cb)
        loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

