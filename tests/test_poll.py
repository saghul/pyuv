
from common import unittest2
import common
import errno
import pyuv
import socket
import sys


TEST_PORT = 1234

if sys.platform == "win32":
    NONBLOCKING = (errno.WSAEWOULDBLOCK,)
else:
    NONBLOCKING = (errno.EAGAIN, errno.EINPROGRESS, errno.EWOULDBLOCK)

class PollTest(unittest2.TestCase):

    def setUp(self):
        self.loop = pyuv.Loop.default_loop()
        self.poll = None
        self.server = None
        self.sock = None
        self.client_connection = None
        self.connecting = False

    def on_connection(self, server, error):
        self.assertEqual(error, None)
        client = pyuv.TCP(pyuv.Loop.default_loop())
        server.accept(client)
        self.client_connection = client
        client.start_read(self.on_client_connection_read)
        client.write(b"PING"+common.linesep)

    def on_client_connection_read(self, client, data, error):
        self.assertEqual(data, b"PONG"+common.linesep)
        self.poll.close()
        self.client_connection.close()
        self.server.close()

    def on_client_connection(self, client, error):
        self.assertEqual(error, None)
        client.start_read(self.on_client_read)

    def poll_cb(self, handle, events, error):
        if self.connecting:
            err = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)
            if not err:
                self.connecting = False
                self.poll.stop()
                self.poll.start(pyuv.UV_READABLE, self.poll_cb)
            elif err in NONBLOCKING:
                return
            else:
                self.poll.close()
                self.client_connection.close()
                self.server.close()
                self.fail("Error connecting socket: %d" % err)
            return
        if (events & pyuv.UV_READABLE):
            self.poll.stop()
            data = self.sock.recv(1024)
            self.assertEqual(data, b"PING"+common.linesep)
            self.poll.start(pyuv.UV_WRITABLE, self.poll_cb)
        elif (events & pyuv.UV_WRITABLE):
            self.poll.stop()
            self.sock.send(b"PONG"+common.linesep)

    def test_poll(self):
        self.server = pyuv.TCP(self.loop)
        self.server.bind(("0.0.0.0", TEST_PORT))
        self.server.listen(self.on_connection)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setblocking(False)
        while True:
            r = self.sock.connect_ex(("127.0.0.1", TEST_PORT))
            if r and r != errno.EINTR:
                break
        if r not in NONBLOCKING:
            self.server.close()
            self.fail("Error connecting socket: %d" % r)
            return
        self.connecting = True
        self.poll = pyuv.Poll(self.loop, self.sock.fileno())
        self.poll.start(pyuv.UV_WRITABLE, self.poll_cb)
        self.loop.run()


if __name__ == '__main__':
    unittest2.main(verbosity=2)


