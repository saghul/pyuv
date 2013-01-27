#
# Adapted from tulip's test_events.py. Apache 2.0 license.

from common import unittest2

import pyuv
import time
import socket
import threading
import io


def get_event_loop():
    uvloop = pyuv.Loop()  # a new loop for every test
    loop = pyuv.tulip.EventLoop(uvloop)
    return loop


try:
    socketpair = socket.socketpair

except AttributeError:

    def socketpair(family=socket.AF_INET, type=socket.SOCK_STREAM, proto=0):
        """Emulate the Unix socketpair() function on Windows."""
        # We create a connected TCP socket. Note the trick with setblocking(0)
        # that prevents us from having to create a thread.
        lsock = socket.socket(family, type, proto)
        lsock.bind(('localhost', 0))
        lsock.listen(1)
        addr, port = lsock.getsockname()
        csock = socket.socket(family, type, proto)
        csock.setblocking(False)
        try:
            csock.connect((addr, port))
        except socket.error as e:
            if e.errno != errno.WSAEWOULDBLOCK:
                lsock.close()
                csock.close()
                raise
        ssock, _ = lsock.accept()
        csock.setblocking(True)
        lsock.close()
        return (ssock, csock)


class TulipTest(unittest2.TestCase):

    def testCallLater(self):
        el = get_event_loop()
        results = []
        def callback(arg):
            results.append(arg)
        el.call_later(0.1, callback, 'hello world')
        t0 = time.time()
        el.run()
        t1 = time.time()
        self.assertEqual(results, ['hello world'])
        self.assertTrue(t1-t0 >= 0.09)

    def testCallRepeatedly(self):
        el = get_event_loop()
        results = []
        def callback(arg):
            results.append(arg)
        el.call_repeatedly(0.03, callback, 'ho')
        el.call_later(0.1, el.stop)
        el.run()
        self.assertEqual(results, ['ho', 'ho', 'ho'])

    def testCallSoon(self):
        el = get_event_loop()
        results = []
        def callback(arg1, arg2):
            results.append((arg1, arg2))
        el.call_soon(callback, 'hello', 'world')
        el.run()
        self.assertEqual(results, [('hello', 'world')])

    def testCallSoonWithHandler(self):
        el = get_event_loop()
        results = []
        def callback():
            results.append('yeah')
        handler = pyuv.tulip.Handler(None, callback, ())
        self.assertEqual(el.call_soon(handler), handler)
        el.run()
        self.assertEqual(results, ['yeah'])

    def testCallSoonThreadsafe(self):
        el = get_event_loop()
        results = []
        def callback(arg):
            results.append(arg)
        def run():
            el.call_soon_threadsafe(callback, 'hello')
        t = threading.Thread(target=run)
        el.call_later(0.1, callback, 'world')
        t0 = time.time()
        t.start()
        el.run()
        t1 = time.time()
        t.join()
        self.assertEqual(results, ['hello', 'world'])
        self.assertTrue(t1-t0 >= 0.09)

    def testCallSoonThreadsafeWithHandler(self):
        el = get_event_loop()
        results = []
        def callback(arg):
            results.append(arg)
        handler = pyuv.tulip.Handler(None, callback, ('hello',))
        def run():
            self.assertEqual(el.call_soon_threadsafe(handler), handler)
        t = threading.Thread(target=run)
        el.call_later(0.1, callback, 'world')
        t0 = time.time()
        t.start()
        el.run()
        t1 = time.time()
        t.join()
        self.assertEqual(results, ['hello', 'world'])
        self.assertTrue(t1-t0 >= 0.09)

    def testReaderCallback(self):
        el = get_event_loop()
        r, w = socketpair()
        bytes_read = []
        def reader():
            try:
                data = r.recv(1024)
            except io.BlockingIOError:
                # Spurious readiness notifications are possible
                # at least on Linux -- see man select.
                return
            if data:
                bytes_read.append(data)
            else:
                self.assertTrue(el.remove_reader(r.fileno()))
                r.close()
        el.add_reader(r.fileno(), reader)
        el.call_later(0.05, w.send, b'abc')
        el.call_later(0.1, w.send, b'def')
        el.call_later(0.15, w.close)
        el.run()
        self.assertEqual(b''.join(bytes_read), b'abcdef')

    def testReaderCallbackWithHandler(self):
        el = get_event_loop()
        r, w = socketpair()
        bytes_read = []
        def reader():
            try:
                data = r.recv(1024)
            except io.BlockingIOError:
                # Spurious readiness notifications are possible
                # at least on Linux -- see man select.
                return
            if data:
                bytes_read.append(data)
            else:
                self.assertTrue(el.remove_reader(r.fileno()))
                r.close()
        handler = pyuv.tulip.Handler(None, reader, ())
        self.assertEqual(el.add_reader(r.fileno(), handler), handler)
        el.call_later(0.05, w.send, b'abc')
        el.call_later(0.1, w.send, b'def')
        el.call_later(0.15, w.close)
        el.run()
        self.assertEqual(b''.join(bytes_read), b'abcdef')

    def testReaderCallbackCancel(self):
        el = get_event_loop()
        r, w = socketpair()
        bytes_read = []
        def reader():
            data = r.recv(1024)
            if data:
                bytes_read.append(data)
            if sum(len(b) for b in bytes_read) >= 6:
                handler.cancel()
            if not data:
                r.close()
        handler = el.add_reader(r.fileno(), reader)
        el.call_later(0.05, w.send, b'abc')
        el.call_later(0.1, w.send, b'def')
        el.call_later(0.15, w.close)
        el.run()
        self.assertEqual(b''.join(bytes_read), b'abcdef')

    def testWriterCallback(self):
        el = get_event_loop()
        r, w = socketpair()
        w.setblocking(False)
        el.add_writer(w.fileno(), w.send, b'x'*(256*1024))
        def remove_writer():
            self.assertTrue(el.remove_writer(w.fileno()))
        el.call_later(0.1, remove_writer)
        el.run()
        w.close()
        data = r.recv(256*1024)
        r.close()
        self.assertTrue(len(data) >= 200)

    def testWriterCallbackWithHandler(self):
        el = get_event_loop()
        r, w = socketpair()
        w.setblocking(False)
        handler = pyuv.tulip.Handler(None, w.send, (b'x'*(256*1024),))
        self.assertEqual(el.add_writer(w.fileno(), handler), handler)
        def remove_writer():
            self.assertTrue(el.remove_writer(w.fileno()))
        el.call_later(0.1, remove_writer)
        el.run()
        w.close()
        data = r.recv(256*1024)
        r.close()
        self.assertTrue(len(data) >= 200)

    def testWriterCallbackCancel(self):
        el = get_event_loop()
        r, w = socketpair()
        w.setblocking(False)
        def sender():
            w.send(b'x'*256)
            handler.cancel()
        handler = el.add_writer(w.fileno(), sender)
        el.run()
        w.close()
        data = r.recv(1024)
        r.close()
        self.assertTrue(data == b'x'*256)


if __name__ == '__main__':
    unittest2.main(verbosity=2)
