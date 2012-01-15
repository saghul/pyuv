
import os
import sys
import pyuv

if sys.version_info >= (3, 0):
    LINESEP = os.linesep.encode()
else:
    LINESEP = os.linesep

def on_pipe_read(handle, data, error):
    data = data.strip()
    if data == b"exit":
        pipe_stdin.close()
        pipe_stdout.close()
    else:
        pipe_stdout.write([data, LINESEP])


loop = pyuv.Loop.default_loop()

pipe_stdin = pyuv.Pipe(loop)
pipe_stdin.open(sys.stdin.fileno())
pipe_stdin.start_read(on_pipe_read)

pipe_stdout = pyuv.Pipe(loop)
pipe_stdout.open(sys.stdout.fileno())

loop.run()

