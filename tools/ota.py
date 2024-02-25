#!/usr/bin/python

import argparse
import socket
import hashlib
import struct

ap = argparse.ArgumentParser(
    description="send firmware binary to OTA TCP server running on board"
)
ap.add_argument(
    "file",
    help="path to firmware binary file",
)
ap.add_argument(
    "host",
    help="board hostname or ip"
)
args = ap.parse_args()

with open(args.file, "rb") as f:
    firmware = f.read()
    md5h = hashlib.md5(firmware)
    md5 = md5h.digest()
    md5str = md5h.hexdigest()


BLK_SIZE = 8192
""" Размер блока может быть любым """
MAGIC = b"2720OTAUPDATE"
SERVER_PORT = 5256

size = len(firmware)
print(f"size={size}, md5={md5str}")

header = MAGIC + struct.pack("!I", size) + md5

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ip_addr = socket.gethostbyname(args.host)
print(f"ip addr: {ip_addr}")
sock.connect((ip_addr, SERVER_PORT))
sock.sendall(header)

sent = 0
try:
    while sent < size:
        blk = firmware[sent:min(sent + BLK_SIZE, size)]
        sock.sendall(blk)
        sent += len(blk)
        print(f"{int(100*sent/size)}%\r", end='')
except ConnectionResetError:
    pass

print()

resp = b''
while True:
    bs = sock.recv(1024)
    if len(bs) == 0:
        break
    resp += bs

print(resp)
assert len(resp) >= 1, "malformed response"
if resp[0] == ord('-'):

    print(f"error, server responded: {resp[1:].decode()}")
elif resp[0] == ord('+'):
    print("ok")
else:
    assert False, f"malformed response {resp}"

