#!/usr/bin/python

import time
import socket
import traceback
import subprocess
import argparse
import select
import struct
import signal
import sys

CTL_PORT = 8419
BCAST_PORT = 8419

MAGIC = b"2720NETLOG"

# all time values in seconds

TCP_LOG_START_KEEPALIVE_AFTER = 3
TCP_LOG_KEEPALIVE_INTERVAL = 2
TCP_LOG_KEEPALIVE_MAX_FAILED = 2

RECV_MAX_SIZE = 1024

# ctl tcp
CTL_TRY_CONN_INTERVAL = 1
CTL_TIMEOUT = 1


def stdout_write(bs: bytes):
    sys.stdout.buffer.write(bs)
    sys.stdout.buffer.flush()


def log(msg: str):
    # purple
    msg = "\033[0;95m" + msg + "\033[0m\n"
    bs = msg.encode()
    sys.stderr.buffer.write(bs)
    sys.stderr.buffer.flush()


def logv(msg: str):
    msg = "(verbose): " + msg
    if args.verbose:
        log(msg)


class BoardInfo:
    MAC_ADDR_SIZE = 6
    SESSION_ID_SIZE = 32
    SIZE = MAC_ADDR_SIZE + SESSION_ID_SIZE

    def __init__(self, mac_addr: bytes, session_id: bytes):
        self.mac_addr = mac_addr
        assert len(self.mac_addr) == BoardInfo.MAC_ADDR_SIZE
        self.session_id = session_id
        assert len(self.session_id) == BoardInfo.SESSION_ID_SIZE

    @classmethod
    def from_bytes(cls, bs: bytes):
        fmt = f"!{cls.MAC_ADDR_SIZE}s{cls.SESSION_ID_SIZE}s"
        return BoardInfo(*struct.unpack(fmt, bs))

    def __repr__(self) -> str:
        m_hex = self.mac_addr.hex().upper()
        sid_hex = self.session_id.hex()
        m_str = ':'.join([m_hex[i:i+2] for i in range(0, 12, 2)])
        return f"MAC: {m_str}, session ID: {sid_hex}"

    @classmethod
    def bcast_filter(cls, cur, bcast) -> bool:
        if cur.mac_addr != bcast.mac_addr:
            return False

        return cur.session_id != bcast.session_id


def handle_ctl_resp(s: socket.socket) -> BoardInfo:
    bs = s.recv(1)
    if len(bs) != 1:
        raise Exception("cannot get status from server's response")

    if bs[0] == ord('-'):
        msg = b''
        while len(bs := s.recv(RECV_MAX_SIZE)) != 0:
            msg += bs

        raise Exception(f"server responded: error `{msg.decode()}`")

    if bs[0] != ord('+'):
        raise Exception(f"server responded with unknown status `{bs[0]}`")

    board_info_bs = b''
    while len(board_info_bs) < BoardInfo.SIZE:
        bs = s.recv(RECV_MAX_SIZE)
        if len(bs) == 0:
            raise Exception(f"server's response is too small: `{board_info_bs}`")
        board_info_bs += bs

    return BoardInfo.from_bytes(board_info_bs)


def ctl_sub():
    log(f"waiting for {args.host}:{CTL_PORT} server to accept connection")

    ctl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ctl_sock.settimeout(CTL_TIMEOUT)

    proto = b"tcp" if args.tcp else b"udp"
    req = MAGIC + proto + struct.pack("!H", log_port) + b"S"

    while True:
        try:
            ip_addr = socket.gethostbyname(args.host)
            logv(f"ip of netlog server: {ip_addr}")

            ctl_sock.connect((ip_addr, CTL_PORT))
            logv("socket connected to ctl server")
        except OSError:
            logv("failed to connect socket, waiting to retry")
            time.sleep(CTL_TRY_CONN_INTERVAL)
            continue

        logv(f"sending sub req {req} to netlog ctl")
        ctl_sock.sendall(req)

        global board_info
        board_info = handle_ctl_resp(ctl_sock)
        logv(f"server is running on board {board_info}")

        ctl_sock.close()
        log(f"connected to {args.host}:{CTL_PORT} (ip {ip_addr})")

        logv(f"listening {proto.decode()} local port {log_port} for logs")
        return


def ctl_unsub():
    ctl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ctl_sock.settimeout(CTL_TIMEOUT)
    ip_addr = socket.gethostbyname(args.host)
    ctl_sock.connect((ip_addr, CTL_PORT))
    logv(f"connected socket to {args.host}:{CTL_PORT} (ip {ip_addr})")

    proto = b"tcp" if args.tcp else b"udp"
    req = MAGIC + proto + struct.pack("!H", int(args.unsub_port)) + b"U"
    logv(f"sending unsub request {req} to server")
    ctl_sock.sendall(req)

    board_info = handle_ctl_resp(ctl_sock)
    logv(f"board info: {board_info}")


