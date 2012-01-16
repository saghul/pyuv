
from __future__ import print_function

import os
import sys
import signal
import threading
import pyuv

if sys.version_info >= (3, 0):
    LINESEP = os.linesep.encode()
else:
    LINESEP = os.linesep

def on_read(handle, ip_port, data, error):
    data = data.strip()
    if data:
        ip, port = ip_port
        handle.send(data+LINESEP, (ip, port))

def async_exit(async):
    async.close()
    signal_h.close()
    server.close()

def signal_cb(sig, frame):
    async.send(async_exit)


print("PyUV version %s" % pyuv.__version__)

loop = pyuv.Loop()
async = pyuv.Async(loop)

server = pyuv.UDP(loop)
server.bind(("0.0.0.0", 1234))
server.start_recv(on_read)

signal_h = pyuv.Signal(loop)
signal_h.start()

t = threading.Thread(target=loop.run)
t.start()

signal.signal(signal.SIGINT, signal_cb)
signal.pause()

t.join()

print("Stopped!")

