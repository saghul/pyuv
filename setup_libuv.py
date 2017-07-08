
import os
import sys

from distutils.command.build_ext import build_ext


SOURCES = [
    'deps/libuv/src/fs-poll.c',
    'deps/libuv/src/inet.c',
    'deps/libuv/src/threadpool.c',
    'deps/libuv/src/uv-common.c',
    'deps/libuv/src/version.c',
]

if sys.platform == 'win32':
    SOURCES += [
        'deps/libuv/src/win/async.c',
        'deps/libuv/src/win/core.c',
        'deps/libuv/src/win/detect-wakeup.c',
        'deps/libuv/src/win/dl.c',
        'deps/libuv/src/win/error.c',
        'deps/libuv/src/win/fs-event.c',
        'deps/libuv/src/win/fs.c',
        'deps/libuv/src/win/getaddrinfo.c',
        'deps/libuv/src/win/getnameinfo.c',
        'deps/libuv/src/win/handle.c',
        'deps/libuv/src/win/loop-watcher.c',
        'deps/libuv/src/win/pipe.c',
        'deps/libuv/src/win/poll.c',
        'deps/libuv/src/win/process-stdio.c',
        'deps/libuv/src/win/process.c',
        'deps/libuv/src/win/req.c',
        'deps/libuv/src/win/signal.c',
        'deps/libuv/src/win/snprintf.c',
        'deps/libuv/src/win/stream.c',
        'deps/libuv/src/win/tcp.c',
        'deps/libuv/src/win/thread.c',
        'deps/libuv/src/win/timer.c',
        'deps/libuv/src/win/tty.c',
        'deps/libuv/src/win/udp.c',
        'deps/libuv/src/win/util.c',
        'deps/libuv/src/win/winapi.c',
        'deps/libuv/src/win/winsock.c',
    ]
else:
    SOURCES += [
        'deps/libuv/src/unix/async.c',
        'deps/libuv/src/unix/core.c',
        'deps/libuv/src/unix/dl.c',
        'deps/libuv/src/unix/fs.c',
        'deps/libuv/src/unix/getaddrinfo.c',
        'deps/libuv/src/unix/getnameinfo.c',
        'deps/libuv/src/unix/loop-watcher.c',
        'deps/libuv/src/unix/loop.c',
        'deps/libuv/src/unix/pipe.c',
        'deps/libuv/src/unix/poll.c',
        'deps/libuv/src/unix/process.c',
        'deps/libuv/src/unix/signal.c',
        'deps/libuv/src/unix/stream.c',
        'deps/libuv/src/unix/tcp.c',
        'deps/libuv/src/unix/thread.c',
        'deps/libuv/src/unix/timer.c',
        'deps/libuv/src/unix/tty.c',
        'deps/libuv/src/unix/udp.c',
    ]


if sys.platform.startswith('linux'):
    SOURCES += [
        'deps/libuv/src/unix/linux-core.c',
        'deps/libuv/src/unix/linux-inotify.c',
        'deps/libuv/src/unix/linux-syscalls.c',
        'deps/libuv/src/unix/procfs-exepath.c',
        'deps/libuv/src/unix/proctitle.c',
        'deps/libuv/src/unix/sysinfo-loadavg.c',
        'deps/libuv/src/unix/sysinfo-memory.c',
    ]
elif sys.platform == 'darwin':
    SOURCES += [
        'deps/libuv/src/unix/bsd-ifaddrs.c',
        'deps/libuv/src/unix/darwin.c',
        'deps/libuv/src/unix/darwin-proctitle.c',
        'deps/libuv/src/unix/fsevents.c',
        'deps/libuv/src/unix/kqueue.c',
        'deps/libuv/src/unix/proctitle.c',
        'deps/libuv/src/unix/pthread-barrier.c',
    ]
elif sys.platform.startswith(('freebsd', 'dragonfly')):
    SOURCES += [
        'deps/libuv/src/unix/bsd-ifaddrs.c',
        'deps/libuv/src/unix/freebsd.c',
        'deps/libuv/src/unix/kqueue.c',
        'deps/libuv/src/unix/posix-hrtime.c',
    ]
elif sys.platform.startswith('openbsd'):
    SOURCES += [
        'deps/libuv/src/unix/bsd-ifaddrs.c',
        'deps/libuv/src/unix/kqueue.c',
        'deps/libuv/src/unix/openbsd.c',
        'deps/libuv/src/unix/posix-hrtime.c',
    ]
elif sys.platform.startswith('netbsd'):
    SOURCES += [
        'deps/libuv/src/unix/bsd-ifaddrs.c',
        'deps/libuv/src/unix/kqueue.c',
        'deps/libuv/src/unix/netbsd.c',
        'deps/libuv/src/unix/posix-hrtime.c',
    ]

elif sys.platform.startswith('sunos'):
    SOURCES += [
        'deps/libuv/src/unix/no-proctitle.c',
        'deps/libuv/src/unix/sunos.c',
    ]


class libuv_build_ext(build_ext):
    libuv_dir = os.path.join('deps', 'libuv')

    user_options = build_ext.user_options
    user_options.extend([
        ("use-system-libuv", None, "Use the system provided libuv, instead of the bundled one"),
    ])
    boolean_options = build_ext.boolean_options
    boolean_options.extend(["use-system-libuv"])

    def initialize_options(self):
        build_ext.initialize_options(self)
        self.use_system_libuv = 0

    def build_extensions(self):
        if self.use_system_libuv:
            self.compiler.add_library('uv')
        else:
            self.compiler.add_include_dir(os.path.join(self.libuv_dir, 'include'))
            self.compiler.add_include_dir(os.path.join(self.libuv_dir, 'src'))
            self.extensions[0].sources += SOURCES

        if sys.platform != 'win32':
            self.compiler.define_macro('_LARGEFILE_SOURCE', 1)
            self.compiler.define_macro('_FILE_OFFSET_BITS', 64)

        if sys.platform.startswith('linux'):
            self.compiler.add_library('dl')
            self.compiler.add_library('rt')
        elif sys.platform == 'darwin':
            self.compiler.define_macro('_DARWIN_USE_64_BIT_INODE', 1)
            self.compiler.define_macro('_DARWIN_UNLIMITED_SELECT', 1)
        elif sys.platform.startswith('netbsd'):
            self.compiler.add_library('kvm')
        elif sys.platform.startswith('sunos'):
            self.compiler.define_macro('__EXTENSIONS__', 1)
            self.compiler.define_macro('_XOPEN_SOURCE', 500)
            self.compiler.add_library('kstat')
            self.compiler.add_library('nsl')
            self.compiler.add_library('sendfile')
            self.compiler.add_library('socket')
        elif sys.platform == 'win32':
            self.compiler.define_macro('_GNU_SOURCE', 1)
            self.compiler.define_macro('WIN32', 1)
            self.compiler.define_macro('_CRT_SECURE_NO_DEPRECATE', 1)
            self.compiler.define_macro('_CRT_NONSTDC_NO_DEPRECATE', 1)
            self.compiler.define_macro('_WIN32_WINNT', '0x0600')
            self.compiler.add_library('advapi32')
            self.compiler.add_library('iphlpapi')
            self.compiler.add_library('psapi')
            self.compiler.add_library('shell32')
            self.compiler.add_library('user32')
            self.compiler.add_library('userenv')
            self.compiler.add_library('ws2_32')

        build_ext.build_extensions(self)
