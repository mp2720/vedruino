#!/usr/bin/python

"""
Simple C JSON string generator.
"""

import sys
import json


def escape_str(s: str) -> str:
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n')
    return s


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} CSTR_NAME")
    exit(1)


lines = []
for line in sys.stdin:
    lines.append(line)

cstr = f"const char {sys.argv[1]}[] = {{"
string = False
escape_in_str = False
for c in ''.join(lines):
    if c == '\n':
        if string or escape_in_str:
            print("malformed json", file=sys.stderr)
            exit(1)

        # else skip newline
        continue

    if c in ' \t\r' and not string:
        # skip blank if it's not inside string
        continue

    if escape_in_str:
        cstr += str(ord(c)) + ','
        escape_in_str = False
        continue

    if c == '"':
        string = not string
    
    if c == '\\' and string:
        escape_in_str = True

    cstr += str(ord(c)) + ','


cstr += '0};'


print("/* JSON")
print(*lines, sep='', end='')
print("*/")
print(cstr)

