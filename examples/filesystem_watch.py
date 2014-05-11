from __future__ import print_function
import pyuv
import signal
import sys
import getopt
import os


def usage():
    print('usage: filesystem_watch.py -p path_to_watch')

def fsevent_callback(fsevent_handle, filename, events, error):
    print('file: %s' % filename, ', events: ', events)


def sig_cb(handle, signum):
    handle.close()
    print('\ntype ctrl-c again to exit')


def main(path):
    print('watching path %s' % os.path.abspath(path))   
    loop = pyuv.Loop.default_loop()
    try:
        fsevents = pyuv.fs.FSEvent(loop, path, fsevent_callback, 0)
    except pyuv.error.FSEventError as err:
        print('error', err)
        sys.exit(2)

    signal_h = pyuv.Signal(loop)
    signal_h.start(sig_cb, signal.SIGINT)
    loop.run()


if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "p:")
        for k, v in opts:
            if k == '-p':
                path = v
    except getopt.GetoptError as err:
        print(err)
        usage()
        sys.exit(2)

    main(path)



