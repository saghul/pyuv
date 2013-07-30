
import os
import sys
sys.path.insert(0, '../')
import pyuv


if sys.version_info < (2, 7) or (0x03000000 <= sys.hexversion < 0x03010000):
    # py26 or py30
    import unittest2
else:
    import unittest as unittest2


if sys.version_info >= (3,):
    linesep = os.linesep.encode()

    def reraise(typ, value, tb):
        if value.__traceback__ is not tb:
            raise value.with_traceback(tb)
        raise value

else:
    linesep = os.linesep

    exec("""\
def reraise(typ, value, tb):
    raise typ, value, tb
""")


platform = 'linux' if sys.platform.startswith('linux') else sys.platform

# decorator for class
def platform_skip(platform_list):
    def _noop(obj):
        return obj
    if platform in platform_list:
        return unittest2.skip("Test disabled in the current platform")
    return _noop


class TestLoop(pyuv.Loop):

    def __init__(self):
        super(TestLoop, self).__init__()
        self.excepthook = self._handle_exception_in_callback

    def run(self, *args, **kwargs):
        self._callback_exc_info = None
        super(TestLoop, self).run(*args, **kwargs)
        self._reraise()

    def walk(self, *args, **kwargs):
        self._callback_exc_info = None
        super(TestLoop, self).walk(*args, **kwargs)
        self._reraise()

    def _handle_exception_in_callback(self, typ, value, tb):
        if self._callback_exc_info is None:
            self._callback_exc_info = typ, value, tb
            self.stop()

    def _reraise(self):
        if self._callback_exc_info is not None:
            typ, value, tb = self._callback_exc_info
            self._callback_exc_info = None
            reraise(typ, value, tb)


class TestCase(unittest2.TestCase):

    def setUp(self):
        self.loop = TestLoop()
