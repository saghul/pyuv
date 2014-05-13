
from __future__ import print_function

import pyuv
import signal
import sys
import os
import optparse


def fsevent_callback(fsevent_handle, filename, events, error):
    if error is not None:
        txt = 'error %s: %s' % (error, pyuv.errno.strerror(error))
    else:
        evts = []
        if events & pyuv.fs.UV_RENAME:
            evts.append('rename')
        if events & pyuv.fs.UV_CHANGE:
            evts.append('change')
        txt = 'events: %s' % ', '.join(evts)
    print('file: %s, %s' % (filename, txt))


def sig_cb(handle, signum):
    handle.close()


def main(path):
    loop = pyuv.Loop.default_loop()
    try:
        fsevents = pyuv.fs.FSEvent(loop)
        fsevents.start(path, 0, fsevent_callback)
        fsevents.ref = False
    except pyuv.error.FSEventError as e:
        print('error: %s' % e)
        sys.exit(2)
    signal_h = pyuv.Signal(loop)
    signal_h.start(sig_cb, signal.SIGINT)
    print('Watching path %s' % os.path.abspath(path))
    loop.run()


if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-p', '--path', help='a path to watch', default='.')
    opts, args = parser.parse_args()
    main(opts.path)

