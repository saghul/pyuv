
import gc
import pyuv
import weakref
from common import unittest2


class Foo(object): pass

class GCTest(unittest2.TestCase):

    def test_gc(self):
        loop = pyuv.Loop.default_loop()
        timer = pyuv.Timer(loop)

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
    unittest2.main(verbosity=2)

