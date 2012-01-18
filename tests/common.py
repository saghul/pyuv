
import os
import sys


if sys.version_info < (2, 7) or (0x03000000 <= sys.hexversion < 0x03010000):
    # py26 or py30
    import unittest2
else:
    import unittest as unittest2

if sys.version_info >= (3, 0):
    linesep = os.linesep.encode()
else:
    linesep = os.linesep


platform = 'linux' if sys.platform.startswith('linux') else sys.platform


# decorator for class
def platform_skip(platform_list):
    def _platform_skip(c):
        is_skip = (platform in platform_list)
        for m in c.__dict__:
            if m.startswith("test_"):
                setattr(c, m, unittest2.skipIf(is_skip, 
                    "Test disabled in the current platform")(getattr(c, m)))
        return c
    return _platform_skip
