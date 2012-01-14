
import os
import sys
import pyuv

if (sys.version_info >= (3, 0)):
    LINESEP = os.linesep.encode('latin-1')
else:
    LINESEP = os.linesep

def on_tty_read(handle, data, error):
    data = data.strip()
    if data == "exit":
        tty_stdin.close()
        tty_stdout.close()
    else:
        tty_stdout.write([data, LINESEP])

loop = pyuv.Loop.default_loop()

tty_stdin = pyuv.TTY(loop, sys.stdin.fileno())
tty_stdin.start_read(on_tty_read)

tty_stdout = pyuv.TTY(loop, sys.stdout.fileno())

print "Window size: (%d, %d)" % tty_stdin.get_winsize()

loop.run()

pyuv.TTY.reset_mode()

