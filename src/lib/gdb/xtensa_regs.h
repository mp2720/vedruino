#pragma once

#include <xtensa/xtensa_context.h>

#define GDB_REG_A0 0
#define GDB_REG_A1 1
#define GDB_REG_A2 2
#define GDB_REG_A3 3
#define GDB_REG_A4 4
#define GDB_REG_A5 5
#define GDB_REG_A6 6
#define GDB_REG_A7 7
#define GDB_REG_A8 8
#define GDB_REG_A9 9
#define GDB_REG_A10 10
#define GDB_REG_A11 11
#define GDB_REG_A12 12
#define GDB_REG_A13 13
#define GDB_REG_A14 14
#define GDB_REG_A15 15
#define GDB_REG_CONFIGID0 16
#define GDB_REG_CONFIGID1 17
#define GDB_REG_PS 18
#define GDB_REG_PC 19
#define GDB_REG_SAR 20
#define _GDB_REGS0 GDB_REG_SAR

#if XCHAL_HAVE_LOOPS
	#define GDB_REG_LBEG (_GDB_REGS0 + 1)
	#define GDB_REG_LEND (_GDB_REGS0 + 2)
	#define GDB_REG_LCOUNT (_GDB_REGS0 + 3)
	#define _GDB_REGS1 GDB_REG_LCOUNT
#else
	#define _GDB_REGS1 _GDB_REGS0
#endif // XCHAL_HAVE_LOOPS

#if XCHAL_HAVE_WINDOWED
	#define GDB_REG_WINDOWBASE (_GDB_REGS1 + 1)
	#define GDB_REG_WINDOWSTART (_GDB_REGS1 + 2)
	#define _GDB_REGS2 GDB_REG_WINDOWSTART
#else
	#define _GDB_REGS2 _GDB_REGS1
#endif // XCHAL_HAVE_WINDOWED

#if XCHAL_HAVE_THREADPTR
	#define GDB_REG_THREADPTR (_GDB_REGS2 + 1)
	#define _GDB_REGS3 GDB_REG_THREADPTR
#else
	#define _GDB_REGS3 _GDB_REGS2
#endif // XCHAL_HAVE_THREADPTR

#if XCHAL_HAVE_BOOLEANS
	#define GDB_REG_BR (_GDB_REGS3 + 1)
	#define _GDB_REGS4 GDB_REG_BR
#else
	#define _GDB_REGS4 _GDB_REGS3
#endif // XCHAL_HAVE_BOOLEANS

#if XCHAL_HAVE_S32C1I
	#define GDB_REG_SCOMPARE1 (_GDB_REGS4 + 1)
	#define _GDB_REGS5 GDB_REG_SCOMPARE1
#else
	#define _GDB_REGS5 _GDB_REGS4
#endif // XCHAL_HAVE_S32C1I

#if XCHAL_HAVE_MAC16
	#define GDB_REG_ACCLO (_GDB_REGS5 + 1)
	#define GDB_REG_ACCHI (_GDB_REGS5 + 2)
	#define GDB_REG_M0 (_GDB_REGS5 + 3)
	#define GDB_REG_M1 (_GDB_REGS5 + 4)
	#define GDB_REG_M2 (_GDB_REGS5 + 5)
	#define GDB_REG_M3 (_GDB_REGS5 + 6)
	#define _GDB_REGS6 GDB_REG_M3
#else
	#define _GDB_REGS6 _GDB_REGS5
#endif // XCHAL_HAVE_MAC16

#define GDB_REGS _GDB_REGS6
