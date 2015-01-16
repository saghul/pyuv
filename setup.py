# coding=utf-8

import codecs
import re

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
from setup_libuv import libuv_build_ext, libuv_sdist


def get_version():
    return re.search(r"""__version__\s+=\s+(?P<quote>['"])(?P<version>.+?)(?P=quote)""", open('pyuv/_version.py').read()).group('version')


setup(name             = "pyuv",
      version          = get_version(),
      author           = "Saúl Ibarra Corretgé",
      author_email     = "saghul@gmail.com",
      url              = "http://github.com/saghul/pyuv",
      description      = "Python interface for libuv",
      long_description = codecs.open("README.rst", encoding="utf-8").read(),
      platforms        = ["POSIX", "Microsoft Windows"],
      classifiers      = [
          "Development Status :: 5 - Production/Stable",
          "Intended Audience :: Developers",
          "License :: OSI Approved :: MIT License",
          "Operating System :: POSIX",
          "Operating System :: Microsoft :: Windows",
          "Programming Language :: Python",
          "Programming Language :: Python :: 2",
          "Programming Language :: Python :: 2.7",
          "Programming Language :: Python :: 3",
          "Programming Language :: Python :: 3.3",
          "Programming Language :: Python :: 3.4"
      ],
      cmdclass     = {'build_ext': libuv_build_ext,
                      'sdist'    : libuv_sdist},
      packages     = ['pyuv'],
      ext_modules  = [Extension('pyuv._cpyuv',
                                sources = ['src/pyuv.c'],
                     )]
     )

