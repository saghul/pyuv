
import io
import os
import sys
import unittest

if 'PYUV_INSIDE_TOX' not in os.environ:
    sys.path.insert(0, '../')

import pyuv


if sys.version_info >= (3,):
    linesep = os.linesep.encode()
    StdBufferIO = io.StringIO

    def reraise(typ, value, tb):
        if value.__traceback__ is not tb:
            raise value.with_traceback(tb)
        raise value
else:
    linesep = os.linesep
    StdBufferIO = io.BytesIO

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
        return unittest.skip("Test disabled in the current platform")
    return _noop


class TestLoop(pyuv.Loop):

    def __init__(self):
        super(TestLoop, self).__init__()
        self.excepthook = self._handle_exception_in_callback

    def run(self, *args, **kwargs):
        self._callback_exc_info = None
        super(TestLoop, self).run(*args, **kwargs)
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


class TestCase(unittest.TestCase):

    def setUp(self):
        self.loop = TestLoop()

