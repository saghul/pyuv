
from __future__ import absolute_import, print_function

import logging
import pyuv


class Handler(object):
    """A generic container for a pyuv `Handle`, with a `tulip.events.Handler`
    interface.

    NOTE: this class does not derive from `tulip.events.Handler`.
    """

    def __init__(self, handle, callback, args, single_shot=False):
        self.handle = handle
        self.when = None  # tulip compatibility
        self.callback = callback
        self.args = args
        self.single_shot = single_shot
        self.cancelled = False

    def __repr__(self):
        res = 'Handler({}, {}, {}, single_shot={})' \
                    .format(self.handle, self.callback,
                            self.args, self.single_shot)
        if self.cancelled:
            res += ' <cancelled>'
        return res

    def cancel(self):
        self.cancelled = True
        loop = self.handle.loop._tulip_loop
        if self in loop._handlers:
            loop._handlers.remove(self)
        self.handle.close()
        self.handle = None

    def __call__(self, *ignored):
        if self.cancelled:
            return
        try:
            self.callback(*self.args)
        except Exception:
            logging.exception('Exception in callback %s %r',
                               self.callback, self.args)
        if self.single_shot:
            self.cancel()


def make_handler(handle, callback, args, single_shot=False):
    if isinstance(callback, Handler):
        assert not args
        assert callback.handle is None
        callback.handle = handle
        callback.single_shot = single_shot
        return callback
    return Handler(handle, callback, args, single_shot)


class SharedPoll(object):
    """A shared poll handle.

    This is like pyuv.Poll, but multiple instances can be active
    for the same file descriptor.
    """

    def __init__(self, loop, fd):
        self.loop = loop
        self.fd = fd
        if not hasattr(loop, '_polls'):
            loop._polls = {}
        try:
            poll = loop._polls[fd]
        except KeyError:
            poll = pyuv.Poll(loop, fd)
            poll._events = 0
            poll._readers = set()
            poll._writers = set()
            loop._polls[fd] = poll
        self.poll = poll
        self.events = 0
        self.callback = None
        self.closed = False

    def __del__(self):
        self.close()

    def start(self, events, callback):
        if self.events:
            self.stop()
        self.events = events
        self.callback = callback
        if events & pyuv.UV_READABLE:
            self.poll._readers.add(callback)
        if events & pyuv.UV_WRITABLE:
            self.poll._writers.add(callback)
        self._adjust()

    def stop(self):
        if self.events & pyuv.UV_READABLE:
            self.poll._readers.remove(self.callback)
        if self.events & pyuv.UV_WRITABLE:
            self.poll._writers.remove(self.callback)
        self.events = 0
        self.callback = None
        self._adjust()

    def close(self):
        if self.closed:
            return
        self.stop()
        if self.poll._readers or self.poll._writers:
            return
        self.poll.close()
        self.poll = None
        del self.loop._polls[self.fd]
        self.loop = None
        self.fd = None
        self.closed = True

    def _adjust(self):
        mask = 0
        if self.poll._readers:
            mask |= pyuv.UV_READABLE
        if self.poll._writers:
            mask |= pyuv.UV_WRITABLE
        if mask and mask != self.poll._events:
            self.poll._events = mask
            self.poll.start(mask, self._poll_callback)
        elif not mask and mask != self.poll._events:
            self.poll.stop()

    @staticmethod
    def _poll_callback(handle, events, errno):
        if errno:
            return
        if events & pyuv.UV_READABLE:
            for callback in list(handle._readers):
                callback()
        if events & pyuv.UV_WRITABLE:
            for callback in list(handle._writers):
                callback()


