
import errno
import select
import unittest

from threading import Semaphore, Thread

import common; common
import pyuv

try:
    poller = select.epoll
    poller_type = 'epoll'
except AttributeError:
    try:
        poller = select.kqueue
        poller_type = 'kqueue'
    except AttributeError:
        poller = None


class EmbedTest(unittest.TestCase):

    def embed_cb(self, handle):
        self.loop.run(pyuv.UV_RUN_ONCE)
        self.sem.release()

    def timer_cb(self, handle):
        self.embed_timer_called += 1
        self.embed_closed = True
        self.embed_async.close()
        handle.close()

    def poll(self, poll_obj, timeout):
        if poller_type == 'kqueue':
            poll_obj.control(None, 0, timeout)
        elif poller_type == 'epoll':
            poll_obj.poll(timeout)
        else:
            self.fail('Bogus poller type')

    def embed_runner(self):
        fd = self.loop.fileno()
        poll = poller.fromfd(fd)
        while not self.embed_closed:
            timeout = self.loop.get_timeout()
            while True:
                try:
                    self.poll(poll, timeout)
                except OSError as e:
                    if e.args[0] == errno.EINTR:
                        continue
                break
            self.embed_async.send()
            self.sem.acquire()

    def test_embed(self):
        if poller is None:
            self.skipTest("test disabled if no suitable poller method is found")
            return
        self.embed_timer_called = 0
        self.embed_closed = False
        self.external = pyuv.Loop()
        self.embed_async = pyuv.Async(self.external, self.embed_cb)

        self.loop = pyuv.Loop()
        timer = pyuv.Timer(self.loop)
        timer.start(self.timer_cb, 0.25, 0)

        self.sem = Semaphore(0)
        t = Thread(target=self.embed_runner)
        t.start()
        self.external.run()
        t.join()
        external = None

        self.assertEqual(self.embed_timer_called, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)

