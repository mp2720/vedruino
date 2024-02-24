#!/usr/bin/python

import sys
import subprocess

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} ADDR2LINE_PATH ELF_PATH; put text with backtrace to stdin", file=sys.stderr)
    exit(1)

PKBTRACE_PREF = "PKBTRACE "
PANIC_PREF = "Backtrace: "

addr2line_path = sys.argv[1]
elf_path = sys.argv[2]


class TraceRawEntry:
    def __init__(self, pc: int, sp: int):
        self.pc = pc
        self.sp = sp

    def __repr__(self) -> str:
        return f"0x{self.pc:02x}:0x{self.sp:02x}"


class TraceEntry:
    def __init__(self, raw: TraceRawEntry, func: str, file_line: str) -> None:
        self.raw = raw
        self.func = func
        self.file_line = file_line

    def __repr__(self) -> str:
        return repr(self.raw) + "\n   " + self.file_line + "\n   " + self.func


pkbtrace_raw_ents: list[TraceRawEntry] = []
panic: list[TraceRawEntry] = []


def run_addr2line(raw_entries: list[TraceRawEntry]) -> list[TraceEntry]:
    inp = '\n'.join([hex(e.pc) for e in raw_entries])
    inp = inp.encode()
    p = subprocess.run([addr2line_path, "-e" + elf_path, "-f", "-C"], capture_output=True, input=inp)
    lines = p.stdout.decode().split('\n')

    assert lines[-1] == ''
    del lines[-1]

    assert len(lines) % 2 == 0
    assert len(lines) // 2 == len(raw_entries)

    ret = []
    for i in range(0, len(lines), 2):
        ret.append(TraceEntry(raw_entries[i//2], lines[i], lines[i+1]))

    return ret


def dump_pkbtrace(finished: bool):
    global pkbtrace_raw_ents

    if len(pkbtrace_raw_ents) == 0:
        return

    entries = run_addr2line(pkbtrace_raw_ents)
    # Purple
    print("\033[0;95mPikeM backtrace" + (" (not finished)" if not finished else "") + " {")
    for e in entries:
        print(f" - {e}")
    print("}\033[0m")

    pkbtrace_raw_ents = []


try:
    for line in sys.stdin:
        line = line[:-1]
        if line.startswith(PKBTRACE_PREF):
            s = line[len(PKBTRACE_PREF):]
            if s == "END":
                dump_pkbtrace(True)
            elif s == "START":
                dump_pkbtrace(False)
            else:
                pc, sp = map(lambda x: int(x[2:], 16), s.split(':'))
                pkbtrace_raw_ents.append(TraceRawEntry(pc, sp))
        elif line.startswith(PANIC_PREF):
            dump_pkbtrace(False)
            s = line[len(PANIC_PREF):]
            panic = []
            extra_msg = ""
            for es in s.split(' '):
                if es == "|<-CORRUPTED":
                    extra_msg = " CORRUPTED"
                elif es == "|<-CONTINUES":
                    extra_msg = " CONTINUES"
                else:
                    pc, sp = map(lambda x: int(x[2:], 16), es.split(':'))
                    panic.append(TraceRawEntry(pc, sp))
    
            ents = run_addr2line(panic)
            # Purple underline
            print("\033[0;95mESP backtrace {" + extra_msg);
            for ent in ents:
                print(f" - {ent}")
            print("}\033[0m")
        else:
            print(line[:-1])
except KeyboardInterrupt:
    pass
