
import socket

from common import unittest2
import pyuv


class DNSTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.resolver = pyuv.dns.DNSResolver(self.loop)

    def tearDown(self):
        self.resolver = None
        self.loop = None

    def getaddrinfo_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_getaddrinfo(self):
        self.resolver.getaddrinfo('localhost', self.getaddrinfo_cb, 80, socket.AF_INET)
        self.loop.run()

    def gethostbyaddr_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_gethostbyaddr(self):
        self.resolver.gethostbyaddr('127.0.0.1', self.gethostbyaddr_cb)
        self.loop.run()

    def gethostbyname_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)

    def test_gethostbyname(self):
        self.resolver.gethostbyname('localhost', self.gethostbyname_cb)
        self.loop.run()

    def getnameinfo_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertEqual(result, ('localhost', 'http'))

    def test_getnameinfo(self):
        self.resolver.getnameinfo(('127.0.0.1', 80), pyuv.dns.ARES_NI_LOOKUPHOST|pyuv.dns.ARES_NI_LOOKUPSERVICE, self.getnameinfo_cb)
        self.loop.run()

    def query_a_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_a(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_A, 'google.com', self.query_a_cb)
        self.loop.run()

    def query_a_bad_cb(self, resolver, result, errorno):
        self.assertEqual(result, None)
        self.assertEqual(errorno, pyuv.errno.ARES_ENOTFOUND)

    def test_query_a_bad(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_A, 'hgf8g2od29hdohid.com', self.query_a_bad_cb)
        self.loop.run()

    def query_aaaa_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_aaaa(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_AAAA, 'ipv6.google.com', self.query_aaaa_cb)
        self.loop.run()

    def query_cname_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_cname(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_CNAME, 'www.google.com', self.query_cname_cb)
        self.loop.run()

    def query_mx_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_mx(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_MX, 'google.com', self.query_mx_cb)
        self.loop.run()

    def query_ns_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_ns(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_NS, 'google.com', self.query_ns_cb)
        self.loop.run()

    def query_txt_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_txt(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_TXT, 'google.com', self.query_txt_cb)
        self.loop.run()

    def query_srv_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_srv(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_SRV, '_xmpp-server._tcp.google.com', self.query_srv_cb)
        self.loop.run()

    def query_naptr_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, None)
        self.assertTrue(result)

    def test_query_naptr(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_NAPTR, 'sip2sip.info', self.query_naptr_cb)
        self.loop.run()

    def cancelled_cb(self, resolver, result, errorno):
        self.assertEqual(errorno, pyuv.errno.ARES_ECANCELLED)

    def test_query_cancelled(self):
        self.resolver.query(pyuv.dns.QUERY_TYPE_NS, 'google.com', self.cancelled_cb)
        self.resolver.cancel()
        self.loop.run()

    def test_query_bad_type(self):
        def f(*args):
            pass
        self.assertRaises(pyuv.error.DNSError, self.resolver.query, 667, 'google.com', f)
        self.loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)

