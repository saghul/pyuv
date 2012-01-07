#!/usr/bin/env python

import sys
sys.path.insert(0, '../')

import pyuv


TEST_PORT = 1234


def on_channel_write(handle, error):
    global channel, tcp_server
    channel.close()
    tcp_server.close()

def on_ipc_connection(handle, error):
    global channel, connection_accepted, loop, tcp_server
    if connection_accepted:
        return
    conn = pyuv.TCP(loop)
    tcp_server.accept(conn)
    conn.close()
    channel.write("accepted_connection", on_channel_write)
    connection_accepted = True


connection_accepted = False
listen_after_write = sys.argv[1] == "listen_after_write"

loop = pyuv.Loop.default_loop()

channel = pyuv.Pipe(loop, True)
channel.open(sys.stdin.fileno())

tcp_server = pyuv.TCP(loop)
tcp_server.bind(("0.0.0.0", TEST_PORT))
if not listen_after_write:
    tcp_server.listen(on_ipc_connection, 12)

channel.write2("hello", tcp_server)

if listen_after_write:
    tcp_server.listen(on_ipc_connection, 12)

loop.run()

