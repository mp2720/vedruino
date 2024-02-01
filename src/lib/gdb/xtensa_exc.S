#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#define __ASSEMBLY__
#include "hal.h"
#include "xtensa_regs.h"

.extern gdb_stack

.align 4
.type gdb_DebugExceptionVector, @function
.global gdb_DebugExceptionVector
.section .gdb_DebugExceptionVector.text, "ax"
gdb_DebugExceptionVector:
    wsr     a2, (EXCSAVE + XCHAL_DEBUGLEVEL)
    # Jump to handler 0.
    J       gdb_debug_exc0

.extern gdb_regfile, gdb_stack

#.literal_position
#.literal gdb_regfile_addr_cpu0, gdb_regfile
#.literal gdb_regfile_addr_cpu1, (gdb_regfile + GDB_REGS * 4)
#.literal gdb_stack_end, (gdb_stack + GDB_STACK_SIZE - 16)

.align 4
.type gdb_debug_exc0, @function
.global gdb_debug_exc0
.section .iram1.text, "ax"
gdb_debug_exc0:
    # Disable other interrupts.
    movi    a2, PS_INTLEVEL(7) | PS_UM | PS_WOE
    wsr     a2, PS

    # Get CPU id
    rsr     a2, PRID
    beqz    a2, 1f

    # CPU 1
    # l32r    a2, gdb_regfile_addr_cpu1
    movi    a2, (gdb_regfile + GDB_REGS * 4)
    j       2f

1:
    # CPU 0
    #l32r    a2, gdb_regfile_addr_cpu0
    movi    a2, gdb_regfile

2:
    # a2 is base of regfile
    # Before setting stack, we need save a little bit of regs that will be used.
    s32i    a0, a2, GDB_REG_A0
    s32i    a4, a2, GDB_REG_A4

    movi    a0, 0
    # sp is a1
    movi    sp, (gdb_stack + GDB_STACK_SIZE - 16)
    # a4 = sp + 32
    addi    a4, sp, 32
    s32e    a4, sp, -12
    
    # Call to the first function.
    call8   gdb_test_func

    # EPC contains PC of instruction that caused exception.
    

    # movi            a0, 0
    # wsr.intenable   a0

    #movi    a0, 0x1488
    #jx      a0
    #movi    a2, 68
    #call0   gdb_test_func
    #addi    a0, a0, 0x1488
    #rfi     6
    #movi    a0, 0
    #movi    sp, gdb_hnd_stack_cpu0 + 4096 - 16
    #addi    a4, sp, 32
    #s32e    a4, sp, -12
    #call4   gdb_hnd_debug_exc1
    #rfi     6
.loop:
    j       .loop

    #40147420


.align 4
.type gdb_test_func, @function
gdb_test_func:
    entry a1, 32
    nop.n
    retw.n


#.align 4
#.section .text
#.type gdb_set_vecbase, @function
#.global gdb_set_vecbase
#gdb_set_vecbase:
#    entry       a1, 32
#    l32r        a2, .vecbase_addr
##.L1:
##    wsr.vecbase a2
##    j .L1
#    retw.n
#.size gdb_set_vecbase, .-gdb_set_vecbase
#.literal_position
#.literal .vecbase_addr, _gdb_vecbase

#.align 4
#.section .text
#.type gdb_get_vecbase, @function
#.global gdb_get_vecbase
#gdb_get_vecbase:
#    j .L1
#    nop
#    nop
#    nop
#.L1:
#    j .L1
##    entry       a1, 32
##    rsr.litbase a2
##    retw.n
#.size gdb_get_vecbase, .-gdb_get_vecbase


#.align 4
#.section .iram0, "ax"
#.global xt_debugexception
#.type xt_debugexception, @function
#xt_debugexception:
#    j xt_debugexception


#.section .iram1, "ax"
#.global xt_highint5
#.type xt_highint5, @function
#.align 4
#xt_highint5:
#    j xt_highint5

