
import unittest
import weakref

from common import TestCase
import pyuv


class WeakrefTest(TestCase):
    handle_types = ('Check', 'Idle', 'Pipe', 'Prepare', 'TCP', 'Timer', 'UDP')

    def test_weakref(self):
        refs = []
        for type in self.handle_types:
            klass = getattr(pyuv, type)
            obj = klass(self.loop)
            refs.append(weakref.ref(obj))
            del obj
        for ref in refs:
            self.assertNotEqual(ref(), None)
        self.loop.run()
        for ref in refs:
            self.assertEqual(ref(), None)


if __name__ == '__main__':
    unittest.main(verbosity=2)
