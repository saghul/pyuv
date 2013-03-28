
import weakref

from common import unittest2
import pyuv


class WeakrefTest(unittest2.TestCase):
    handle_types = ('Check', 'Idle', 'Pipe', 'Prepare', 'TCP', 'Timer', 'UDP')

    def test_weakref(self):
        loop = pyuv.Loop()
        refs = []
        for type in self.handle_types:
            klass = getattr(pyuv, type)
            obj = klass(loop)
            refs.append(weakref.ref(obj))
            del obj
        for ref in refs:
            self.assertNotEqual(ref(), None)
        loop.run()
        for ref in refs:
            self.assertEqual(ref(), None)


if __name__ == '__main__':
    unittest2.main(verbosity=2)

