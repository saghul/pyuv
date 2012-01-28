
import errno
import os
import shutil
import subprocess
import sys

from distutils import log
from distutils.command.build_ext import build_ext
from distutils.command.sdist import sdist
from distutils.errors import DistutilsError


def makedirs(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno==errno.EEXIST and os.path.isdir(path) and os.access(path, os.R_OK | os.W_OK | os.X_OK):
            return
        raise

def rmtree(path):
    try:
        shutil.rmtree(path)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

def exec_process(cmdline, silent=True, input=None, **kwargs):
    """Execute a subprocess and returns the returncode, stdout buffer and stderr buffer.
    Optionally prints stdout and stderr while running."""
    try:
        sub = subprocess.Popen(args=cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
        stdout, stderr = sub.communicate(input=input)
        returncode = sub.returncode
        if not silent:
            sys.stdout.write(stdout)
            sys.stderr.write(stderr)
    except OSError as e:
        if e.errno == 2:
            raise DistutilsError('"%s" is not present on this system' % cmdline[0])
        else:
            raise
    if returncode != 0:
        raise DistutilsError('Got return value %d while executing "%s", stderr output was:\n%s' % (returncode, " ".join(cmdline), stderr.rstrip("\n")))
    return stdout


class libuv_build_ext(build_ext):
    libuv_dir      = os.path.join('deps', 'libuv')
    libuv_repo     = 'https://github.com/joyent/libuv.git'
    libuv_branch   = 'master'
    libuv_revision = '3de0411'
    libuv_patches  = []

    user_options = build_ext.user_options
    user_options.extend([
        ("libuv-clean-compile", None, "Clean libuv tree before compilation"),
        ("libuv-force-fetch", None, "Remove libuv (if present) and fetch it again")
    ])
    boolean_options = build_ext.boolean_options
    boolean_options.extend(["libuv-clean-compile", "libuv-force-fetch"])

    def initialize_options(self):
        build_ext.initialize_options(self)
        self.libuv_clean_compile = 0
        self.libuv_force_fetch = 0

    def build_extensions(self):
        if self.libuv_force_fetch or self.libuv_clean_compile:
            self.force = 1
        self.get_libuv()
        build_ext.build_extensions(self)

    def finalize_options(self):
        build_ext.finalize_options(self)
        self.include_dirs.append(os.path.join(self.libuv_dir, 'include'))
        self.include_dirs.append(os.path.join(self.libuv_dir, 'src', 'ares'))
        self.library_dirs.append(self.libuv_dir)
        self.libraries.append('uv')
        if sys.platform.startswith('linux'):
            self.libraries.append('rt')
        elif sys.platform == 'darwin':
            self.extensions[0].extra_link_args = ['-framework', 'CoreServices']
        elif sys.platform == 'win32':
            self.libraries.append('iphlpapi')
            self.libraries.append('psapi')
            self.libraries.append('ws2_32')

    def get_libuv(self):
        #self.debug_mode =  bool(self.debug) or hasattr(sys, 'gettotalrefcount')
        def download_libuv():
            log.info('Downloading libuv...')
            makedirs(self.libuv_dir)
            exec_process(['git', 'clone', '--depth', '1', '-b', self.libuv_branch, self.libuv_repo, self.libuv_dir])
            exec_process(['git', 'checkout', self.libuv_revision], cwd=self.libuv_dir)
        def patch_libuv():
            log.info('Patching libuv...')
            for patch_file in self.libuv_patches:
                exec_process(['patch', '--forward', '-d', self.libuv_dir, '-p0', '-i', os.path.abspath(patch_file)])
        def build_libuv():
            cflags = '-fPIC'
            env = os.environ.copy()
            env['CFLAGS'] = ' '.join(x for x in (cflags, env.get('CFLAGS', None)) if x)
            log.info('Building libuv...')
            exec_process(['make', 'uv.a'], cwd=self.libuv_dir, env=env)
            shutil.move(os.path.join(self.libuv_dir, 'uv.a'), os.path.join(self.libuv_dir, 'libuv.a'))
        if self.libuv_force_fetch:
            rmtree('deps')
        if not os.path.exists(self.libuv_dir):
            download_libuv()
            patch_libuv()
            build_libuv()
        else:
            if self.libuv_clean_compile:
                exec_process(['make', 'clean'], cwd=self.libuv_dir)
            if not os.path.exists(os.path.join(self.libuv_dir, 'libuv.a')):
                log.info('libuv needs to be compiled.')
                build_libuv()
            else:
                log.info('No need to build libuv.')


class libuv_sdist(sdist):
    libuv_dir      = os.path.join('deps', 'libuv')
    libuv_repo     = libuv_build_ext.libuv_repo
    libuv_branch   = libuv_build_ext.libuv_branch
    libuv_revision = libuv_build_ext.libuv_revision
    libuv_patches  = libuv_build_ext.libuv_patches

    def initialize_options(self):
        sdist.initialize_options(self)
        rmtree('deps')
        makedirs(self.libuv_dir)
        log.info('Downloading libuv...')
        exec_process(['git', 'clone', '-b', self.libuv_branch, self.libuv_repo, self.libuv_dir])
        exec_process(['git', 'checkout', self.libuv_revision], cwd=self.libuv_dir)
        log.info('Patching libuv...')
        for patch_file in self.libuv_patches:
            exec_process(['patch', '--forward', '-d', self.libuv_dir, '-p0', '-i', os.path.abspath(patch_file)])
        rmtree(os.path.join(self.libuv_dir, '.git'))

