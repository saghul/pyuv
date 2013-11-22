
import ast
import errno
import os
import platform
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
        if e.errno == errno.ENOENT:
            raise DistutilsError('"%s" is not present on this system' % cmdline[0])
        else:
            raise
    if returncode != 0:
        raise DistutilsError('Got return value %d while executing "%s", stderr output was:\n%s' % (returncode, " ".join(cmdline), stderr.decode("utf-8").rstrip("\n")))
    return stdout


def prepare_windows_env(env):
    env.pop('VS110COMNTOOLS', None)
    if sys.version_info < (3, 3):
        env.pop('VS100COMNTOOLS', None)

    if env.get('PYTHON'):
        return  # Already manually set by user.

    if (2, 6) <= sys.version_info[:2] <= (2, 7):
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
        if (2, 6) <= version <= (2, 7):
            return  # Python on PATH is fine
    except OSError:
        pass

    # Check default install locations
    for v in ('26', '27'):
        path = os.path.join('%SYSTEMDRIVE%', 'Python%s' % v, 'python.exe')
        path = os.path.expandvars(path)
        if os.path.isfile(path):
            log.info('Using "%s" to build libuv...' % path)
            env['PYTHON'] = path
            return

    raise DistutilsError('No appropriate Python version found. An '
                         'installation of 2.6 or 2.7 is required to '
                         'build libuv. You can set the environment '
                         'variable "PYTHON" to point to a custom '
                         'installation location.')


class libuv_build_ext(build_ext):
    libuv_dir      = os.path.join('deps', 'libuv')
    libuv_repo     = 'https://github.com/joyent/libuv.git'
    libuv_branch   = 'v0.10'
    libuv_revision = '33959f7'
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
        if self.compiler.compiler_type == 'mingw32':
            # Dirty hack to avoid linking with more than one C runtime when using MinGW
            # Note that this hack forces the compilation to use the old MSVCT
            self.compiler.dll_libraries = [lib for lib in self.compiler.dll_libraries if not lib.startswith('msvcr')]
        self.force = self.libuv_force_fetch or self.libuv_clean_compile
        if self.compiler.compiler_type == 'msvc':
            self.libuv_lib = os.path.join(self.libuv_dir, 'Release', 'lib', 'libuv.lib')
        else:
            self.libuv_lib = os.path.join(self.libuv_dir, 'libuv.a')
        self.get_libuv()
        # Set compiler options
        if self.compiler.compiler_type == 'mingw32':
            self.compiler.add_library_dir(self.libuv_dir)
            self.compiler.add_library('uv')
        else:
            self.extensions[0].extra_objects.extend([self.libuv_lib])
        self.compiler.add_include_dir(os.path.join(self.libuv_dir, 'include'))
        if sys.platform.startswith('linux'):
            self.compiler.add_library('rt')
        elif sys.platform == 'darwin':
            self.extensions[0].extra_link_args.extend(['-framework', 'CoreServices'])
        elif sys.platform == 'win32':
            self.extensions[0].define_macros.append(('WIN32', 1))
            if self.compiler.compiler_type == 'msvc':
                self.extensions[0].extra_link_args.extend(['/NODEFAULTLIB:libcmt', '/LTCG'])
                self.compiler.add_library('advapi32')
            self.compiler.add_library('iphlpapi')
            self.compiler.add_library('psapi')
            self.compiler.add_library('ws2_32')
        build_ext.build_extensions(self)

    def get_libuv(self):
        #self.debug_mode =  bool(self.debug) or hasattr(sys, 'gettotalrefcount')
        win32_msvc = self.compiler.compiler_type=='msvc'
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
            if win32_msvc:
                prepare_windows_env(env)
                libuv_arch = {'32bit': 'x86', '64bit': 'x64'}[platform.architecture()[0]]
                exec_process(['cmd.exe', '/C', 'vcbuild.bat', libuv_arch, 'release'], cwd=self.libuv_dir, env=env, shell=True)
            else:
                exec_process(['make', 'libuv.a'], cwd=self.libuv_dir, env=env)
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
                if win32_msvc:
                    env = os.environ.copy()
                    prepare_windows_env(env)
                    exec_process(['cmd.exe', '/C', 'vcbuild.bat', 'clean'], cwd=self.libuv_dir, env=env, shell=True)
                    rmtree(os.path.join(self.libuv_dir, 'Release'))
                else:
                    exec_process(['make', 'clean'], cwd=self.libuv_dir)
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

