
from __future__ import print_function

import pyuv
import signal
import socket


reader, writer = socket.socketpair()
reader.setblocking(False)
writer.setblocking(False)

def prepare_cb(handle):
    print("Inside prepare_cb")

def excepthook(typ, val, tb):
    print("Inside excepthook")
    if typ is KeyboardInterrupt:
        prepare.stop()
        signal_checker.stop()

loop = pyuv.Loop.default_loop()
loop.excepthook = excepthook
prepare = pyuv.Prepare(loop)
prepare.start(prepare_cb)

signal.set_wakeup_fd(writer.fileno())
signal_checker = pyuv.util.SignalChecker(loop, reader.fileno())
signal_checker.start()

loop.run()

