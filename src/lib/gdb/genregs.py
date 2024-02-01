#!/usr/bin/python

gdb_pref = 'GDB_REG'
xtensa_pref = 'XCHAL'


def numregs(base: str, n: int) -> list[str]:
    return [f'{base}{i}' for i in range(n)]


groups = [
    ('BASE', [*numregs('A', 16), *numregs('CONFIGID', 2), 'PS', 'PC', 'SAR']),
    ('LOOPS', ['LBEG', 'LEND', 'LCOUNT']),
    ('WINDOWED', ['WINDOWBASE', 'WINDOWSTART']),
    ('THREADPTR', ['THREADPTR']),
    ('BOOLEANS', ['BR']),
    ('S32C1I', ['SCOMPARE1']),
    ('MAC16', ['ACCLO', 'ACCHI', *numregs('M', 4)]),
]

print("#pragma once")
print()
print("#include <xtensa/xtensa_context.h>")
print()

for i, r in enumerate(groups[0][1]):
    print(f"#define {gdb_pref}_{r} {i}")

print(f"#define _{gdb_pref}S0 {gdb_pref}_{groups[0][1][-1]}")
print()

for i, g in enumerate(groups[1:]):
    print(f"#if {xtensa_pref}_HAVE_{g[0]}")
    for j, r in enumerate(g[1]):
        print(f"\t#define {gdb_pref}_{r} (_{gdb_pref}S{i} + {j+1})")

    print(f"\t#define _{gdb_pref}S{i+1} {gdb_pref}_{g[1][-1]}")
    print("#else")
    print(f"\t#define _{gdb_pref}S{i+1} _{gdb_pref}S{i}")
    print(f"#endif // {xtensa_pref}_HAVE_{g[0]}")
    print()

print(f"#define {gdb_pref}S _{gdb_pref}S{len(groups)-1}")
