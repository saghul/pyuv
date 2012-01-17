
import os
import sys
sys.path.insert(0, '../')


if sys.version_info < (2, 7):
    import unittest2
else:
    import unittest as unittest2

if sys.version_info >= (3, 0):
    linesep = os.linesep.encode()
else:
    linesep = os.linesep


loader = unittest2.TestLoader()
suites = []

platform = 'linux' if sys.platform.startswith('linux') else sys.platform

is_linux = True if sys.platform.startswith('linux') else False
is_windows = True if sys.platform.startswith('win32') else False


class TestCaseMeta(type):
    def __init__(cls, name, bases, dic):
        super(TestCaseMeta, cls).__init__(name, bases, dic)
        if name == 'UVTestCase':
            return
        if platform not in dic.get('__disabled__', []):
            suites.append(loader.loadTestsFromTestCase(cls))

UVTestCase = TestCaseMeta('UVTestCase', (unittest2.TestCase,), {})


def load_tests():
    loaded_modules = []
    mod_list = [f for f in os.listdir(os.path.dirname(__file__)) if f.startswith('test_')]
    for mod in mod_list:
        try:
            __import__(mod)
        except ImportError:
            pass
        else:
            loaded_modules.append(mod)
    return loaded_modules


