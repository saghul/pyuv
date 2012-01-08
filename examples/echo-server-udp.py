
import os
import signal
import threading
import pyuv


def on_read(handle, (ip, port), data, error):
    data = data.strip()
    if data:
	handle.send(data+os.linesep, (ip, port))

def async_exit(async):
    async.close()
    signal_h.close()
    server.close()

def signal_cb(sig, frame):
    async.send(async_exit)


print "PyUV version %s" % pyuv.__version__

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

print "Stopped!"

