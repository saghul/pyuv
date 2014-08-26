
import re
import sys
import unittest

from common import StdBufferIO
import pyuv


class ExcepthookTestCase(unittest.TestCase):
    """Base class for excepthook tests"""

    def assertOutput(self, regexp):
        output = sys.stderr.getvalue()
        match = re.match(regexp, output)
        if match is None:
            self.fail("Output doesn't match %s\n\nOutput:\n-------\n%s"
                      % (unittest.util.safe_repr(regexp), output))

    def setUp(self):
        super(ExcepthookTestCase, self).setUp()
        self.stderr = sys.stderr
        sys.stderr = StdBufferIO()
        self.loop = self.make_loop()
        self.loop.calls = []
        self.loop.exception = None

    def tearDown(self):
        sys.stderr = self.stderr
        super(ExcepthookTestCase, self).tearDown()

    def excepthook_cb(self, typ, value, tb):
        self.loop.calls.append((typ, value, tb))
        if self.loop.exception:
            raise self.loop.exception

    def setup_idle(self, exception=None):
        def idle_cb(handle):
            handle.stop()
            handle.close()
            if exception is not None:
                raise exception
        self.idle = pyuv.Idle(self.loop)
        self.idle.start(idle_cb)


class TestExcepthookDefault(ExcepthookTestCase):
    """Test the default excepthook method"""

    def make_loop(self):
        return pyuv.Loop()

    def test_default_method(self):
        self.setup_idle(RuntimeError("Bad"))
        self.loop.run()
        self.assertOutput("(?s)^Unhandled exception in callback\n"
                          "Traceback.*\nRuntimeError: Bad\n$")
        self.assertNotIn('AttributeError', sys.stderr.getvalue())


class LoopNoExcepthook(pyuv.Loop):

    @property
    def excepthook(self):
        raise AttributeError("object has no attribute 'excepthook'")


class TestExcepthookMissing(TestExcepthookDefault):
    """Test a missing excepthook attribute"""

    def make_loop(self):
        return LoopNoExcepthook()


class TestExcepthookNone(TestExcepthookDefault):
    """Test setting excepthook to None"""

    def make_loop(self):
        loop = pyuv.Loop()
        loop.excepthook = None
        return loop


class LoopBadAttr(pyuv.Loop):

    @property
    def excepthook(self):
        raise TypeError("oops")


class TestExcepthookBadAttr(ExcepthookTestCase):
    """Test exception while retrieving the excepthook attribute"""

    def make_loop(self):
        return LoopBadAttr()

    def test_default_method(self):
        self.setup_idle(RuntimeError("Bad"))
        self.loop.run()
        self.assertOutput("(?s)^Exception while getting excepthook\n"
                          "Traceback.*\nTypeError: oops\n\n"
                          "Unhandled exception in callback\n"
                          "Traceback.*\nRuntimeError: Bad\n$")


class TestExcepthookAttribute(ExcepthookTestCase):
    """Test setting Loop.excepthook as an instance attribute"""

    def make_loop(self):
        loop = pyuv.Loop()
        loop.excepthook = self.excepthook_cb
        return loop

    def test_no_exception(self):
        self.setup_idle()
        self.loop.run()
        self.assertEqual(0, len(self.loop.calls))

    def test_exception(self):
        exception = RuntimeError("Bad")
        self.setup_idle(exception)
        self.loop.run()
        self.assertEqual(1, len(self.loop.calls))
        self.assertEqual((exception.__class__, exception),
                         self.loop.calls[0][:2])

    def test_exception_in_hook(self):
        exception = RuntimeError("Bad")
        self.setup_idle(exception)
        self.loop.exception = NameError("abc")
        self.loop.run()
        self.assertOutput("(?s)^Unhandled exception in excepthook\n"
                          "Traceback.*\nNameError: abc\n\n"
                          "Unhandled exception in callback\n"
                          "Traceback.*\nRuntimeError: Bad\n$")


class LoopOverride(pyuv.Loop):

    def excepthook(self, typ, value, tb):
        self.calls.append((typ, value, tb))
        if self.exception:
            raise self.exception


class TestExcepthookOverride(TestExcepthookAttribute):
    """Test subclassing Loop and overriding excepthook"""

    def make_loop(self):
        return LoopOverride()


class TestExcepthookIgnore(ExcepthookTestCase):

    def make_loop(self):
        return pyuv.Loop()

    def test_ignore_excepthook(self):
        self.setup_idle(RuntimeError("Boom!"))
        self.loop.excepthook = lambda *args: None
        self.loop.run()
        self.assertOutput("(?s)^$")


if __name__ == '__main__':
    unittest.main(verbosity=2)
