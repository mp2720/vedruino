#!/usr/bin/python

import serial
import argparse
import sys

ap = argparse.ArgumentParser(
    description="serial monitor"
)
ap.add_argument(
    "port",
    help="path to serial port"
)
ap.add_argument(
    "-b",
    "--bitrate",
    action="store",
    help="optional bitrate (default is 115200)"
)
ap.add_argument(
    '-r',
    '--cr',
    action="store_true",
    help="do not convert CRLF to LF"
)
args = ap.parse_args()

if args.bitrate is None:
    bitrate = 115200
else:
    bitrate = int(args.bitrate)

s = serial.Serial(args.port, bitrate)

prev = b''
try:
    while True:
        c = s.read(1)
        if not args.cr:
            if prev == b'\r' and c != b'\n':
                sys.stdout.buffer.write(prev)

        prev = c

        if not args.cr:
            if c == b'\r':
                continue

        sys.stdout.buffer.write(c)
        sys.stdout.flush()
except KeyboardInterrupt:
    pass
