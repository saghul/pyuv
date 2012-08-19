from __future__ import print_function

import os
import sys
import signal
import pyuv

if sys.version_info >= (3, 0):
    LINESEP = os.linesep.encode()
else:
    LINESEP = os.linesep

server = None
clients = []

def on_read(client, data, error):
    global clients
    if data is None:
        client.close()
        clients.remove(client)
        return
    data = data.strip()
    if not data:
        return
    client.write(data+LINESEP)

def on_connection(server, error):
    global clients
    client = pyuv.TCP(server.loop)
    server.accept(client)
    clients.append(client)
    client.start_read(on_read)


def init_signal(loop, server):
    global clients

    def async_exit(async):
        [c.close() for c in clients]
        async.close()
        signal_h.close()
        server.close()

    async = pyuv.Async(loop, async_exit)

    def signal_cb(handle, signum):
        print("got signal")
        async.send()

    signal.signal(signal.SIGINT, signal_cb)
    signal_h = pyuv.Signal(loop)
    signal_h.start()


if __name__ == "__main__":
    print("PyUV version %s" % pyuv.__version__)
    loop = pyuv.Loop.default_loop()

    server = pyuv.TCP(loop)
    server.bind(("0.0.0.0", 1234))
    server.listen(on_connection)


    init_signal(loop, server)
    print("Start loop")
    loop.run()
    print("Stopped!")
