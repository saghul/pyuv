
from __future__ import print_function

import signal
import pyuv


def on_read(handle, ip_port, flags, data, error):
    if data is not None:
        handle.send(ip_port, data)

def signal_cb(handle, signum):
    signal_h.close()
    server.close()


print("PyUV version %s" % pyuv.__version__)

loop = pyuv.Loop.default_loop()

server = pyuv.UDP(loop)
server.bind(("0.0.0.0", 1234))
server.start_recv(on_read)

signal_h = pyuv.Signal(loop)
signal_h.start(signal_cb, signal.SIGINT)

loop.run()

print("Stopped!")

