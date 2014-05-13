from __future__ import print_function
import pyuv
import signal
import sys
import os
import argparse


def fsevent_callback(fsevent_handle, filename, events, error):
    print('file: %s' % filename, ', events: ', events)


def sig_cb(handle, signum):
    handle.close()
    print('\ntype ctrl-c again to exit')


def main(path):
    loop = pyuv.Loop.default_loop()
    try:
        fsevents = pyuv.fs.FSEvent(loop)
        fsevents.start(path, 0, fsevent_callback)
    except pyuv.error.FSEventError as err:
        print('error', err)
        sys.exit(2)

    signal_h = pyuv.Signal(loop)
    signal_h.start(sig_cb, signal.SIGINT)
    print('Watching path %s' % os.path.abspath(path))
    loop.run()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--path', help='a path to watch', required=True)
    main(parser.parse_args().path)