#include "gdb.h"

#include <string.h>

static void inf_lp();
asm(".type inf_lp, @function\n"
    ".align 4\n"
    "inf_lp:\n"
    "   entry sp, 32\n"
    "label1488:\n"
    "   j label1488\n"
    "   retw.n\n"
    ".size inf_lp, .-inf_lp\n");

static void test_breakpoint() {
    asm("break.n 0\n");
}

void gdb() {
    test_breakpoint();
}
