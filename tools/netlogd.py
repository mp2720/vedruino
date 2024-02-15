#!/usr/bin/python

import socket
import argparse
import struct
import threading
import daemonize
import time
import sys
import os

ap = argparse.ArgumentParser(
    description="subscribe and listen to netlog"
)
ap.add_argument(
    "-i",
    "--ip",
    dest="ip",
    action="store",
    help="ip of the board (you may provide --hostname instead)",
    required=True
)
ap.add_argument(
    '-n',
    '--hostname',
    dest="hostname",
    action="store",
    help="hostname of the board (you may provide --ip instead)"
)
ap.add_argument(
    "-p",
    "--port",
    dest="port",
    help="port of tcp netlog server on the board",
    required=True
)
args = ap.parse_args()

if (args.hostname is not None) == (args.ip is not None):
    print("provide only --ip or only --hostname")
    exit(1)

TCP_REQ_INTERVAL_SEC = 1*60

udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp.bind(('', 0))
udp_port = udp.getsockname()[1]

recv_lock = threading.Lock()
recv_flag = False

fifo = None
for i in range(16):
    try:
        fifo_path = f"/tmp/vnonetlog{i}"
        os.mkfifo(fifo_path)
        break
    except FileExistsError as e:
        continue

if fifo is None:
    print("all attempts to create fifo /tmp/vnonetlogN failed", file=sys.stderr)
    exit(1)


def write_fifo(data: bytes | str, py_pref=True):
    if type(data) is str:
        if py_pref:
            data = "netlog.py: " + data

        data = data.encode()

    fifo.write(data)
    fifo.flush()


def set_recv_flag(val: bool):
    global recv_flag
    recv_lock.acquire()
    recv_flag = val
    recv_lock.release()


def get_recv_flag() -> bool:
    recv_lock.acquire()
    val = recv_flag
    recv_lock.release()
    return val


def udp_client():
    last = 255
    while True:
        msg = udp.recv(512)
        set_recv_flag(True)
        i = msg[0]

        write_fifo(msg[1:], py_pref=False)
        if i != (last + 1) % 256:
            write_fifo("some packets arrived out of order, were missed or duplicated")
        last = i


def tcp_req(type: bytes) -> bytes | None:
    resp = None
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp:
            tcp.connect((args.ip, int(args.port)))
            tcp.sendall(b"NETLOG" + type + struct.pack("!H", udp_port))
            resp = tcp.recv(64)
            if resp == b"failed":
                write_fifo(f"server failed to process req {type.decode()}")
                resp = None
    except Exception as e:
        write_fifo(f"failed to send tcp req {type.decode()}: {e}")
    return resp


def tcp_client():
    while True:
        if get_recv_flag():
            time.sleep(TCP_REQ_INTERVAL_SEC)

        resp = tcp_req(b"STA")
        if resp == b"notsub":
            resp = tcp_req(b"SUB")
            if resp == b"ok":
                write_fifo(f"subscribed to netlog on {args.ip}")
                write_fifo(f"listening on port {udp_port}")
            elif resp is not None:
                write_fifo(f"unknown response to SUB req: {resp}")
        elif resp is not None:
            write_fifo(f"unknown response to STA req: {resp}")

        set_recv_flag(False)
        time.sleep(TCP_REQ_INTERVAL_SEC)


def daemon_main():
    threading.Thread(target=udp_client).run()
    tcp_client()


daemonize.Daemonize(app="vedruino-netlogd", pid="~/.vedruino-netlogd.pid", action=daemon_main,
                    keep_fds=[udp.fileno])
