
import gc
import unittest
import weakref

from common import TestCase
import pyuv


class Foo(object): pass

class GCTest(TestCase):

    def test_gc(self):
        timer = pyuv.Timer(self.loop)

        w_timer = weakref.ref(timer)
        self.assertNotEqual(w_timer(), None)

        foo = Foo()
        foo.timer = timer
        timer.foo = foo

        timer = None
        foo = None

        gc.collect()
        self.assertEqual(w_timer(), None)


if __name__ == '__main__':
    unittest.main(verbosity=2)
