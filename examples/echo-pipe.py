
import os
import sys
import pyuv


def on_pipe_read(handle, data, error):
    data = data.strip()
    if data == "exit":
        pipe_stdin.close()
        pipe_stdout.close()
    else:
        pipe_stdout.write([data, os.linesep])


loop = pyuv.Loop.default_loop()

pipe_stdin = pyuv.Pipe(loop)
pipe_stdin.open(sys.stdin.fileno())
pipe_stdin.start_read(on_pipe_read)

pipe_stdout = pyuv.Pipe(loop)
pipe_stdout.open(sys.stdout.fileno())

loop.run()