#.extern _WindowOverflow4, _WindowUnderflow4, _WindowOverflow8, _WindowUnderflow8, _WindowOverflow12, _WindowUnderflow12
#.extern _Level2Vector, _Level3Vector, _Level4Vector, _Level5Vector
#.extern _DebugExceptionVector
#.extern _NMIExceptionVector
#.extern _KernelExceptionVector
#.extern _UserExceptionVector
#.extern _DoubleExceptionVector

#.macro PADDING64
#.align 64
#.byte 0
#
#.align 64
#.byte 0
#
#.align 64
#.byte 0
#.endm

#.align 256
#.global _gdb_vecbase
#.section .iram0, "ax"
#_gdb_vecbase:

#.align 256
#.type _gdb_WindowOverflow4, @function
#.global _gdb_WindowOverflow4 
#.section .iram0, "ax"
#_gdb_WindowOverflow4:
#    J _WindowOverflow4

#PADDING64

#.align 64
#.type _gdb_WindowUnderflow4, @function
#.global _gdb_WindowUnderflow4 
#.section .iram0, "ax"
#_gdb_WindowUnderflow4:
#    J _WindowUnderflow4
#
##PADDING64
#
#.align 64
#.type _gdb_WindowOverflow8, @function
#.global _gdb_WindowOverflow8 
#.section .iram0, "ax"
#_gdb_WindowOverflow8:
#    J _WindowOverflow8
#
##PADDING64
#
#.align 64
#.type _gdb_WindowUnderflow8, @function
#.global _gdb_WindowUnderflow8
#.section .iram0, "ax"
#_gdb_WindowUnderflow8:
#    J _WindowUnderflow8
#
##PADDING64
#
#.align 64
#.type _gdb_WindowOverflow12, @function
#.global _gdb_WindowOverflow12
#.section .iram0, "ax"
#_gdb_WindowOverflow12:
#    J _WindowOverflow12
#
##PADDING64
#
#.align 64
#.type _gdb_WindowUnderflow12, @function
#.global _gdb_WindowUnderflow12
#.section .iram0, "ax"
#_gdb_WindowUnderflow12:
#    J _WindowUnderflow12
#
##PADDING64
#
#.align 64
#.type _gdb_Level2Vector, @function
#.global _gdb_Level2Vector
#.section .iram0, "ax"
#_gdb_Level2Vector:
#    J _Level2Vector
#
##PADDING64
#
#.align 64
#.type _gdb_Level3Vector, @function
#.global _gdb_Level3Vector
#.section .iram0, "ax"
#_gdb_Level3Vector:
#    J _Level3Vector
#
##PADDING64
#
#.align 64
#.type _gdb_Level4Vector, @function
#.global _gdb_Level4Vector
#.section .iram0, "ax"
#_gdb_Level4Vector:
#    J _Level4Vector
#
##PADDING64
#
#.align 64
#.type _gdb_Level5Vector, @function
#.global _gdb_Level5Vector
#.section .iram0, "ax"
#_gdb_Level5Vector:
#    J _Level5Vector
#
##PADDING64
#
#.align 64
#.type _gdb_DebugExceptionVector, @function
#.global _gdb_DebugExceptionVector
#.section .iram0, "ax"
#_gdb_DebugExceptionVector:
#    J _gdb_DebugExceptionVector
#
##PADDING64
#
#.align 64
#.type _gdb_NMIExceptionVector, @function
#.global _gdb_NMIExceptionVector
#.section .iram0, "ax"
#_gdb_NMIExceptionVector:
#    J _NMIExceptionVector
#
##PADDING64
#
#.align 64
#.type _gdb_KernelExceptionVector, @function
#.global _gdb_KernelExceptionVector
#.section .iram0, "ax"
#_gdb_KernelExceptionVector:
#    J _KernelExceptionVector
#
##PADDING64
#
#.align 64
#.type _gdb_UserExceptionVector, @function
#.global _gdb_UserExceptionVector
#.section .iram0, "ax"
#_gdb_UserExceptionVector:
#    J _UserExceptionVector
#
##PADDING64
#
#.align 64
#.type _gdb_DoubleExceptionVector, @function
#.global _gdb_DoubleExceptionVector
#.section .iram0, "ax"
#_gdb_DoubleExceptionVector:
#    J _DoubleExceptionVector
#
