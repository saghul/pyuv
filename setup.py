
import os
import shutil
import subprocess
import sys

from distutils import log
from distutils.command.build_ext import build_ext
from distutils.core import setup, Extension
from distutils.errors import DistutilsError


__version__ = "0.0.0"

class pyuv_build_ext(build_ext):
    libuv_repo = 'https://github.com/joyent/libuv.git'
    libuv_revision = '8f617b93bcb3e4b54fd4fa33883b14bad014dfc0'
    libuv_patches = ['patches/use_lib_prefix.patch']

    @staticmethod
    def exec_process(cmdline, silent=True, input=None, **kwargs):
        """Execute a subprocess and returns the returncode, stdout buffer and stderr buffer.
        Optionally prints stdout and stderr while running."""
        try:
            sub = subprocess.Popen(cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
            stdout, stderr = sub.communicate(input=input)
            returncode = sub.returncode
            if not silent:
                sys.stdout.write(stdout)
                sys.stderr.write(stderr)
        except OSError,e:
            if e.errno == 2:
                raise DistutilsError('"%s" is not present on this system' % cmdline[0])
            else:
                raise
        if returncode != 0:
            raise DistutilsError('Got return value %d while executing "%s", stderr output was:\n%s' % (returncode, " ".join(cmdline), stderr.rstrip("\n")))
        return stdout

    def finalize_options(self):
        build_ext.finalize_options(self)
        self.get_libuv()
        self.include_dirs.append(os.path.join(self.libuv_dir, 'include'))
        self.library_dirs.append(self.libuv_dir)
        self.libraries.append(os.path.join(self.libuv_dir, 'uv'))
        self.libraries.append('rt')

    def get_libuv(self):
        self.libuv_dir = os.path.join(self.build_temp, 'libuv')
        #self.debug_mode =  bool(self.debug) or hasattr(sys, 'gettotalrefcount')
        def download_libuv():
            log.info('Downloading libuv...')
            self.exec_process(['git', 'clone', self.libuv_repo, self.libuv_dir])
            self.exec_process(['git', 'checkout', self.libuv_revision], cwd=self.libuv_dir)
        def patch_libuv():
            log.info('Patching libuv...')
            for patch_file in self.libuv_patches:
                self.exec_process(['patch', '--forward', '-d', self.libuv_dir, '-p0', '-i', os.path.abspath(patch_file)])
        def build_libuv():
            cflags = '-fPIC'
            env = os.environ.copy()
            env['CFLAGS'] = ' '.join(x for x in (cflags, env.get('CFLAGS', None)) if x)
            log.info('Building libuv...')
            self.exec_process(['make', 'libuv.a'], cwd=self.libuv_dir, env=env)
        def update_libuv():
            shutil.rmtree(self.libuv_dir)
            download_libuv()
            patch_libuv()
            build_libuv()
        if not os.path.exists(self.libuv_dir):
            download_libuv()
            patch_libuv()
            build_libuv()
        else:
            if not os.path.exists(os.path.join(self.libuv_dir, 'libuv.a')):
                log.info('libuv needs to be compiled.')
                update_libuv()
            else:
                rev = self.exec_process(['git', 'rev-list', '--max-count=1', 'HEAD'], cwd=self.libuv_dir)
                if rev.strip() == self.libuv_revision:
                    log.info('No need to download libuv.')
                else:
                    # TODO: implement a command line switch to force rebuild
                    log.info('libuv needs to be updated.')
                    update_libuv()


setup(name = 'pyuv',
      version = __version__,
      description = 'Python bindings for libuv',
      cmdclass={'build_ext': pyuv_build_ext},
      ext_modules = [Extension('pyuv', sources = ['src/pyuv.c'], define_macros=[('MODULE_VERSION', __version__)])]
     )

