#!/usr/bin/python

import time
import socket
import argparse
import typing
import signal
import sys

NETLOG_PORT = 8419
NOTIFY_PORT = 8419
TRY_CONN_INTERVAL_SECS = 1
START_KEEPALIVE_AFTER_SECS = 3
KEEPALIVE_INTERVAL_SECS = 1
KEEPALIVE_MAX_FAILED = 1

ap = argparse.ArgumentParser(
    description="listen to netlog"
)
ap.add_argument(
    "host",
    help="netlog host ip or name"
)
ap.add_argument(
    "-v",
    "--verbose",
    action="store_true",
    help="enable verbose log",
)
args = ap.parse_args()

def log(msg: str):
    msg = "  netlog.py: " + msg + "\n"
    sys.stdout.buffer.write(msg.encode())
    sys.stdout.buffer.flush()


def logv(msg: str):
    if args.verbose:
        log(msg)


def connect() -> socket.socket:
    log(f"waiting for netlog on {args.host}:{NETLOG_PORT} to accept connection...")
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, START_KEEPALIVE_AFTER_SECS)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, KEEPALIVE_INTERVAL_SECS)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, KEEPALIVE_MAX_FAILED)
            ip_addr = socket.gethostbyname(args.host)
            s.connect((ip_addr, NETLOG_PORT))
            log(f"connected to netlog on {args.host}:{NETLOG_PORT}, local port: {s.getsockname()[1]}")
            return s
        except OSError:
            time.sleep(TRY_CONN_INTERVAL_SECS)
        except Exception as e:
            fatal(e)


def reconnect(e: Exception, s: socket.socket) -> socket.socket:
    log(f"socket for {args.host}:{NETLOG_PORT} failed: {e}")
    try:
        s.shutdown(socket.SHUT_RDWR)
        s.close()
    except Exception as e:
        log(f"error closing socket: {e}")
    return connect()


def fatal(e: Exception) -> typing.NoReturn:
    log(f"fatal exception {e}")
    exit(1)


def sig_handler(signame, frame):
    if signame in [signal.SIGTERM, signal.SIGINT]:
        sys.stdout.buffer.write(b'\n')
        exit(0)


signal.signal(signal.SIGINT, sig_handler)
signal.signal(signal.SIGTERM, sig_handler)


log_sock = connect()
while True:
    try:
        logv("log recv...")
        text = log_sock.recv(4096)
        logv("log recv done")
    except OSError as e:
        log_sock = reconnect(e, log_sock)
        continue
    except Exception as e:
        fatal(e)

    if len(text) == 0:
        log_sock.close()
        break

    sys.stdout.buffer.write(text)
    sys.stdout.buffer.flush()