def parse_args():
    ap = argparse.ArgumentParser(
        description="netlog client"
    )
    ap.add_argument(
        "host",
        help="board hostname or ip"
    )
    ap.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="verbose mode",
    )
    ap.add_argument(
        "-t",
        "--tcp",
        action="store_true",
        help="receive logs using tcp (if not set, udp is used)",
    )
    ap.add_argument(
        "-k",
        "--no-keepalive",
        action="store_true",
        help="disable keepalive on receiving logs (use only with --tcp flag)"
    )
    ap.add_argument(
        '-u',
        '--unsub',
        dest="unsub_port",
        action="store",
        help="send unsub request and exit"
    )
    a = ap.parse_args()

    if a.no_keepalive and not a.tcp:
        raise Exception("--no-keepalive can be used only with --tcp")

    if a.unsub_port is not None and a.no_keepalive:
        raise Exception("--no-keepalive option is not allowed with --unsub")

    return a


def handle_bcast(bs: bytes, addr_port: tuple[str, int]):
    logv(f"got broadcast boot message: {bs}")
    try:
        if len(bs) < len(MAGIC) or bs[:len(MAGIC)] != MAGIC:
            raise Exception("invalid magic")

        bcast_board_info = BoardInfo.from_bytes(bs[len(MAGIC):])
    except Exception as e:
        logv(f"error reading boot message: {e}")
        logv(f"boot message from {addr_port[0]}:{addr_port[1]}: {bs}")
        log("extraneous boot message ignored")
        return

    logv(f"boot message from {addr_port[0]}:{addr_port[1]} with info: {bcast_board_info}")

    global board_info
    if not BoardInfo.bcast_filter(board_info, bcast_board_info):
        logv(f"boot message from {addr_port[0]}:{addr_port[1]} ignored by MAC/session ID filter")
        return

    board_info = bcast_board_info

    if args.tcp:
        if 'tcp_log' in socks:
            del socks['tcp_log']
    else:
        global udp_pack_cnt
        udp_pack_cnt = 127

    ctl_sub()


args = parse_args()

board_info: BoardInfo
log_port: int = -1
udp_pack_cnt: int

socks: dict[str, socket.socket] = {}


def main_loop():
    for s in select.select(socks.values(), [], [])[0]:
        if s == socks['bcast']:
            bs, addr_port = s.recvfrom(len(MAGIC) + BoardInfo.SIZE)
            handle_bcast(bs, addr_port)
        elif args.tcp and s == socks['tcp_serv']:
            tcp_log_sock, addr = s.accept()
            if 'tcp_log' in socks:
                log(f"tcp connection from {addr[0]}:{addr[1]} ignored")
                continue

            if not args.no_keepalive:
                tcp_log_sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
                tcp_log_sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, TCP_LOG_START_KEEPALIVE_AFTER)
                tcp_log_sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, TCP_LOG_KEEPALIVE_INTERVAL)
                tcp_log_sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, TCP_LOG_KEEPALIVE_MAX_FAILED)

            socks['tcp_log'] = tcp_log_sock

            log(f"{addr[0]}:{addr[1]} connected to local tcp socket")
        elif args.tcp and s == socks['tcp_log']:
            try:
                bs = s.recv(RECV_MAX_SIZE)
                if len(bs) == 0:
                    raise Exception("socket closed by server")
            except OSError as e:
                try:
                    s.close()
                except:
                    # бывает
                    pass

                log(f"socket closed: {e}")
                log(f"waiting for boot message")
                del socks['tcp_log']
                continue

            stdout_write(bs) 
        elif not args.tcp and s == socks['udp']:
            bs, _ = s.recvfrom(RECV_MAX_SIZE)
            cnt = bs[0] & 0x7f
            
            stdout_write(bs[1:])

            global udp_pack_cnt
            if cnt != (udp_pack_cnt + 1) & 0x7f:
                log(f"package {udp_pack_cnt} arrived out of order")

            udp_pack_cnt = cnt


def fork_for_unsub():
    if log_port < 0:
        return

    unsub_args = [sys.argv[0], args.host, f'-u{log_port}']
    if args.tcp:
        unsub_args.append('-t')

    logv("forked for unsub")
    subprocess.Popen(unsub_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                     start_new_session=True)


def sig_handler(signame, _):
    if signame in [signal.SIGINT, signal.SIGTERM]:
        fork_for_unsub()
        stdout_write(b'\033[0m\n')
        exit(0)


def main():
    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    bcast_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    bcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    bcast_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    bcast_sock.bind(('', BCAST_PORT))
    socks['bcast'] = bcast_sock

    global log_port
    if args.tcp:
        tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


        tcp_sock.bind(('', 0))
        tcp_sock.listen(1)

        socks['tcp_serv'] = tcp_sock
        log_port = tcp_sock.getsockname()[1]
    else:
        udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_sock.bind(('', 0))

        socks['udp'] = udp_sock
        log_port = udp_sock.getsockname()[1]

        global udp_pack_cnt
        udp_pack_cnt = 127
    
    ctl_sub()

    while True:
        main_loop()


if args.unsub_port:
    try:
        ctl_unsub()
        exit(0)
    except Exception as e:
        log("failed to unsub")
        traceback.print_exc()
        exit(1)

try:
    main()
except Exception as e:
    log("fatal error")
    traceback.print_exc()
    fork_for_unsub()
    exit(1)

