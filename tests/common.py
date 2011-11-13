
import os
import sys
sys.path.insert(0, '../')
import unittest


loader = unittest.TestLoader()
suites = []

class TestCaseMeta(type):
    def __init__(cls, name, bases, dic):
        super(TestCaseMeta, cls).__init__(name, bases, dic)
        suites.append(loader.loadTestsFromTestCase(cls))

class UVTestCase(unittest.TestCase):
    __metaclass__ = TestCaseMeta


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


