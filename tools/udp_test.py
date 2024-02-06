#!/usr/bin/python

import socket
import signal

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('', 1498))
total = 10000
last = -1
outoford = 0
dup = 0
mp = [False] * total
print("listening on hohlokost:1498 (Ctrl-C to stop)")

def signal_handler(sig, frame):
    recv = mp.count(True)
    print(f"received {recv} of {total} packets ({recv/total*100}%), {total-recv} missed")
    print(f"{outoford} out of order, {dup} duplicates")
    exit(0)

signal.signal(signal.SIGINT, signal_handler)

while True:
    data, _ = s.recvfrom(8192)
    i = int(data[:9])
    print(f"recv {i}")
    if last > i:
        print("out of order")
        outoford += 1
    if mp[i]:
        print("dup")
        dup += 1
    else:
        mp[i] = True
    last = i

