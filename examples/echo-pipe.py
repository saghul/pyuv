import signal
import sys
import pyuv


def on_pipe_read(handle, data, error):
    if data is None or data == b"exit":
        pipe_stdin.close()
        pipe_stdout.close()
    else:
        pipe_stdout.write(data)

def signal_cb(handle, signum):
    if not pipe_stdin.closed:
        pipe_stdin.close()
    if not pipe_stdin.closed:
        pipe_stdout.close()
    signal_h.close()


loop = pyuv.Loop.default_loop()

pipe_stdin = pyuv.Pipe(loop)
pipe_stdin.open(sys.stdin.fileno())
pipe_stdin.start_read(on_pipe_read)

pipe_stdout = pyuv.Pipe(loop)
pipe_stdout.open(sys.stdout.fileno())

signal_h = pyuv.Signal(loop)
signal_h.start(signal_cb, signal.SIGINT)

loop.run()

