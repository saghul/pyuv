
import socket

from common import unittest2
import pyuv


class DNSTest(unittest2.TestCase):

    def getaddrinfo_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_getaddrinfo(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.getaddrinfo('localhost', self.getaddrinfo_cb, 80, socket.AF_INET)
        loop.run()

    def gethostbyaddr_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_gethostbyaddr(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.gethostbyaddr('127.0.0.1', self.gethostbyaddr_cb)
        loop.run()

    def gethostbyname_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_gethostbyname(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.gethostbyname('localhost', self.gethostbyname_cb)
        loop.run()

    def getnameinfo_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertEqual(result, ('localhost', 'http'))

    def test_getnameinfo(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.getnameinfo(('127.0.0.1', 80), pyuv.dns.ARES_NI_LOOKUPHOST|pyuv.dns.ARES_NI_LOOKUPSERVICE, self.getnameinfo_cb)
        loop.run()

    def query_a_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_a(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_a('google.com', self.query_a_cb)
        loop.run()

    def query_a_bad_cb(self, resolver, result, errorno):
        self.assertEqual(result, None)
        self.assertEqual(errorno, pyuv.errno.ARES_ENOTFOUND)

    def test_query_a_bad(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_a('hgf8g2od29hdohid.com', self.query_a_bad_cb)
        loop.run()

    def query_aaaa_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_aaaa(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_aaaa('ipv6.google.com', self.query_aaaa_cb)
        loop.run()

    def query_cname_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_cname(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_cname('www.google.com', self.query_cname_cb)
        loop.run()

    def query_mx_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_mx(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_mx('google.com', self.query_mx_cb)
        loop.run()

    def query_ns_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_ns(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_ns('google.com', self.query_ns_cb)
        loop.run()

    def query_txt_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_txt(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_txt('google.com', self.query_txt_cb)
        loop.run()

    def query_srv_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_srv(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_srv('_xmpp-server._tcp.google.com', self.query_srv_cb)
        loop.run()

    def query_naptr_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_naptr(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_naptr('sip2sip.info', self.query_naptr_cb)
        loop.run()

    def cancelled_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, pyuv.errno.ARES_ECANCELLED)

    def test_query_cancelled(self):
        loop = pyuv.Loop.default_loop()
        resolver = pyuv.dns.DNSResolver(loop)
        resolver.query_ns('google.com', self.cancelled_cb)
        resolver.cancel()
        loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

