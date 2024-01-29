#!/usr/bin/python

"""

"""

from dataclasses import dataclass
from enum import Enum
import struct


class ElfBits(Enum):
    Bits32 = 1
    Bits64 = 2


class ElfEndiannes(Enum):
    LittleEndian = 1
    BigEndian = 2


class ElfType(Enum):
    Relocatable = 1
    Executable = 2
    Shared = 3
    Core = 4


@dataclass
class ElfHeader:
    magic: bytes
    bits: ElfBits
    endiannes: ElfEndiannes
    header_version: int
    abi: int
    type_: ElfType
    isa: int
    elf_version: int
    entry_pos: int
    prog_header_tab_pos: int
    section_header_tab_pos: int
    flags: int
    header_size: int
    prog_header_tab_entry_size: int
    prog_header_tab_entries: int
    section_header_tab_entry_size: int
    section_header_tab_entries: int
    section_header_names_index: int

    def __repr__(self) -> str:
        return f"magic: {self.magic}\n" +\
        f"bits: {self.bits.name}\n" +\
        f"endiannes: {self.endiannes.name}\n" +\
        f"header_version: {self.header_version}\n" +\
        f"abi: {self.abi}\n" +\
        f"type_: {self.type_.name}\n" +\
        f"isa: {self.isa}\n" +\
        f"elf_version: {self.elf_version}\n" +\
        f"entry_pos: {hex(self.entry_pos)}\n" +\
        f"prog_header_tab_pos: {hex(self.prog_header_tab_pos)}\n" +\
        f"section_header_tab_pos: {hex(self.section_header_tab_pos)}\n" +\
        f"flags: {hex(self.flags)}\n" +\
        f"header_size: {self.header_size}\n" +\
        f"prog_header_tab_entry_size: {self.prog_header_tab_entry_size}\n" +\
        f"prog_header_tab_entries: {self.prog_header_tab_entries}\n" +\
        f"section_header_tab_entry_size: {self.section_header_tab_entry_size}\n" +\
        f"section_header_tab_entries: {self.section_header_tab_entries}\n" +\
        f"section_header_names_index: {self.section_header_names_index}"


def read_header(file) -> ElfHeader:
    magic = file.read(4)
    assert magic == b'\x7fELF', f'invalid ELF magic {magic}'

    b = file.read(48)
    tup = struct.unpack("<BBBB8x HHIIIIIHHHHHH", b)
    header = ElfHeader(magic, *tup)
    header.bits = ElfBits(header.bits)
    header.endiannes = ElfEndiannes(header.endiannes)
    header.type_ = ElfType(header.type_)

    return header


class ElfSegmentType(Enum):
    Null = 0
    Load = 1
    Dynamic = 2
    Interp = 3
    Note = 4


class ElfSegmentFlag(Enum):
    Executable = 1
    Writable = 2
    Readable = 4


@dataclass
class ElfSegment:
    type_: ElfSegmentType
    file_offset: int
    virt_addr: int
    undefined: int
    file_size: int
    mem_size: int
    flags: list[ElfSegmentFlag]
    alignment: int

    def __repr__(self) -> str:
        return f"type: {self.type_.name}\n" +\
    f"file_offset: {hex(self.file_offset)}\n" +\
    f"virt_addr: {hex(self.virt_addr)}\n" +\
    f"undefined: {hex(self.undefined)}\n" +\
    f"file_size: {self.file_size}\n" +\
    f"mem_size: {self.mem_size}\n" +\
    f"flags: {', '.join(map(lambda x: x.name, self.flags))}\n" +\
    f"alignment: {self.alignment}"


def read_segment(seg_size: int, file) -> ElfSegment:
    fmt = "<IIIIIIII"
    padding = seg_size - struct.calcsize(fmt)
    assert padding >= 0, "ELF segment size is too small"

    fmt += "x" * padding
    tup = struct.unpack(fmt, file.read(seg_size))

    seg = ElfSegment(*tup)
    seg.type_ = ElfSegmentType(seg.type_)

    flags = []
    for flag in ElfSegmentFlag:
        if flag.value & seg.__dict__["flags"]:
            flags.append(flag)

    seg.flags = flags
    return seg


def add_tab_on_lines(s: str) -> str:
    lines = s.split("\n")
    return '\n'.join(map(lambda x: '\t' + x, lines))


@dataclass
class ElfFile:
    header: ElfHeader
    segments: list[ElfSegment]

    def __repr__(self) -> str:
        segs_str = []
        for seg in self.segments:
            segs_str.append(add_tab_on_lines(str(seg) + '\n---'))

        return f"header: {self.header}\n" +\
                f"segments:\n{''.join(segs_str)}"


def read_elf(file) -> ElfFile:
    header = read_header(file)

    file.seek(header.prog_header_tab_pos)
    segments = []
    for _ in range(header.prog_header_tab_entries):
        segments.append(read_segment(header.prog_header_tab_entry_size, file))

    return ElfFile(header, segments)


if __name__ == "__main__":
    f = open("build/vedruino.ino.elf", "rb")
    e = read_elf(f)
    print(e)
