
import ast
import errno
import os
import platform
import shutil
import stat
import subprocess
import sys

from distutils import log
from distutils.command.build_ext import build_ext
from distutils.command.sdist import sdist
from distutils.errors import DistutilsError


PY3 = sys.version_info[0] == 3


def makedirs(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno==errno.EEXIST and os.path.isdir(path) and os.access(path, os.R_OK | os.W_OK | os.X_OK):
            return
        raise


def rmtree(path):
    def remove_readonly(func, path, excinfo):
        os.chmod(path, stat.S_IWRITE)
        func(path)

    try:
        shutil.rmtree(path, onerror=remove_readonly)
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
        if PY3:
            stderr = stderr.decode('utf-8')
            stdout = stdout.decode('utf-8')
        if not silent:
            sys.stdout.write(stdout)
            sys.stderr.write(stderr)
    except OSError as e:
        if e.errno == errno.ENOENT:
            raise DistutilsError('"%s" is not present on this system' % cmdline[0])
        else:
            raise
    if returncode != 0:
        output = 'stderr:\n%s\nstdout:\n%s' % (stderr.rstrip("\n"), stdout.rstrip("\n"))
        raise DistutilsError('Got return value %d while executing "%s", output was:\n%s' % (returncode, " ".join(cmdline), output))
    return stdout


def prepare_windows_env(env):
    env.pop('VS140COMNTOOLS', None)
    env.pop('VS120COMNTOOLS', None)
    env.pop('VS110COMNTOOLS', None)
    if sys.version_info < (3, 3):
        env.pop('VS100COMNTOOLS', None)
        env['GYP_MSVS_VERSION'] = '2008'
    else:
        env['GYP_MSVS_VERSION'] = '2010'

    if env.get('PYTHON'):
        return  # Already manually set by user.

    if sys.version_info[:2] == (2, 7):
        env['PYTHON'] = sys.executable
        return  # The current executable is fine.

    # Try if `python` on PATH is the right one. If we would execute
    # `python` directly the current executable might be used so we
    # delegate this to cmd.
    cmd = ['cmd.exe', '/C',  'python', '-c', 'import sys; '
           'v = str(sys.version_info[:2]); sys.stdout.write(v); '
           'sys.stdout.flush()']
    try:
        sub = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, _ = sub.communicate()
        version = ast.literal_eval(stdout.decode('utf-8').strip())
        if version == (2, 7):
            return  # Python on PATH is fine
    except OSError:
        pass

    # Check default install locations
    path = os.path.join('%SYSTEMDRIVE%', 'Python27', 'python.exe')
    path = os.path.expandvars(path)
    if os.path.isfile(path):
        log.info('Using "%s" to build libuv...' % path)
        env['PYTHON'] = path
    else:
        raise DistutilsError('No appropriate Python version found. An '
                             'installation of 2.7 is required to '
                             'build libuv. You can set the environment '
                             'variable "PYTHON" to point to a custom '
                             'installation location.')


class libuv_build_ext(build_ext):
    libuv_dir      = os.path.join('deps', 'libuv')
    libuv_repo     = 'https://github.com/libuv/libuv.git'
    libuv_branch   = 'v1.x'
    libuv_revision = '30c8be0'
    libuv_patches  = []

    user_options = build_ext.user_options
    user_options.extend([
        ("libuv-clean-compile", None, "Clean libuv tree before compilation"),
        ("libuv-force-fetch", None, "Remove libuv (if present) and fetch it again"),
        ("libuv-verbose-build", None, "Print output of libuv build process"),
        ("use-system-libuv", None, "Use the system provided libuv, instead of the bundled one")
    ])
    boolean_options = build_ext.boolean_options
    boolean_options.extend(["libuv-clean-compile", "libuv-force-fetch", "libuv-verbose-build", "use-system-libuv"])

    def initialize_options(self):
        build_ext.initialize_options(self)
        self.libuv_clean_compile = 0
        self.libuv_force_fetch = 0
        self.libuv_verbose_build = 0
        self.use_system_libuv = 0

    def build_extensions(self):
        self.force = self.force or self.libuv_force_fetch or self.libuv_clean_compile
        if self.use_system_libuv:
            if sys.platform == 'win32':
                raise DistutilsError('using a system provided libuv is unsupported on Windows')
            self.compiler.add_library('uv')
        else:
            if sys.platform == 'win32':
                self.libuv_lib = os.path.join(self.libuv_dir, 'Release', 'lib', 'libuv.lib')
            else:
                self.libuv_lib = os.path.join(self.libuv_dir, '.libs', 'libuv.a')
            self.get_libuv()
            # Set compiler options
            self.extensions[0].extra_objects.extend([self.libuv_lib])
            self.compiler.add_include_dir(os.path.join(self.libuv_dir, 'include'))
        if sys.platform.startswith('linux'):
            self.compiler.add_library('rt')
        elif sys.platform == 'win32':
            self.extensions[0].define_macros.append(('WIN32', 1))
            self.extensions[0].extra_link_args.extend(['/NODEFAULTLIB:libcmt', '/LTCG'])
            self.compiler.add_library('advapi32')
            self.compiler.add_library('iphlpapi')
            self.compiler.add_library('psapi')
            self.compiler.add_library('ws2_32')
        elif sys.platform.startswith('freebsd'):
            self.compiler.add_library('kvm')
        build_ext.build_extensions(self)

    def get_libuv(self):
        #self.debug_mode =  bool(self.debug) or hasattr(sys, 'gettotalrefcount')
        def download_libuv():
            log.info('Downloading libuv...')
            makedirs(self.libuv_dir)
            exec_process(['git', 'clone', '-b', self.libuv_branch, self.libuv_repo, self.libuv_dir])
            exec_process(['git', 'reset', '--hard', self.libuv_revision], cwd=self.libuv_dir)
        def patch_libuv():
            if self.libuv_patches:
                log.info('Patching libuv...')
                for patch_file in self.libuv_patches:
                    exec_process(['patch', '--forward', '-d', self.libuv_dir, '-p0', '-i', os.path.abspath(patch_file)])
        def build_libuv():
            cflags = '-fPIC'
            env = os.environ.copy()
            env['CFLAGS'] = ' '.join(x for x in (cflags, env.get('CFLAGS', None), env.get('ARCHFLAGS', None)) if x)
            log.info('Building libuv...')
            if sys.platform == 'win32':
                prepare_windows_env(env)
                libuv_arch = {'32bit': 'x86', '64bit': 'x64'}[platform.architecture()[0]]
                exec_process(['cmd.exe', '/C', 'vcbuild.bat', libuv_arch, 'release'], cwd=self.libuv_dir, env=env, shell=True, silent=not self.libuv_verbose_build)
            else:
                exec_process(['sh', 'autogen.sh'], cwd=self.libuv_dir, env=env, silent=not self.libuv_verbose_build)
                exec_process(['./configure'], cwd=self.libuv_dir, env=env, silent=not self.libuv_verbose_build)
                exec_process(['make'], cwd=self.libuv_dir, env=env, silent=not self.libuv_verbose_build)
        if self.libuv_force_fetch:
            rmtree('deps')
        if not os.path.exists(self.libuv_dir):
            try:
                download_libuv()
            except BaseException:
                rmtree('deps')
                raise
            patch_libuv()
            build_libuv()
        else:
            if self.libuv_clean_compile:
                if sys.platform == 'win32':
                    env = os.environ.copy()
                    prepare_windows_env(env)
                    exec_process(['cmd.exe', '/C', 'vcbuild.bat', 'clean'], cwd=self.libuv_dir, env=env, shell=True)
                    rmtree(os.path.join(self.libuv_dir, 'Release'))
                else:
                    exec_process(['make', 'clean'], cwd=self.libuv_dir)
                    exec_process(['make', 'distclean'], cwd=self.libuv_dir)
            if not os.path.exists(self.libuv_lib):
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

    gyp_dir = os.path.join(libuv_dir, 'build', 'gyp')
    gyp_repo = 'https://chromium.googlesource.com/external/gyp.git'

    def initialize_options(self):
        sdist.initialize_options(self)
        rmtree('deps')
        makedirs(self.libuv_dir)
        log.info('Downloading libuv...')
        exec_process(['git', 'clone', '-b', self.libuv_branch, self.libuv_repo, self.libuv_dir])
        exec_process(['git', 'checkout', self.libuv_revision], cwd=self.libuv_dir)
        if self.libuv_patches:
            log.info('Patching libuv...')
            for patch_file in self.libuv_patches:
                exec_process(['patch', '--forward', '-d', self.libuv_dir, '-p0', '-i', os.path.abspath(patch_file)])
        rmtree(os.path.join(self.libuv_dir, '.git'))

        log.info('Downloading gyp...')
        exec_process(['git', 'clone', self.gyp_repo, self.gyp_dir])
        rmtree(os.path.join(self.gyp_dir, 'test'))
        rmtree(os.path.join(self.gyp_dir, '.git'))

