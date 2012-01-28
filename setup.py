# coding=utf8

from distutils.core import setup, Extension
from setup_libuv import libuv_build_ext, libuv_sdist


__version__ = "0.5.0"

setup(name             = "pyuv",
      version          = __version__,
      author           = "Saúl Ibarra Corretgé",
      author_email     = "saghul@gmail.com",
      url              = "http://github.com/saghul/pyuv",
      description      = "Python interface for libuv",
      long_description = open("README").read(),
      platforms        = ["POSIX", "Microsoft Windows"],
      classifiers      = [
          "Development Status :: 4 - Beta",
          "Intended Audience :: Developers",
          "License :: OSI Approved :: MIT License",
          "Operating System :: POSIX",
          "Operating System :: Microsoft :: Windows",
          "Programming Language :: Python",
          "Programming Language :: Python :: 2.6",
          "Programming Language :: Python :: 2.7",
          "Programming Language :: Python :: 3",
          "Programming Language :: Python :: 3.0",
          "Programming Language :: Python :: 3.1",
          "Programming Language :: Python :: 3.2"
      ],
      cmdclass     = {'build_ext': libuv_build_ext,
                      'sdist'    : libuv_sdist},
      ext_modules  = [Extension('pyuv',
                                sources = ['src/pyuv.c'],
                                define_macros=[('MODULE_VERSION', __version__),
                                               ('LIBUV_REVISION', libuv_build_ext.libuv_revision)]
                     )]
     )

