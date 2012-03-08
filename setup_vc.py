# coding=utf8

import os
import shutil
import subprocess
import sys

from distutils import log
from distutils.command.build_ext import build_ext
from distutils.core import setup, Extension
from distutils.errors import DistutilsError

__version__ = "0.5.0"

thisfolder = os.getcwd()
libfolder = thisfolder + "\\deps\\libuv\\Release\\lib"
includefolder= thisfolder + "\\deps\\libuv\\include"
careasfolder = thisfolder + r"\deps\libuv\src\ares"
libuv_bat = thisfolder + "\\deps\\libuv\\vcbuild.bat release"

if len(sys.argv)>1 and sys.argv[1] == "build":
    if not os.path.isfile(libfolder+"\\uv.lib"):
        os.system(libuv_bat)
    
if len(sys.argv)>1 and sys.argv[1] == "clean":
    os.system(libuv_clean_bat)

setup(name         = "pyuv",
      version      = __version__,
      author       = "Sa¨²l Ibarra Corretg¨¦",
      author_email = "saghul@gmail.com",
      url          = "http://github.com/saghul/pyuv",
      description  = "Python interface for libuv",
      platforms    = ["POSIX","Windows"],
      classifiers  = [
          "Development Status :: 3 - Alpha",
          "Intended Audience :: Developers",
          "License :: OSI Approved :: MIT License",
          "Operating System :: POSIX,Windows",
          "Programming Language :: C"
      ],
      ext_modules  = [Extension('pyuv', 
                                sources = ['src/pyuv.c'], 
                                define_macros=[('MODULE_VERSION', __version__), ('LIBUV_REVISION', 6)],
                                extra_compile_args = ["/DMT", "/DWINDOWS=1","/DMODULE_VERSION="+__version__,"/DLIBUV_REVISION=6","/TP"],
                                extra_link_args = ["/MANIFEST"],
                                libraries = ["uv","ws2_32","kernel32","user32","advapi32","Psapi","Iphlpapi"],
                                library_dirs = [libfolder],
                                include_dirs = [includefolder, careasfolder]
                                )],
     )
