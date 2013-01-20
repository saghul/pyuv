
from __future__ import print_function

import signal
import sys
import pyuv


def on_tty_read(handle, data, error):
    if data is None or data == b"exit":
        tty_stdin.close()
        tty_stdout.close()
    else:
        tty_stdout.write(data)

def signal_cb(handle, signum):
    tty_stdin.close()
    tty_stdout.close()
    signal_h.close()


loop = pyuv.Loop.default_loop()

tty_stdin = pyuv.TTY(loop, sys.stdin.fileno(), True)
tty_stdin.start_read(on_tty_read)
tty_stdout = pyuv.TTY(loop, sys.stdout.fileno(), False)

if sys.platform != "win32":
    print("Window size: (%d, %d)" % tty_stdin.get_winsize())

signal_h = pyuv.Signal(loop)
signal_h.start(signal_cb, signal.SIGINT)

loop.run()

pyuv.TTY.reset_mode()

