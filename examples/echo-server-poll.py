
import sys
import socket
import signal
import weakref
import errno
import logging
import pyuv

logging.basicConfig(level=logging.DEBUG)

STOPSIGNALS = (signal.SIGINT, signal.SIGTERM)
NONBLOCKING = (errno.EAGAIN, errno.EWOULDBLOCK)
if sys.platform == "win32":
    NONBLOCKING = NONBLOCKING + (errno.WSAEWOULDBLOCK,)


class Connection(object):

    def __init__(self, sock, address, loop):
        self.sock = sock
        self.address = address
        self.sock.setblocking(0)
        self.buf = ""
        self.watcher = pyuv.Poll(loop, self.sock._sock)
        self.watcher.start(pyuv.UV_READABLE, self.io_cb)
        logging.debug("{0}: ready".format(self))

    def reset(self, events):
        self.watcher.start(events, self.io_cb)

    def handle_error(self, msg, level=logging.ERROR, exc_info=True):
        logging.log(level, "{0}: {1} --> closing".format(self, msg), exc_info=exc_info)
        self.close()

    def handle_read(self):
        try:
            buf = self.sock.recv(1024)
        except socket.error as err:
            if err.args[0] not in NONBLOCKING:
                self.handle_error("error reading from {0}".format(self.sock))
        if buf:
            self.buf += buf
            self.reset(pyuv.UV_READABLE | pyuv.UV_WRITABLE)
        else:
            self.handle_error("connection closed by peer", logging.DEBUG, False)

    def handle_write(self):
        try:
            sent = self.sock.send(self.buf)
        except socket.error as err:
            if err.args[0] not in NONBLOCKING:
                self.handle_error("error writing to {0}".format(self.sock))
        else :
            self.buf = self.buf[sent:]
            if not self.buf:
                self.reset(pyuv.UV_READABLE)

    def io_cb(self, watcher, revents, error):
        if error is not None:
            logging.error("Error in connection: %d: %s" % (error, pyuv.errno.strerror(error)))
            return
        if revents & pyuv.UV_READABLE:
            self.handle_read()
        elif revents & pyuv.UV_WRITABLE:
            self.handle_write()

    def close(self):
        self.watcher.stop()
        self.watcher = None
        self.sock.close()
        logging.debug("{0}: closed".format(self))


class Server(object):

    def __init__(self, address):
        self.sock = socket.socket()
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(address)
        self.sock.setblocking(0)
        self.address = self.sock.getsockname()
        self.loop = pyuv.Loop.default_loop()
        self.signal_watcher = pyuv.Signal(self.loop)
        self.poll_watcher = pyuv.Poll(self.loop, self.sock.fileno())
        self.async = pyuv.Async(self.loop, self.async_cb)
        self.conns = weakref.WeakValueDictionary()
        for sig in STOPSIGNALS:
            signal.signal(sig, self.signal_cb)

    def handle_error(self, msg, level=logging.ERROR, exc_info=True):
        logging.log(level, "{0}: {1} --> stopping".format(self, msg), exc_info=exc_info)
        self.stop()

    def signal_cb(self, sig, frame):
        self.async.send()

    def async_cb(self, handle):
        self.stop()

    def io_cb(self, watcher, revents, error):
        try:
            while True:
                try:
                    sock, address = self.sock.accept()
                except socket.error as err:
                    if err.args[0] in NONBLOCKING:
                        break
                    else:
                        raise
                else:
                    self.conns[address] = Connection(sock, address, self.loop)
        except Exception:
            self.handle_error("error accepting a connection")

    def start(self):
        self.sock.listen(socket.SOMAXCONN)
        self.poll_watcher.start(pyuv.UV_READABLE, self.io_cb)
        self.signal_watcher.start()
        logging.debug("{0}: started on {0.address}".format(self))
        self.loop.run()
        logging.debug("{0}: stopped".format(self))

    def stop(self):
        self.poll_watcher.stop()
        self.signal_watcher.stop()
        self.sock.close()
        for conn in self.conns.values():
            conn.close()
        logging.debug("{0}: stopping".format(self))


if __name__ == "__main__":
    server = Server(("127.0.0.1", 9876))
    server.start()

