#!/usr/bin/env python

import sys
sys.path.insert(0, '../')

import pyuv


def on_write2(handle, error):
    global channel, recv_handle
    recv_handle.close()
    channel.close()

def on_channel_read(handle, data, pending, error):
    global channel, loop, recv_handle
    assert pending == pyuv.UV_NAMED_PIPE, "wrong handle"
    recv_handle = pyuv.Pipe(loop)
    channel.accept(recv_handle)
    channel.write2(b".", recv_handle, on_write2)


loop = pyuv.Loop.default_loop()

recv_handle = None

channel = pyuv.Pipe(loop, True)
channel.open(sys.stdin.fileno())
channel.start_read2(on_channel_read)

loop.run()