class EventLoop(object):
    """An tulip EventLoop for pyuv.

    This implements `tulip.events.EventLoop` for pyuv. Currently only a minimal
    subset is implemented.

    NOTE: this class does not derive from `tulip.events.EventLoop`.
    """

    def __init__(self, loop=None):
        self.loop = loop or pyuv.Loop.default_loop()
        self.loop._tulip_loop = self
        # libuv does not support multiple callbacks per file descriptor.
        # Therefore we need to keep a map from file descriptors to handler, and
        # we also need to multiplex readers and writers for the same FD onto
        # one Poll instance.
        self._readers = {}  # { fd: reader, ... }
        self._writers = {}  # { fd: writer, ... }
        self._handlers = set()
        self._stop_handle = None

    # Running the event loop

    def run(self):
        """Run until there are no more active handles or until `stop()` is
        called."""
        stopped = [False]  # lack of "nonlocal" in Python 2.x
        def stop_loop(async_handle):
            stopped[0] = True
        self._stop_handle = pyuv.Async(self.loop, stop_loop)
        # There is currently no way to terminate a loop in libuv.  Therefore,
        # to implement the stop() method, we need to run the loop one iteration
        # at a time, and we check every iteration if we need to stop. 
        while not stopped[0] and self.loop.active_handles > 1:  # _stop_handle
            self.run_once()
        self._stop_handle.close()
        self._stop_handle = None

    def run_forever(self):
        """Run until stop() is called."""
        # Keep the loop busy with a once-a-day timer
        timer = pyuv.Timer(self.loop)
        timer.start(24*3600, lambda *args: None)
        self.run()
        timer.close()

    def run_once(self, timeout=None):
        if timeout is not None:
            timer = pyuv.Timer(self.loop)
            def stop_loop(handle):
                pass
            timer.start(stop_loop, timeout, 0)
        self.loop.run(pyuv.UV_RUN_ONCE)
        if timeout is not None:
            timer.close()

    def stop(self):
        if self._stop_handle:
            self._stop_handle.send()

    # Scheduling callbacks

    def call_later(self, delay, callback, *args):
        timer = pyuv.Timer(self.loop)
        handler = make_handler(timer, callback, args, single_shot=True)
        # The user is not required to keep a reference to the handler we
        # return. Therefore, we need to ensure that we keep a reference to the
        # pyuv handle ourselves. If not, it might get garbage collected.
        self._handlers.add(handler)
        timer.start(handler, delay, 0)
        return handler

    def call_repeatedly(self, interval, callback, *args):
        timer = pyuv.Timer(self.loop)
        handler = make_handler(timer, callback, args, single_shot=False)
        self._handlers.add(handler)
        timer.start(handler, interval, interval)
        return handler

    def call_soon(self, callback, *args):
        timer = pyuv.Timer(self.loop)
        handler = make_handler(timer, callback, args, single_shot=True)
        self._handlers.add(handler)
        timer.start(handler, 0, 0)
        return handler

    def call_soon_threadsafe(self, callback, *args):
        handler = make_handler(None, callback, args, single_shot=True)
        self._handlers.add(handler)
        async = pyuv.Async(self.loop, handler)
        handler.handle = async  # break circural dependency for constructor
        async.send()
        return handler

    # Low-level FD operations

    def add_reader(self, fd, callback, *args):
        reader = self._readers.get(fd)
        if reader:
            raise RuntimeError('cannot add multiple readers per fd')
        poll = SharedPoll(self.loop, fd)
        reader = make_handler(poll, callback, args)
        self._readers[fd] = reader
        poll.start(pyuv.UV_READABLE, reader)
        return reader

    def add_writer(self, fd, callback, *args):
        writer = self._writers.get(fd)
        if writer:
            raise ValueError('Cannot add multiple writers per fd')
        poll = SharedPoll(self.loop, fd)
        writer = make_handler(poll, callback, args)
        self._writers[fd] = writer
        poll.start(pyuv.UV_WRITABLE, writer)
        return writer

    def remove_reader(self, fd):
        reader = self._readers.get(fd)
        if not reader:
            return False
        reader.cancel()
        del self._readers[fd]
        return True

    def remove_writer(self, fd):
        writer = self._writers.get(fd)
        if not writer:
            return False
        writer.cancel()
        del self._writers[fd]
        return True
