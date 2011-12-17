
import common
import unittest

import pyuv


class UtilTest(common.UVTestCase):

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

    #def test_process_title(self):
    #    title = 'my process'
    #    r = pyuv.util.get_process_title()
    #    self.assertTrue(r)
    #    r = pyuv.util.set_process_title(title)
    #    self.assertEquals(r, None)
    #    r = pyuv.util.get_process_title()
    #    self.assertEqual(r, title)

    def test_resident_set_memory(self):
        r = pyuv.util.resident_set_memory()
        self.assertTrue(r)

    def test_interface_addresses(self):
        r = pyuv.util.interface_addresses()
        self.assertTrue(r)

    def test_cpu_info(self):
        r = pyuv.util.cpu_info()
        self.assertTrue(r)


if __name__ == '__main__':
    unittest.main()

