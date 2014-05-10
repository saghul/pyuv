from __future__ import print_function
import pyuv
import signal

PATH = '.'


def fsevent_callback(fsevent_handle, filename, events, error):
    print(filename)
    print(events)


def sig_cb(handle, signum):
    fsevents.close()
    signal_h.close()


loop = pyuv.Loop.default_loop()
fsevents = pyuv.fs.FSEvent(loop, PATH, fsevent_callback, 0)
signal_h = pyuv.Signal(loop)
signal_h.start(sig_cb, signal.SIGINT)
loop.run()
