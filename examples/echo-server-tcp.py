
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


def on_read(client, data, error):
    if data is None:
        client.close()
        clients.remove(client)
        return
    data = data.strip()
    if not data:
        return
    client.write(data+LINESEP)

def on_connection(server, error):
    client = pyuv.TCP(server.loop)
    server.accept(client)
    clients.append(client)
    client.start_read(on_read)

def async_exit(async):
    [c.close() for c in clients]
    async.close()
    signal_h.close()
    server.close()

def signal_cb(sig, frame):
    async.send(async_exit)


print("PyUV version %s" % pyuv.__version__)

loop = pyuv.Loop()

async = pyuv.Async(loop)
clients = []

server = pyuv.TCP(loop)
server.bind(("0.0.0.0", 1234))
server.listen(on_connection)

signal_h = pyuv.Signal(loop)
signal_h.start()

t = threading.Thread(target=loop.run)
t.start()

signal.signal(signal.SIGINT, signal_cb)
signal.pause()

t.join()

print("Stopped!")


