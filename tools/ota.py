#!/usr/bin/python

import argparse
import socket
import hashlib
import struct

ap = argparse.ArgumentParser(
    description="send firmware binary to OTA TCP server running on board"
)
ap.add_argument(
    "-i",
    "--ip",
    dest="ip",
    action="store",
    help="ip of the board (optional)",
    required=True
)
ap.add_argument(
    "-p",
    "--port",
    dest="port",
    help="port of tcp server on the board",
    required=True
)
ap.add_argument(
    "file",
    help="path to firmware binary file",
)
args = ap.parse_args()

with open(args.file, "rb") as f:
    firmware = f.read()
    md5h = hashlib.md5(firmware)
    md5 = md5h.digest()
    md5str = md5h.hexdigest()

MAGIC = b"TCPOTAUPDATE"
size = struct.pack("!I", len(firmware))

print(f"size={len(firmware)}, md5={md5str}")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((args.ip, int(args.port)))
    s.sendall(MAGIC + md5 + size + firmware)
    resp = s.recv(128)
    print(f"server responded: {resp}")

