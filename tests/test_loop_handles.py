
import gc
import unittest
import weakref

from common import TestCase
import pyuv


class HandlesTest(TestCase):
    handle_types = ('Check', 'Idle', 'Pipe', 'Prepare', 'TCP', 'Timer', 'UDP')

    def test_handles(self):
        timer = pyuv.Timer(self.loop)
        self.assertTrue(timer in self.loop.handles)
        timer.close()
        timer = None
        self.loop.run()
        self.assertFalse(self.loop.handles)

    def test_handle_lifetime(self):
        refs = []
        for type in self.handle_types:
            klass = getattr(pyuv, type)
            obj = klass(self.loop)
            refs.append(weakref.ref(obj))
            del obj

        # There are no more references to the handles at this point.
        # Garbage collection should be prevented from freeing them, though.
        # Touching each of these without segfault is a best effort check.
        # The validity of the weakrefs is implementation dependent :(.
        gc.collect()
        handles = self.loop.handles
        self.assertEqual(len(handles), len(self.handle_types))
        for handle in handles:
            self.assertTrue(handle.closed)
            del handle
        del handles

        # Give the loop a chance to finish closing the handles.
        self.loop.run()

        # Ensure the weakref is gone now.
        for ref in refs:
            self.assertEqual(ref(), None)


if __name__ == '__main__':
    unittest.main(verbosity=2)
