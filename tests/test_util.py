
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

    def test_exepath(self):
        r = pyuv.util.exepath()
        self.assertTrue(r)


if __name__ == '__main__':
    unittest.main()

