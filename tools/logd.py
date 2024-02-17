#!/usr/bin/python

import socket
import argparse
import signal
import serial
import threading
import daemonize
import time
import sys
import os
import typing

IPC_SOCK_PATH_DEFAULT = '/tmp/pk-netlogd'
NETLOG_PORT = 8419
CONNECT_ATTEMPT_INTERVAL_SECS = 10

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
    "-s",
    "--serial-port",
    dest="serial_port",
    action="store",
    help="path to serial port",
)
ap.add_argument(
    "-p",
    "--ipc-sock",
    dest="ipc_sock",
    action="store",
    help=f"path to ipc sock (default is {IPC_SOCK_PATH_DEFAULT}N, where N >= 0)",
)
ap.add_argument(
    "-v",
    "--verbose",
    dest="verbose",
    action="store_true"
)
ap.add_argument(
    "-f",
    "--foreground",
    dest="foreground",
    action="store_true",
    help="do not daemonize (for debug) and write ipc sock data to stdout"
)
args = ap.parse_args()


class LogReader:
    def readbyte(self) -> bytes:
        raise NotImplementedError()

    def try_open(self) -> bool:
        raise NotImplementedError()

    def close(self):
        raise NotImplementedError()

    def is_active(self) -> bool:
        raise NotImplementedError()


class SerialLogReader(LogReader):
    def __init__(self, path: str) -> None:
        self.path = path
        self.sp: serial.Serial | None = None

    def readbyte(self) -> bytes:
        assert self.sp is not None
        return self.sp.read(1)

    def try_open(self) -> bool:
        try:
            self.sp = serial.Serial(self.path, 115200)
            ipc_log(f"connected to serial port on {self.path}")
            return True
        except Exception as e:
            ipc_log(f"failed to connect to serial port on {self.path}: {e}")
            return False

    def close(self):
        assert self.sp is not None
        self.sp.close()
        self.sp = None
        ipc_log(f"serial port {self.path} closed")

    def is_active(self) -> bool:
        return self.sp is not None


class NetlogReader(LogReader):
    def __init__(self, ip_addr: str) -> None:
        self.ip_addr = ip_addr
        self.sock: socket.socket | None = None

    def readbyte(self) -> bytes:
        assert self.sock is not None
        return self.sock.recv(1)

    def try_open(self) -> bool:
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.ip_addr, NETLOG_PORT))
            ipc_log(f"connected to netlog on {self.ip_addr}:{NETLOG_PORT}")
            return True
        except Exception as e:
            ipc_log(f"failed to connect to netlog on {self.ip_addr}:{NETLOG_PORT}: {e}")
            return False

    def close(self):
        assert self.sock is not None
        self.sock.close()
        self.sock = None
        ipc_log(f"socket for {self.ip_addr}:{NETLOG_PORT} closed")

    def is_active(self) -> bool:
        return self.sock is not None
        

log_reader_lock = threading.Lock()
if args.serial_port is not None:
    log_reader = SerialLogReader(args.serial_port)
elif args.ip is not None or args.hostname is not None:
    log_reader = NetlogReader(args.ip)
else:
    print("provide exactly one of the following arguments: --serial_port, --ip, --hostname",
          file=sys.stderr)
    exit(1)

if args.ipc_sock is not None:
    ipc_sock_path = args.ipc_sock
else:
    i = 0
    while True:
        ipc_sock_path = f"{IPC_SOCK_PATH_DEFAULT}{i}"
        if not os.path.exists(ipc_sock_path):
            break

        print(f"ipc socket {ipc_sock_path} already exists, trying next...")
        i += 1

ipc_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
ipc_sock.bind(ipc_sock_path)

cl_ipc_socks: list[socket.socket] = []
cl_ipc_socks_lock = threading.Lock()


def ipc_listener():
    ipc_sock.listen(8)
    ipc_vlog("ipc server started")
    while True:
        cl_sock, _ = ipc_sock.accept()
        cl_ipc_socks_lock.acquire()
        cl_ipc_socks.append(cl_sock)
        cl_ipc_socks_lock.release()


def sig_handler(sig, frame):
    if sig == signal.SIGTERM or sig == signal.SIGINT:
        ipc_sock.close()
        # os.unlink(ipc_sock_path)
        exit(0)
    elif sig == signal.SIGSTOP:
        if log_reader.is_active():
            log_reader.close()
    elif sig == signal.SIGCONT:
        if not log_reader.is_active():
            log_reader.try_open()


def ipc_write(b: bytes):
    del_idxs = []
    for i, cl_ipc_sock in enumerate(cl_ipc_socks):
        try:
            cl_ipc_sock.send(b)
        except:
            del_idxs.append(i)

    for i in del_idxs:
        del cl_ipc_socks[i]


def ipc_log(msg: str):
    b = b"\033[2;30;47m" + msg.encode() + b"\033[0m\n"
    ipc_write(b)
    if args.foreground:
        sys.stdout.buffer.write(b)
        sys.stdout.flush()


def ipc_vlog(msg: str):
    if args.verbose:
        ipc_log(msg)


def daemon_main():
    global ipc_listener_thread

    ipc_listener_thread = threading.Thread(target=ipc_listener)
    ipc_listener_thread.start()
    ipc_listener_thread.join

    # signal.signal(signal.SIGTERM, sig_handler)
    # signal.signal(signal.SIGSTOP, sig_handler)
    # signal.signal(signal.SIGCONT, sig_handler)
    signal.signal(signal.SIGINT, sig_handler)

    ipc_vlog("log receiving loop started")
    while True:
        if not log_reader.is_active():
            if not log_reader.try_open():
                time.sleep(CONNECT_ATTEMPT_INTERVAL_SECS)
                continue

        log_reader_lock.acquire()
        ipc_vlog("waiting for log messages from board")
        b = log_reader.readbyte()
        log_reader_lock.release()

        ipc_write(b)


if args.foreground:
    daemon_main()
else:
    daemon = daemonize.Daemonize(app="pk-netlogd", pid="~/.pk-netlogd.pid", action=daemon_main)
    daemon.start()

