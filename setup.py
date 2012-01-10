# coding=utf8

from distutils.core import setup, Extension
from setup_libuv import libuv_build_ext, libuv_sdist


__version__ = "0.3.0"

setup(name         = "pyuv",
      version      = __version__,
      author       = "Saúl Ibarra Corretgé",
      author_email = "saghul@gmail.com",
      url          = "http://github.com/saghul/pyuv",
      description  = "Python interface for libuv",
      platforms    = ["POSIX"],
      classifiers  = [
          "Development Status :: 4 - Beta",
          "Intended Audience :: Developers",
          "License :: OSI Approved :: MIT License",
          "Operating System :: POSIX",
          "Programming Language :: C"
      ],
      cmdclass     = {'build_ext': libuv_build_ext,
                      'sdist'    : libuv_sdist},
      ext_modules  = [Extension('pyuv',
                                sources = ['src/pyuv.c'],
                                define_macros=[('MODULE_VERSION', __version__),
                                               ('LIBUV_REVISION', libuv_build_ext.libuv_revision)]
                     )]
     )

