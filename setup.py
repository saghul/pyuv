# coding=utf-8

import codecs

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
from setup_libuv import libuv_build_ext, libuv_sdist


__version__ = "1.0.1"

setup(name             = "pyuv",
      version          = __version__,
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
      ext_modules  = [Extension('pyuv',
                                sources = ['src/pyuv.c'],
                                define_macros=[('MODULE_VERSION', __version__),
                                               ('LIBUV_REVISION', libuv_build_ext.libuv_revision)]
                     )]
     )

