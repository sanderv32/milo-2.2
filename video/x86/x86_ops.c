/* 
 * Copyright (c) 1995 Free Software Foundation.
 *
 * The file is part of x86emu.
 * 
 * x86emu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * x86emu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with x86emu; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  
 */
/*
 * There are approximately 250 subroutines in here, which correspond
 * to the 256 byte-"opcodes" found on the 8086.  The table which
 * dispatches this is found in the files optab.[ch].
 *
 * Each opcode proc has a comment preceeding it which gives it's table
 * address.  Several opcodes are missing (undefined) in the table.
 *
 * Each proc includes information for decoding (DECODE_PRINTF and
 * DECODE_PRINTF2), debugging (TRACE_REGS, SINGLE_STEP), and misc
 * functions (START_OF_INSTR, END_OF_INSTR).
 *
 * Many of the procedures are *VERY* similar in coding.  This has
 * allowed for a very large amount of code to be generated in a fairly
 * short amount of time (i.e. cut, paste, and modify).  The result is
 * that much of the code below could have been folded into subroutines
 * for a large reduction in size of this file.  The downside would be
 * that there would be a penalty in execution speed.  The file could
 * also have been *MUCH* larger by inlining certain functions which
 * were called.  This could have resulted even faster execution.  The
 * prime directive I used to decide whether to inline the code or to
 * modularize it, was basically: 1) no unnecessary subroutine calls,
 * 2) no routines more than about 200 lines in size, and 3) modularize
 * any code that I might not get right the first time.  The fetch_*
 * subroutines fall into the latter category.  The The decode_* fall
 * into the second category.  The coding of the "switch(mod){ .... }"
 * in many of the subroutines below falls into the first category.
 * Especially, the coding of {add,and,or,sub,...}_{byte,word}
 * subroutines are an especially glaring case of the third guideline.
 * Since so much of the code is cloned from other modules (compare
 * opcode #00 to opcode #01), making the basic operations subroutine
 * calls is especially important; otherwise mistakes in coding an
 * "add" would represent a nightmare in maintenance.
 *
 * So, without further ado, ...
 */
#include "x86_regs.h"
#include "x86_ops.h"
#include "x86_prim_ops.h"
#include "x86_decode.h"
#include "x86_debug.h"
#include "x86_fpu.h"
#include "x86_bios.h"


static inline int
xorl(int a, int b)
{
    return (a && !b) || (!a && b);
}


void
x86op_illegal_op (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ILLEGAL 8086 OPCODE\n");
    TRACE_REGS(m);
    halt_sys(m);
    END_OF_INSTR(m);
}


void
x86op_long_jump(SysEnv *m, u8 op2)
{
    s32 target;
    char *name = 0;
    int cond = 0;

    /* conditional jump to word offset. */
    START_OF_INSTR(m);
    switch (op2) {
      case 0x80:
	name = "JO\t";
	cond =  ACCESS_FLAG(m, F_OF);
	break;
      case 0x81:
	name = "JNO\t";
	cond = !ACCESS_FLAG(m, F_OF);
	break;
      case 0x82:
	name = "JB\t";
	cond = ACCESS_FLAG(m, F_CF);
	break;
      case 0x83:
	name = "JNB\t";
	cond = !ACCESS_FLAG(m, F_CF);
	break;
      case 0x84:
	name = "JZ\t";
	cond = ACCESS_FLAG(m, F_ZF);
	break;
      case 0x85:
	name = "JNZ\t";
	cond = !ACCESS_FLAG(m, F_ZF);
	break;
      case 0x86:
	name = "JBE\t";
	cond = ACCESS_FLAG(m, F_CF) || ACCESS_FLAG(m, F_ZF);
	break;
      case 0x87:
	name = "JNBE\t";
	cond = !(ACCESS_FLAG(m, F_CF) || ACCESS_FLAG(m, F_ZF));
	break;
      case 0x88:
	name = "JS\t";
	cond = ACCESS_FLAG(m, F_SF);
	break;
      case 0x89:
	name = "JNS\t";
	cond = !ACCESS_FLAG(m, F_SF);
	break;
      case 0x8a:
	name = "JP\t";
	cond = ACCESS_FLAG(m, F_PF);
	break;
      case 0x8b:
	name = "JNP\t";
	cond = !ACCESS_FLAG(m, F_PF);
	break;
      case 0x8c:
	name = "JL\t";
	cond = xorl(ACCESS_FLAG(m, F_SF), ACCESS_FLAG(m, F_OF));
	break;
      case 0x8d:
	name = "JNL\t";
	cond = xorl(ACCESS_FLAG(m, F_SF), ACCESS_FLAG(m, F_OF));
	break;
      case 0x8e:
	name = "JLE\t";
	cond = (xorl(ACCESS_FLAG(m, F_SF), ACCESS_FLAG(m, F_OF)) ||
		ACCESS_FLAG(m, F_ZF));
	break;
      case 0x8f:
	name = "JNLE\t";
	cond = !(xorl(ACCESS_FLAG(m, F_SF), ACCESS_FLAG(m, F_OF)) ||
		 ACCESS_FLAG(m, F_ZF));
	break;
    }
    DECODE_PRINTF(m, name);
    target = (s16) fetch_word_imm(m);
    target += (s16) m->x86.R_IP;
    DECODE_PRINTF2(m, "%04x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (cond)
      m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x00 */
void
x86op_add_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\t");

    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x01 */
void
x86op_add_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = add_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x02 */
void
x86op_add_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x03 */
void
x86op_add_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = add_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x04 */
void
x86op_add_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = add_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x05 */
void
x86op_add_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADD\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%d\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = add_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x06 */
void
x86op_push_ES (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tES\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_ES);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x07 */
void
x86op_pop_ES (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tES\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_ES = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x08 */
void
x86op_or_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}

/*opcode=0x09 */
void
x86op_or_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = or_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_word(m, *destreg, *srcreg);
	break;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}

/*opcode=0x0a */
void
x86op_or_byte_R_RM (SysEnv *m)
{

    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x0b */
void
x86op_or_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = or_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x0c */
void
x86op_or_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\tAL,");

    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = or_byte(m, m->x86.R_AL, srcval);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x0d */
void
x86op_or_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OR\tAX,");

    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = or_word(m, m->x86.R_AX, srcval);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x0e */
void
x86op_push_CS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tCS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_CS);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x0f == escape for two-byte opcode (286??? or better) */
void
x86op_two_byte (SysEnv *m)
{
    u8 op2;

    op2 = sys_rdb(m, ((u32) m->x86.R_CS << 4) + (m->x86.R_IP++));
    INC_DECODED_INST_LEN(m, 1);
    switch (op2) {
      case 0x80 ... 0x8f:
	x86op_long_jump(m, op2);
	break;

      default:
	DECODE_PRINTF(m, "Unimplemented extended opcode\n");
	halt_sys(m);
    }
}


/*opcode=0x10 */
void
x86op_adc_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x11 */
void
x86op_adc_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = adc_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x12 */
void
x86op_adc_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x13 */
void
x86op_adc_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = adc_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x14 */
void
x86op_adc_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = adc_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x15 */
void
x86op_adc_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ADC\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = adc_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x16 */
void
x86op_push_SS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tSS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_SS);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x17 */
void
x86op_pop_SS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tSS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SS = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x18 */
void
x86op_sbb_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x19 */
void
x86op_sbb_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sbb_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1a */
void
x86op_sbb_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1b */
void
x86op_sbb_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sbb_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1c */
void
x86op_sbb_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = sbb_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1d */
void
x86op_sbb_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SBB\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = sbb_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1e */
void
x86op_push_DS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tDS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_DS);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x1f */
void
x86op_pop_DS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tDS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DS = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x20 */
void
x86op_and_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x21 */
void
x86op_and_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = and_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x22 */
void
x86op_and_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x23 */
void
x86op_and_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = and_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x24 */
void
x86op_and_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = and_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x25 */
void
x86op_and_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AND\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = and_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x26 */
void
x86op_segovr_ES (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "ES:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_SEGOVR_ES;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR(m);
}


/*opcode=0x27 */
void
x86op_daa (SysEnv *m)
{
    u16 dbyte;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DAA\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);

    dbyte = m->x86.R_AL;

    if (ACCESS_FLAG(m, F_AF) || (dbyte & 0xf) > 9) {
	dbyte += 6;
	if (dbyte & 0x100)
	    SET_FLAG(m, F_CF);
	SET_FLAG(m, F_AF);
    } else
	CLEAR_FLAG(m, F_AF);

    if (ACCESS_FLAG(m, F_CF) || (dbyte & 0xf0) > 0x90) {
	dbyte += 0x60;
	SET_FLAG(m, F_CF);
    } else
	CLEAR_FLAG(m, F_CF);
    m->x86.R_AL = dbyte;

    CONDITIONAL_SET_FLAG((m->x86.R_AL & 0x80), m, F_SF);
    CONDITIONAL_SET_FLAG((m->x86.R_AL == 0), m, F_ZF);
    CONDITIONAL_SET_FLAG((PARITY(m->x86.R_AL)), m, F_PF);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x28 */
void
x86op_sub_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x29 */
void
x86op_sub_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = sub_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x2a */
void
x86op_sub_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x2b */
void
x86op_sub_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = sub_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x2c */
void
x86op_sub_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = sub_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x2d */
void
x86op_sub_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SUB\tAX,");

    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = sub_word(m, m->x86.R_AX, srcval);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x2e */
void
x86op_segovr_CS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CS:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_SEGOVR_CS;
    /* note no DECODE_CLEAR_SEGOVER here. */
    END_OF_INSTR(m);
}


/*opcode=0x2f */
void
x86op_das (SysEnv *m)
{
    u16 dbyte;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DAS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);

    dbyte = m->x86.R_AL;
    if (ACCESS_FLAG(m, F_AF) || (dbyte & 0xf) > 9) {
	dbyte -= 6;

	if (dbyte & 0x100)	/* XXXXX --- this is WRONG */
	    SET_FLAG(m, F_CF);

	SET_FLAG(m, F_AF);
    } else
	CLEAR_FLAG(m, F_AF);

    if (ACCESS_FLAG(m, F_CF) || (dbyte & 0xf0) > 0x90) {
	dbyte -= 0x60;
	SET_FLAG(m, F_CF);
    } else
	CLEAR_FLAG(m, F_CF);

    m->x86.R_AL = dbyte;

    CONDITIONAL_SET_FLAG(m->x86.R_AL & 0x80, m, F_SF);
    CONDITIONAL_SET_FLAG(m->x86.R_AL == 0, m, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(m->x86.R_AL), m, F_PF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x30 */
void
x86op_xor_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_byte(m, destval, *srcreg);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x31 */
void
x86op_xor_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = xor_word(m, destval, *srcreg);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x32 */
void
x86op_xor_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x33 */
void
x86op_xor_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = xor_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x34 */
void
x86op_xor_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = xor_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x35 */
void
x86op_xor_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XOR\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = xor_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x36 */
void
x86op_segovr_SS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SS:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_SEGOVR_SS;
    /* no DECODE_CLEAR_SEGOVER ! */
    END_OF_INSTR(m);
}

/*opcode=0x37 */
void
x86op_aaa (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AAA\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if ((m->x86.R_AL & 0xf) > 0x9 || ACCESS_FLAG(m, F_AF)) {
	m->x86.R_AL += 0x6;
	m->x86.R_AH += 1;
	SET_FLAG(m, F_AF);
	SET_FLAG(m, F_CF);
    } else {
	CLEAR_FLAG(m, F_CF);
	CLEAR_FLAG(m, F_AF);
    }

    m->x86.R_AL &= 0xf;

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x38 */
void
x86op_cmp_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, destval, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, destval, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, destval, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x39 */
void
x86op_cmp_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, destval, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, destval, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, destval, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x3a */
void
x86op_cmp_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_byte(m, *destreg, *srcreg);
	break;

    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x3b */
void
x86op_cmp_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, *destreg, srcval);
	break;

    case 1:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, *destreg, srcval);
	break;

    case 2:
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, *destreg, srcval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	cmp_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x3c */
void
x86op_cmp_byte_AL_IMM (SysEnv *m)
{
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\tAL,");
    srcval = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    cmp_byte(m, m->x86.R_AL, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x3d */
void
x86op_cmp_word_AX_IMM (SysEnv *m)
{
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMP\tAX,");
    srcval = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", srcval);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    cmp_word(m, m->x86.R_AX, srcval);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x3e */
void
x86op_segovr_DS (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DS:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_SEGOVR_DS;
    /* NO DECODE_CLEAR_SEGOVR! */
    END_OF_INSTR(m);
}


/*opcode=0x3f */
void
x86op_aas (SysEnv *m)
{
    /*
     * ????  Check out the subtraction here.  Will this ?ever? cause
     * the contents of R_AL or R_AH to be affected incorrectly since
     * they are being subtracted from *and* are unsigned.  Should put
     * an assertion in here.
     */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AAS\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);

    if ((m->x86.R_AL & 0xf) > 0x9 || ACCESS_FLAG(m, F_AF)) {
	m->x86.R_AL -= 0x6;
	m->x86.R_AH -= 1;
	SET_FLAG(m, F_AF);
	SET_FLAG(m, F_CF);
    } else {
	CLEAR_FLAG(m, F_CF);
	CLEAR_FLAG(m, F_AF);
    }

    m->x86.R_AL &= 0xf;

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x40 */
void
x86op_inc_AX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tAX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = inc_word(m, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x41 */
void
x86op_inc_CX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tCX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX = inc_word(m, m->x86.R_CX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x42 */
void
x86op_inc_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tDX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DX = inc_word(m, m->x86.R_DX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x43 */
void
x86op_inc_BX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tBX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BX = inc_word(m, m->x86.R_BX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x44 */
void
x86op_inc_SP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tSP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SP = inc_word(m, m->x86.R_SP);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x45 */
void
x86op_inc_BP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tBP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BP = inc_word(m, m->x86.R_BP);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x46 */
void
x86op_inc_SI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tSI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SI = inc_word(m, m->x86.R_SI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x47 */
void
x86op_inc_DI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INC\tDI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DI = inc_word(m, m->x86.R_DI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x48 */
void
x86op_dec_AX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tAX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = dec_word(m, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x49 */
void
x86op_dec_CX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tCX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX = dec_word(m, m->x86.R_CX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4a */
void
x86op_dec_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tDX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DX = dec_word(m, m->x86.R_DX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4b */
void
x86op_dec_BX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tBX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BX = dec_word(m, m->x86.R_BX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4c */
void
x86op_dec_SP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tSP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SP = dec_word(m, m->x86.R_SP);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4d */
void
x86op_dec_BP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tBP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BP = dec_word(m, m->x86.R_BP);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4e */
void
x86op_dec_SI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tSI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SI = dec_word(m, m->x86.R_SI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x4f */
void
x86op_dec_DI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DEC\tDI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DI = dec_word(m, m->x86.R_DI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x50 */
void
x86op_push_AX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tAX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x51 */
void
x86op_push_CX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tCX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_CX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x52 */
void
x86op_push_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tDX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_DX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x53 */
void
x86op_push_BX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tBX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_BX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x54 */
void
x86op_push_SP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tSP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);

    /* ....  Note this weirdness: One book I have access to 
       claims that the value pushed here is actually sp-2.  I.e.
       it decrements the stackpointer, and then pushes it.  The 286
       I have does it this way.  Changing this causes many problems. */
    /* changed to push SP-2, since this *IS* how a 8088 does this */

    push_word(m, m->x86.R_SP - 2);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x55 */
void
x86op_push_BP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tBP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_BP);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x56 */
void
x86op_push_SI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tSI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_SI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x57 */
void
x86op_push_DI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSH\tDI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_DI);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x58 */
void
x86op_pop_AX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tAX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x59 */
void
x86op_pop_CX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tCX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5a */
void
x86op_pop_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tDX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DX = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5b */
void
x86op_pop_BX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tBX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BX = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5c */
void
x86op_pop_SP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tSP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SP = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5d */
void
x86op_pop_BP (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tBP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BP = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5e */
void
x86op_pop_SI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tSI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SI = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x5f */
void
x86op_pop_DI (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\tDI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DI = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x60   ILLEGAL OP, calls x86op_illegal_op() */
void
x86op_push_all (SysEnv *m)
{
    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "PUSHAD\n");
    } else {
	DECODE_PRINTF(m, "PUSHA\n");
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 old_sp = m->x86.R_ESP;

	push_long(m, m->x86.R_EAX);
	push_long(m, m->x86.R_ECX);
	push_long(m, m->x86.R_EDX);
	push_long(m, m->x86.R_EBX);
	push_long(m, old_sp);
	push_long(m, m->x86.R_EBP);
	push_long(m, m->x86.R_ESI);
	push_long(m, m->x86.R_EDI);
    } else {
	u16 old_sp = m->x86.R_SP;

	push_word(m, m->x86.R_AX);
	push_word(m, m->x86.R_CX);
	push_word(m, m->x86.R_DX);
	push_word(m, m->x86.R_BX);
	push_word(m, old_sp);
	push_word(m, m->x86.R_BP);
	push_word(m, m->x86.R_SI);
	push_word(m, m->x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x61   ILLEGAL OP, calls x86op_illegal_op() */
void
x86op_pop_all (SysEnv *m)
{
    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "POPAD\n");
    } else {
	DECODE_PRINTF(m, "POPA\n");
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	m->x86.R_EDI = pop_long(m);
	m->x86.R_ESI = pop_long(m);
	m->x86.R_EBP = pop_long(m);
	m->x86.R_ESP += 4;		/* skip ESP */
	m->x86.R_EBX = pop_long(m);
	m->x86.R_EDX = pop_long(m);
	m->x86.R_ECX = pop_long(m);
	m->x86.R_EAX = pop_long(m);
    } else {
	m->x86.R_DI = pop_word(m);
	m->x86.R_SI = pop_word(m);
	m->x86.R_BP = pop_word(m);
	m->x86.R_SP += 2;		/* skip SP */
	m->x86.R_BX = pop_word(m);
	m->x86.R_DX = pop_word(m);
	m->x86.R_CX = pop_word(m);
	m->x86.R_AX = pop_word(m);
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x62   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x63   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x64   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x65   ILLEGAL OP, calls x86op_illegal_op() */


/*opcode=0x66 prefix for 32-bit register */
void
x86op_prefix_data (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "DATA:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_PREFIX_DATA;
    /* note no DECODE_CLEAR_SEGOVER here. */
    END_OF_INSTR(m);
}


/*opcode=0x67 prefix for 32-bit address */

/*opcode=0x68   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x69   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x6a   ILLEGAL OP, calls x86op_illegal_op() */
/*opcode=0x6b   ILLEGAL OP, calls x86op_illegal_op() */

/* opcode=0x6c */
void
x86op_ins_byte (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INSB\n");
    ins(m, 1);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0x6d */
void
x86op_ins_word (SysEnv *m)
{
    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "INSD\n");
	ins(m, 4);
    } else {
	DECODE_PRINTF(m, "INSW\n");
	ins(m, 2);
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0x6e */
void
x86op_outs_byte (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OUTSB\n");
    outs(m, 1);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0x6f */
void
x86op_outs_word (SysEnv *m)
{
    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "OUTSD\n");
	outs(m, 4);
    } else {
	DECODE_PRINTF(m, "OUTSW\n");
	outs(m, 2);
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x70 */
void
x86op_jump_near_O (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if overflow flag is set */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JO\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_OF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x71 */
void
x86op_jump_near_NO (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if overflow is not set */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNO\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!ACCESS_FLAG(m, F_OF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x72 */
void
x86op_jump_near_B (SysEnv *m)
{

    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is set. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JB\t");
    offset = (s8) fetch_byte_imm(m);	/* sign extended ??? */
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_CF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x73 */
void
x86op_jump_near_NB (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is clear. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNB\t");
    offset = (s8) fetch_byte_imm(m);	/* sign extended ??? */
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!ACCESS_FLAG(m, F_CF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x74 */
void
x86op_jump_near_Z (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if zero flag is set. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JZ\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_ZF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x75 */
void
x86op_jump_near_NZ (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if zero flag is clear. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNZ\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!ACCESS_FLAG(m, F_ZF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x76 */
void
x86op_jump_near_BE (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is set or if the zero 
       flag is set. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JBE\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_CF) || ACCESS_FLAG(m, F_ZF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x77 */
void
x86op_jump_near_NBE (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if carry flag is clear and if the zero 
       flag is clear */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNBE\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!(ACCESS_FLAG(m, F_CF) || ACCESS_FLAG(m, F_ZF)))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x78 */
void
x86op_jump_near_S (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if sign flag is set */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JS\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_SF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x79 */
void
x86op_jump_near_NS (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if sign flag is clear */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNS\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!ACCESS_FLAG(m, F_SF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7a */
void
x86op_jump_near_P (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if parity flag is set (even parity) */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JP\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_PF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7b */
void
x86op_jump_near_NP (SysEnv *m)
{
    s8 offset;
    u16 target;

    /* jump to byte offset if parity flag is clear (odd parity) */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNP\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (!ACCESS_FLAG(m, F_PF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7c */
void
x86op_jump_near_L (SysEnv *m)
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JL\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);

    /* note:
     *  this is the simplest expression i could think of which 
     *  expresses SF != OF.  m->x86.R_FLG&F_SF either equals x80 or x00.
     *  Similarly m->x86.R_FLG&F_OF either equals x800 or x000.
     *  The former shifted right by 7 puts a 1 or 0 in bit 0.
     *  The latter shifter right by 11 puts a 1 or 0 in bit 0.
     *  if the two expressions are the same, i.e. equal, then
     *  a zero results from the xor.  If they are not equal,
     *  then a 1 results, and the jump is taken. 
     */

    sf = ACCESS_FLAG(m, F_SF) != 0;
    of = ACCESS_FLAG(m, F_OF) != 0;

    /* was: if ( ((m->x86.R_FLG & F_SF)>>7) ^ ((m->x86.R_FLG & F_OF) >> 11)) */
    if (sf ^ of)
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7d */
void
x86op_jump_near_NL (SysEnv *m)
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNL\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);

    sf = ACCESS_FLAG(m, F_SF) != 0;
    of = ACCESS_FLAG(m, F_OF) != 0;

    /* note: inverse of above, but using == instead of xor. */
    /* was: if (((m->x86.R_FLG & F_SF)>>7) == ((m->x86.R_FLG & F_OF) >> 11)) */

    if (sf == of)
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7e */
void
x86op_jump_near_LE (SysEnv *m)
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag not equal to overflow flag
       or the zero flag is set */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JLE\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);

    sf = ACCESS_FLAG(m, F_SF) != 0;
    of = ACCESS_FLAG(m, F_OF) != 0;
    /* note: modification of JL */
    /* sf != of */
    /* was:  if ((((m->x86.R_FLG & F_SF)>>7) ^ ((m->x86.R_FLG & F_OF) >> 11)) 
       || (m->x86.R_FLG & F_ZF) ) */
    if ((sf ^ of) || ACCESS_FLAG(m, F_ZF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x7f */
void
x86op_jump_near_NLE (SysEnv *m)
{
    s8 offset;
    u16 target;
    int sf, of;

    /* jump to byte offset if sign flag equal to overflow flag.
       and the zero flag is clear */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JNLE\t");
    offset = (s8) fetch_byte_imm(m);
    target = (s16) (m->x86.R_IP) + offset;
    DECODE_PRINTF2(m, "%x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);

    sf = ACCESS_FLAG(m, F_SF) != 0;
    of = ACCESS_FLAG(m, F_OF) != 0;

    /* if (((m->x86.R_FLG & F_SF)>>7) == ((m->x86.R_FLG & F_OF) >> 11)
       && (!(m->x86.R_FLG & F_ZF))) */

    if ((sf == of) && !ACCESS_FLAG(m, F_ZF))
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


static u8 (*opc80_byte_operation[])(SysEnv *m, u8 d, u8 s) =
{
    add_byte,		/* 00 */
    or_byte,		/* 01 */
    adc_byte,		/* 02 */
    sbb_byte,		/* 03 */
    and_byte,		/* 04 */
    sub_byte,		/* 05 */
    xor_byte,		/* 06 */
    cmp_byte,		/* 07 */
};


/*opcode=0x80 */
void
x86op_opc80_byte_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 imm;
    u8 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ADD\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "OR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "ADC\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "SBB\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "AND\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SUB\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "XOR\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "CMP\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc80_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc80_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc80_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc80_byte_operation[rh]) (m, *destreg, imm);
	if (rh != 7)
	    *destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


static u16 (*opc81_word_operation[]) (SysEnv *m, u16 d, u16 s) =
{
    add_word,		/*00 */
    or_word,		/*01 */
    adc_word,		/*02 */
    sbb_word,		/*03 */
    and_word,		/*04 */
    sub_word,		/*05 */
    xor_word,		/*06 */
    cmp_word,		/*07 */
};


/*opcode=0x81 */
void
x86op_opc81_word_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 imm;
    u16 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ADD\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "OR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "ADC\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "SBB\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "AND\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SUB\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "XOR\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "CMP\t");
	    break;
	}
    }
#endif

    /*
     * Know operation, decode the mod byte to find the addressing 
     * mode.
     */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc81_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc81_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc81_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, "%x\n", imm);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc81_word_operation[rh]) (m, *destreg, imm);
	if (rh != 7)
	    *destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


static u8 (*opc82_byte_operation[]) (SysEnv *m, u8 s, u8 d) =
{
    add_byte,		/*00 */
    or_byte,		/*01 *//*YYY UNUSED ???? */
    adc_byte,		/*02 */
    sbb_byte,		/*03 */
    and_byte,		/*04 *//*YYY UNUSED ???? */
    sub_byte,		/*05 */
    xor_byte,		/*06 *//*YYY UNUSED ???? */
    cmp_byte,		/*07 */
};


/*opcode=0x82 */
void
x86op_opc82_byte_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 imm;
    u8 destval;


    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction Similar to opcode 81, except that
     * the immediate byte is sign extended to a word length.
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ADD\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "OR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "ADC\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "SBB\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "AND\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SUB\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "XOR\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "CMP\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc82_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc82_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	destval = fetch_data_byte(m, destoffset);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc82_byte_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc82_byte_operation[rh]) (m, *destreg, imm);
	if (rh != 7)
	    *destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


static u16 (*opc83_word_operation[]) (SysEnv *m, u16 s, u16 d) =
{
    add_word,		/*00 */
    or_word,		/*01 *//*YYY UNUSED ???? */
    adc_word,		/*02 */
    sbb_word,		/*03 */
    and_word,		/*04 *//*YYY UNUSED ???? */
    sub_word,		/*05 */
    xor_word,		/*06 *//*YYY UNUSED ???? */
    cmp_word,		/*07 */
};


/*opcode=0x83 */
void
x86op_opc83_word_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 imm;
    u16 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction Similar to opcode 81, except that
     * the immediate byte is sign extended to a word length.
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ADD\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "OR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "ADC\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "SBB\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "AND\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SUB\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "XOR\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "CMP\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm00_address(m, rl);
	destval = fetch_data_word(m, destoffset);
	imm = (s8) fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc83_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm01_address(m, rl);
	destval = fetch_data_word(m, destoffset);
	imm = (s8) fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc83_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm10_address(m, rl);
	destval = fetch_data_word(m, destoffset);
	imm = (s8) fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc83_word_operation[rh]) (m, destval, imm);
	if (rh != 7)
	    store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	imm = (s8) fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opc83_word_operation[rh]) (m, *destreg, imm);
	if (rh != 7)
	    *destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x84 */
void
x86op_test_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "TEST\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_byte(m, destval, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_byte(m, destval, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_byte(m, destval, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_byte(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x85 */
void
x86op_test_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "TEST\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_word(m, destval, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_word(m, destval, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_word(m, destval, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	test_word(m, *destreg, *srcreg);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x86 */
void
x86op_xchg_byte_RM_R (SysEnv *m)
{

    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;
    u8 destval;
    u8 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_byte(m, destoffset);
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = *destreg;
	*destreg = tmp;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x87 */
void
x86op_xchg_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	destval = fetch_data_word(m, destoffset);
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = destval;
	destval = tmp;
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	tmp = *srcreg;
	*srcreg = *destreg;
	*destreg = tmp;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x88 */
void
x86op_mov_byte_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 destoffset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = *srcreg;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x89 */
void
x86op_mov_word_RM_R (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, *srcreg);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, *srcreg);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, *srcreg);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = *srcreg;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8a */
void
x86op_mov_byte_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg, *srcreg;
    u16 srcoffset;
    u8 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 1:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 2:
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_byte(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = *srcreg;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8b */
void
x86op_mov_word_R_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 srcoffset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);
    switch (mod) {
    case 0:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *destreg, srcval;

	    destreg = DECODE_RM_LONG_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm00_address(m, rl);
	    srcval = fetch_data_long(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	} else {
	    u16 *destreg, srcval;

	    destreg = DECODE_RM_WORD_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm00_address(m, rl);
	    srcval = fetch_data_word(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	}
	break;

    case 1:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *destreg, srcval;

	    destreg = DECODE_RM_LONG_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm01_address(m, rl);
	    srcval = fetch_data_long(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	} else {
	    u16 *destreg, srcval;

	    destreg = DECODE_RM_WORD_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm01_address(m, rl);
	    srcval = fetch_data_word(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	}
	break;

    case 2:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *destreg, srcval;

	    destreg = DECODE_RM_LONG_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm10_address(m, rl);
	    srcval = fetch_data_long(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	} else {
	    u16 *destreg, srcval;

	    destreg = DECODE_RM_WORD_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcoffset = decode_rm10_address(m, rl);
	    srcval = fetch_data_word(m, srcoffset);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = srcval;
	}
	break;

    case 3:			/* register to register */
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *destreg, *srcreg;

	    destreg = DECODE_RM_LONG_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcreg = DECODE_RM_LONG_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = *srcreg;
	} else {
	    u16 *destreg, *srcreg;

	    destreg = DECODE_RM_WORD_REGISTER(m, rh);
	    DECODE_PRINTF(m, ",");
	    srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = *srcreg;
	}
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8c */
void
x86op_mov_word_RM_SR (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = *srcreg;
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = *srcreg;
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = *srcreg;
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",");
	srcreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = *srcreg;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8d */
void
x86op_lea_word_R_M (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *srcreg;
    u16 destoffset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LEA\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*srcreg = destoffset;
	break;

    case 1:
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*srcreg = destoffset;
	break;

    case 2:
	srcreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*srcreg = destoffset;
	break;

    case 3:			/* register to register */
	/* undefined.  Do nothing. */
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8e */
void
x86op_mov_word_SR_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg, *srcreg;
    u16 srcoffset;
    u16 srcval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    switch (mod) {
    case 0:
	destreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 1:
	destreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 2:
	destreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	srcval = fetch_data_word(m, srcoffset);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = srcval;
	break;

    case 3:			/* register to register */
	destreg = decode_rm_seg_register(m, rh);
	DECODE_PRINTF(m, ",");
	srcreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = *srcreg;
	break;
    }

    /*
     * Clean up, and reset all the R_xSP pointers to the correct
     * locations.  This is about 3x too much overhead (doing all the
     * segreg ptrs when only one is needed, but this instruction
     * *cannot* be that common, and this isn't too much work anyway.
     */

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x8f */
void
x86op_pop_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POP\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    if (rh != 0) {
	DECODE_PRINTF(m, "ILLEGAL DECODE OF OPCODE 8F\n");
	halt_sys(m);
    }
    switch (mod) {
    case 0:

	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = pop_word(m);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = pop_word(m);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = pop_word(m);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = pop_word(m);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x90 */
void
x86op_nop (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "NOP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x91 */
void
x86op_xchg_word_AX_CX (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,CX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_CX;
    m->x86.R_CX = tmp;

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x92 */
void
x86op_xchg_word_AX_DX (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,DX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_DX;
    m->x86.R_DX = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x93 */
void
x86op_xchg_word_AX_BX (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,BX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_BX;
    m->x86.R_BX = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x94 */
void
x86op_xchg_word_AX_SP (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,SP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_SP;
    m->x86.R_SP = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x95 */
void
x86op_xchg_word_AX_BP (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,BP\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_BP;
    m->x86.R_BP = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x96 */
void
x86op_xchg_word_AX_SI (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,SI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_SI;
    m->x86.R_SI = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x97 */
void
x86op_xchg_word_AX_DI (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XCHG\tAX,DI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    tmp = m->x86.R_AX;
    m->x86.R_AX = m->x86.R_DI;
    m->x86.R_DI = tmp;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x98 */
void
x86op_cbw (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CBW\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (m->x86.R_AL & 0x80) {
	m->x86.R_AH = 0xff;
    } else {
	m->x86.R_AH = 0x0;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x99 */
void
x86op_cwd (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CWD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (m->x86.R_AX & 0x8000) {
	m->x86.R_DX = 0xffff;
    } else {
	m->x86.R_DX = 0x0;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9a */
void
x86op_call_far_IMM (SysEnv *m)
{
    u16 farseg, faroff;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CALL\t");
    faroff = fetch_word_imm(m);
    farseg = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%04x:", farseg);
    DECODE_PRINTF2(m, "%04x\n", faroff);

    /* XXX
     * 
     * Hooked interrupt vectors calling into our "BIOS" will cause
     * problems unless all intersegment stuff is checked for BIOS
     * access.  Check needed here.  For moment, let it alone.
     */
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_CS);
    m->x86.R_CS = farseg;
    push_word(m, m->x86.R_IP);
    m->x86.R_IP = faroff;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9b */
void
x86op_wait (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "WAIT");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    /* NADA.  */
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9c */
void
x86op_pushf_word (SysEnv *m)
{
    u16 flags;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "PUSHF\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);

    flags = m->x86.R_FLG;
    /* clear out *all* bits not representing flags */
    flags &= F_MSK;
    /* TURN ON CHARACTERISTIC BITS OF FLAG FOR 8088 */
    flags |= F_ALWAYS_ON;
    push_word(m, flags);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9d */
void
x86op_popf_word (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "POPF\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_FLG = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9e */
void
x86op_sahf (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SAHF\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    /* clear the lower bits of the flag register */
    m->x86.R_FLG &= 0xffffff00;
    /* or in the AH register into the flags register */
    m->x86.R_FLG |= m->x86.R_AH;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0x9f */
void
x86op_lahf (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LAHF\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AH = m->x86.R_FLG & 0xff;
    /*undocumented TC++ behavior??? Nope.  It's documented, but 
       you have too look real hard to notice it. */
    m->x86.R_AH |= 0x2;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa0 */
void
x86op_mov_AL_M_IMM (SysEnv *m)
{
    u16 offset;
    u8 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tAL,");
    offset = fetch_word_imm(m);
    DECODE_PRINTF2(m, "[%04x]\n", offset);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    destval = fetch_data_byte(m, offset);
    m->x86.R_AL = destval;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}



/*opcode=0xa1 */
void
x86op_mov_AX_M_IMM (SysEnv *m)
{
    u16 offset;
    u16 destval;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tAX,");
    offset = fetch_word_imm(m);
    DECODE_PRINTF2(m, "[%04x]\n", offset);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    destval = fetch_data_word(m, offset);
    m->x86.R_AX = destval;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa2 */
void
x86op_mov_M_AL_IMM (SysEnv *m)
{
    u16 offset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    offset = fetch_word_imm(m);
    DECODE_PRINTF2(m, "[%04x],AL\n", offset);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    store_data_byte(m, offset, m->x86.R_AL);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa3 */
void
x86op_mov_M_AX_IMM (SysEnv *m)
{
    u16 offset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    offset = fetch_word_imm(m);
    DECODE_PRINTF2(m, "[%04x],AX\n", offset);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    store_data_word(m, offset, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa4 */
void
x86op_movs_byte (SysEnv *m)
{
    u8 val;
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOVS\tBYTE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -1;
    else
	inc = 1;

    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val = fetch_data_byte(m, m->x86.R_SI);
	    store_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI, val);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
	val = fetch_data_byte(m, m->x86.R_SI);
	store_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI, val);
	m->x86.R_SI += inc;
	m->x86.R_DI += inc;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa5 */
void
x86op_movs_word (SysEnv *m)
{
    s16 val;
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOVS\tWORD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -2;
    else
	inc = 2;

    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val = fetch_data_word(m, m->x86.R_SI);
	    store_data_word_abs(m, m->x86.R_ES, m->x86.R_DI, val);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);

    } else {
	val = fetch_data_word(m, m->x86.R_SI);
	store_data_word_abs(m, m->x86.R_ES, m->x86.R_DI, val);
	m->x86.R_SI += inc;
	m->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa6 */
void
x86op_cmps_byte (SysEnv *m)
{
    s8 val1, val2;
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMPS\tBYTE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -1;
    else
	inc = 1;

    if (m->x86.mode & SYSMODE_PREFIX_REPE) {
	/* REPE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val1 = fetch_data_byte(m, m->x86.R_SI);
	    val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_byte(m, val1, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF) == 0)
		break;
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (m->x86.mode & SYSMODE_PREFIX_REPNE) {
	/* REPNE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val1 = fetch_data_byte(m, m->x86.R_SI);
	    val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_byte(m, val1, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF))
		break;		/* zero flag set means equal */
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
	val1 = fetch_data_byte(m, m->x86.R_SI);
	val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	cmp_byte(m, val1, val2);
	m->x86.R_SI += inc;
	m->x86.R_DI += inc;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa7 */
void
x86op_cmps_word (SysEnv *m)
{
    s16 val1, val2;
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMPS\tWORD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -2;
    else
	inc = 2;

    if (m->x86.mode & SYSMODE_PREFIX_REPE) {

	/* REPE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val1 = fetch_data_word(m, m->x86.R_SI);
	    val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_word(m, val1, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF) == 0)
		break;
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (m->x86.mode & SYSMODE_PREFIX_REPNE) {
	/* REPNE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val1 = fetch_data_word(m, m->x86.R_SI);
	    val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_word(m, val1, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF))
		break;		/* zero flag set means equal */
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
	val1 = fetch_data_word(m, m->x86.R_SI);
	val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
	cmp_word(m, val1, val2);
	m->x86.R_SI += inc;
	m->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa8 */
void
x86op_test_AL_IMM (SysEnv *m)
{
    int imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "TEST\tAL,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%04x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    test_byte(m, m->x86.R_AL, imm);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xa9 */
void
x86op_test_AX_IMM (SysEnv *m)
{
    int imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "TEST\tAX,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%04x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    test_word(m, m->x86.R_AX, imm);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xaa */
void
x86op_stos_byte (SysEnv *m)
{
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "STOS\tBYTE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -1;
    else
	inc = 1;
    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    store_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_AL);
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
	store_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_AL);
	m->x86.R_DI += inc;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xab */
void
x86op_stos_word (SysEnv *m)
{
    int inc = 2;

    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "STOS\tLONG\n");
	inc = 4;
    } else {
	DECODE_PRINTF(m, "STOS\tWORD\n");
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -inc;
    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
		store_data_long_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_EAX);
	    } else {
		store_data_word_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_AX);
	    }
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    store_data_long_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_EAX);
	} else {
	    store_data_word_abs(m, m->x86.R_ES, m->x86.R_DI, m->x86.R_AX);
	}
	m->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xac */
void
x86op_lods_byte (SysEnv *m)
{
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LODS\tBYTE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -1;
    else
	inc = 1;
    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    m->x86.R_AL = fetch_data_byte(m, m->x86.R_SI);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
	m->x86.R_AL = fetch_data_byte(m, m->x86.R_SI);
	m->x86.R_SI += inc;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xad */
void
x86op_lods_word (SysEnv *m)
{
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LODS\tWORD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -2;
    else
	inc = 2;
    if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
	/* dont care whether REPE or REPNE */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    m->x86.R_AX = fetch_data_word(m, m->x86.R_SI);
	    m->x86.R_CX -= 1;
	    m->x86.R_SI += inc;
	}
	m->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);

    } else {
	m->x86.R_AX = fetch_data_word(m, m->x86.R_SI);
	m->x86.R_SI += inc;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xae */
void
x86op_scas_byte (SysEnv *m)
{
    s8 val2;
    int inc;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "SCAS\tBYTE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -1;
    else
	inc = 1;

    if (m->x86.mode & SYSMODE_PREFIX_REPE) {

	/* REPE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_byte(m, m->x86.R_AL, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF) == 0)
		break;
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (m->x86.mode & SYSMODE_PREFIX_REPNE) {
	/* REPNE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_byte(m, m->x86.R_AL, val2);
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF))
		break;		/* zero flag set means equal */
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
	val2 = fetch_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI);
	cmp_byte(m, m->x86.R_AL, val2);
	m->x86.R_DI += inc;
    }

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xaf */
void
x86op_scas_word (SysEnv *m)
{
    int inc = 2;

    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "SCAS\tLONG\n");
	inc = 4;
    } else {
	DECODE_PRINTF(m, "SCAS\tWORD\n");
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (ACCESS_FLAG(m, F_DF))	/* down */
	inc = -inc;

    if (m->x86.mode & SYSMODE_PREFIX_REPE) {

	/* REPE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
		s32 val2;

		val2 = fetch_data_long_abs(m, m->x86.R_ES, m->x86.R_DI);
		cmp_long(m, m->x86.R_EAX, val2);
	    } else {
		s16 val2;

		val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
		cmp_word(m, m->x86.R_AX, val2);
	    }
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF) == 0)
		break;
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (m->x86.mode & SYSMODE_PREFIX_REPNE) {
	/* REPNE  */
	/* move them until CX is ZERO. */
	while (m->x86.R_CX != 0) {
	    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
		s32 val2;

		val2 = fetch_data_long_abs(m, m->x86.R_ES, m->x86.R_DI);
		cmp_long(m, m->x86.R_EAX, val2);
	    } else {
		s16 val2;

		val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
		cmp_word(m, m->x86.R_AX, val2);
	    }
	    m->x86.R_CX -= 1;
	    m->x86.R_DI += inc;
	    if (ACCESS_FLAG(m, F_ZF))
		break;		/* zero flag set means equal */
	}
	m->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    s32 val2;

	    val2 = fetch_data_long_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_long(m, m->x86.R_EAX, val2);
	} else {
	    s16 val2;

	    val2 = fetch_data_word_abs(m, m->x86.R_ES, m->x86.R_DI);
	    cmp_word(m, m->x86.R_AX, val2);
	}
	m->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb0 */
void
x86op_mov_byte_AL_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tAL,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb1 */
void
x86op_mov_byte_CL_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tCL,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CL = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb2 */
void
x86op_mov_byte_DL_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tDL,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DL = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb3 */
void
x86op_mov_byte_BL_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tBL,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BL = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb4 */
void
x86op_mov_byte_AH_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tAH,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AH = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb5 */
void
x86op_mov_byte_CH_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tCH,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CH = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb6 */
void
x86op_mov_byte_DH_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tDH,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DH = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb7 */
void
x86op_mov_byte_BH_IMM (SysEnv *m)
{
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tBH,");
    imm = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BH = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb8 */
void
x86op_mov_word_AX_IMM (SysEnv *m)
{
    u32 imm;

    START_OF_INSTR(m);
    if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF(m, "MOV\tEAX,");
	imm = fetch_long_imm(m);
    } else {
	DECODE_PRINTF(m, "MOV\tAX,");
	imm = fetch_word_imm(m);
    }
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_EAX = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xb9 */
void
x86op_mov_word_CX_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tCX,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xba */
void
x86op_mov_word_DX_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tDX,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DX = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xbb */
void
x86op_mov_word_BX_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tBX,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BX = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xbc */
void
x86op_mov_word_SP_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tSP,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SP = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xbd */
void
x86op_mov_word_BP_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tBP,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_BP = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xbe */
void
x86op_mov_word_SI_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tSI,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_SI = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xbf */
void
x86op_mov_word_DI_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\tDI,");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_DI = imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* used by opcodes c0, d0, and d2. */
static u8(*opcD0_byte_operation[]) (SysEnv *m, u8 d, u8 s) =
{
    rol_byte,
    ror_byte,
    rcl_byte,
    rcr_byte,
    shl_byte,
    shr_byte,
    shl_byte,		/* sal_byte === shl_byte  by definition */
    sar_byte,
};


/* opcode=0xc0 */
void
x86op_opcC0_byte_RM_MEM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	amt = fetch_byte_imm(m);	
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	amt = fetch_byte_imm(m);	
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	amt = fetch_byte_imm(m);	
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	amt = fetch_byte_imm(m);	
	DECODE_PRINTF2(m, ",%x\n", amt);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, *destreg, amt);
	*destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}

/* used by opcodes c1, d1, and d3. */
static u16(*opcD1_word_operation[]) (SysEnv *m, u16 s, u16 d) =
{
    rol_word,
    ror_word,
    rcl_word,
    rcr_word,
    shl_word,
    shr_word,
    shl_word,		/* sal_byte === shl_byte  by definition */
    sar_word,
};


/* used by opcodes c1, d1, and d3. */
static u32 (*opcD1_long_operation[]) (SysEnv *m, u32 s, u32 d) =
{
    rol_long,
    ror_long,
    rcl_long,
    rcr_long,
    shl_long,
    shr_long,
    shl_long,		/* sal_byte === shl_byte  by definition */
    sar_long,
};


/* opcode=0xc1 */
void
x86op_opcC1_word_RM_MEM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 destval;
    u8 amt;


    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);


#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm00_address(m, rl);
	amt = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm01_address(m, rl);
	amt = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	amt = fetch_byte_imm(m);	
	DECODE_PRINTF2(m, ",%x\n", amt);
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	amt = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%x\n", amt);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = (*opcD1_word_operation[rh]) (m, *destreg, amt);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc2 */
void
x86op_ret_near_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "RET\t");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = pop_word(m);
    m->x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc3 */
void
x86op_ret_near (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "RET\n");
    RETURN_TRACE(m, m->x86.saved_ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc4 */
void
x86op_les_R_IMM (SysEnv *m)
{
    u16 mod, rh, rl;
    u16 *dstreg;
    u16 srcoffset;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LES\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);
    switch (mod) {
    case 0:

	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_ES = fetch_data_word(m, srcoffset + 2);
	break;

    case 1:
	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_ES = fetch_data_word(m, srcoffset + 2);
	break;

    case 2:
	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_ES = fetch_data_word(m, srcoffset + 2);
	break;

    case 3:			/* register to register */
	/* UNDEFINED! */
	TRACE_REGS(m);
	SINGLE_STEP(m);
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc5 */
void
x86op_lds_R_IMM (SysEnv *m)
{
    u16 mod, rh, rl;
    u16 *dstreg;
    u16 srcoffset;


    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LDS\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);
    switch (mod) {
    case 0:
	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_DS = fetch_data_word(m, srcoffset + 2);
	break;

    case 1:
	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_DS = fetch_data_word(m, srcoffset + 2);
	break;

    case 2:
	dstreg = DECODE_RM_WORD_REGISTER(m, rh);
	DECODE_PRINTF(m, ",");
	srcoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*dstreg = fetch_data_word(m, srcoffset);
	m->x86.R_DS = fetch_data_word(m, srcoffset + 2);
	break;

    case 3:			/* register to register */
	/* UNDEFINED! */
	TRACE_REGS(m);
	SINGLE_STEP(m);
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc6 */
void
x86op_mov_byte_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    if (rh != 0) {
	DECODE_PRINTF(m, "ILLEGAL DECODE OF OPCODE c6\n");
	halt_sys(m);
    }
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%2x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, imm);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%2x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, imm);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%2x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_byte(m, destoffset, imm);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	imm = fetch_byte_imm(m);
	DECODE_PRINTF2(m, ",%2x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = imm;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc7 */
void
x86op_mov_word_RM_IMM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "MOV\t");
    FETCH_DECODE_MODRM(m, mod, rh, rl);

    if (rh != 0) {
	DECODE_PRINTF(m, "ILLEGAL DECODE OF OPCODE 8F\n");
	halt_sys(m);
    }
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm00_address(m, rl);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, imm);
	break;

    case 1:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm01_address(m, rl);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, imm);
	break;

    case 2:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm10_address(m, rl);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, ",%x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	store_data_word(m, destoffset, imm);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	imm = fetch_word_imm(m);
	DECODE_PRINTF2(m, ",%2x\n", imm);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = imm;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xc8  ILLEGAL OP */
/*opcode=0xc9  ILLEGAL OP */


/*opcode=0xca */
void
x86op_ret_far_IMM (SysEnv *m)
{
    u16 imm;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "RETF\t");
    imm = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%x\n", imm);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = pop_word(m);
    m->x86.R_CS = pop_word(m);
    m->x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xcb */
void
x86op_ret_far (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "RETF\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = pop_word(m);
    m->x86.R_CS = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xcc */
void
x86op_int3 (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INT 3\n");
    tmp = (u16) mem_access_word(m, 3 * 4 + 2);
    /* access the segment register */
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (tmp == BIOS_SEG) {
	(*bios_intr_tab[3])(m, 3);
    } else {
	tmp = m->x86.R_FLG;
	push_word(m, tmp);
	CLEAR_FLAG(m, F_IF);
	CLEAR_FLAG(m, F_TF);
	push_word(m, m->x86.R_CS);
	tmp = mem_access_word(m, 3 * 4 + 2);
	m->x86.R_CS = tmp;
	push_word(m, m->x86.R_IP);
	tmp = mem_access_word(m, 3 * 4);
	m->x86.R_IP = tmp;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xcd */
void
x86op_int_IMM (SysEnv *m)
{
    u16 tmp;
    u8 intnum;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INT\t");
    intnum = fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x\n", intnum);
    tmp = mem_access_word(m, intnum * 4 + 2);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (tmp == BIOS_SEG) {
	(*bios_intr_tab[intnum])(m, intnum);
    } else {
	tmp = m->x86.R_FLG;
	push_word(m, tmp);
	CLEAR_FLAG(m, F_IF);
	CLEAR_FLAG(m, F_TF);
	push_word(m, m->x86.R_CS);
	tmp = mem_access_word(m, intnum * 4 + 2);
	m->x86.R_CS = tmp;
	push_word(m, m->x86.R_IP);
	tmp = mem_access_word(m, intnum * 4);
	m->x86.R_IP = tmp;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xce */
void
x86op_into (SysEnv *m)
{
    u16 tmp;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "INTO\n");

    TRACE_REGS(m);
    SINGLE_STEP(m);

    if (ACCESS_FLAG(m, F_OF)) {
	tmp = mem_access_word(m, 4 * 4 + 2);
	if (tmp == BIOS_SEG) {
	    (*bios_intr_tab[4])(m, 4);
	} else {
	    tmp = m->x86.R_FLG;
	    push_word(m, tmp);
	    CLEAR_FLAG(m, F_IF);
	    CLEAR_FLAG(m, F_TF);
	    push_word(m, m->x86.R_CS);
	    tmp = (u16) mem_access_word(m, 4 * 4 + 2);
	    m->x86.R_CS = tmp;
	    push_word(m, m->x86.R_IP);
	    tmp = mem_access_word(m, 4 * 4);
	    m->x86.R_IP = tmp;
	}
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xcf */
void
x86op_iret (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "IRET\n");

    TRACE_REGS(m);
    SINGLE_STEP(m);

    m->x86.R_IP = pop_word(m);
    m->x86.R_CS = pop_word(m);
    m->x86.R_FLG = pop_word(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd0 */
void
x86op_opcD0_byte_RM_1 (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 destval;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",1\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, 1);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",1\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, 1);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",1\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, 1);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",1\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, *destreg, 1);
	*destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd1 */
void
x86op_opcD1_word_RM_1 (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 destoffset;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 destval;

	    DECODE_PRINTF(m, "LONG PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_long(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_long_operation[rh]) (m, destval, 1);
	    store_data_long(m, destoffset, destval);
	} else {
	    u16 destval;

	    DECODE_PRINTF(m, "WORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_word_operation[rh]) (m, destval, 1);
	    store_data_word(m, destoffset, destval);
	}
	break;

    case 1:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 destval;

	    DECODE_PRINTF(m, "LONG PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_long(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_long_operation[rh]) (m, destval, 1);
	    store_data_long(m, destoffset, destval);
	} else {
	    u16 destval;

	    DECODE_PRINTF(m, "WORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_word_operation[rh]) (m, destval, 1);
	    store_data_word(m, destoffset, destval);
	}
	break;

    case 2:
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 destval;

	    DECODE_PRINTF(m, "LONG PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_long(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_long_operation[rh]) (m, destval, 1);
	    store_data_long(m, destoffset, destval);
	} else {
	    u16 destval;

	    DECODE_PRINTF(m, "BYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_word_operation[rh]) (m, destval, 1);
	    store_data_word(m, destoffset, destval);
	}
	break;

    case 3:			/* register to register */
	if (m->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 destval, *destreg;

	    destreg = DECODE_RM_LONG_REGISTER(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_long_operation[rh]) (m, *destreg, 1);
	    *destreg = destval;
	} else {
	    u16 destval, *destreg;

	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, ",1\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = (*opcD1_word_operation[rh]) (m, *destreg, 1);
	    *destreg = destval;
	}
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd2 */
void
x86op_opcD2_byte_RM_CL (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    amt = m->x86.R_CL;

    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_byte(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, destval, amt);
	store_data_byte(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD0_byte_operation[rh]) (m, *destreg, amt);
	*destreg = destval;
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd3 */
void
x86op_opcD3_word_RM_CL (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 destval;
    u8 amt;


    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "ROL\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "ROR\t");
	    break;
	case 2:
	    DECODE_PRINTF(m, "RCL\t");
	    break;
	case 3:
	    DECODE_PRINTF(m, "RCR\t");
	    break;
	case 4:
	    DECODE_PRINTF(m, "SHL\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "SHR\t");
	    break;
	case 6:
	    DECODE_PRINTF(m, "SAL\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "SAR\t");
	    break;
	}
    }
#endif

    amt = m->x86.R_CL;
    /* know operation, decode the mod byte to find the addressing 
       mode. */
    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 1:
	DECODE_PRINTF(m, "WORD PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	destval = fetch_data_word(m, destoffset);
	TRACE_REGS(m);
	SINGLE_STEP(m);
	destval = (*opcD1_word_operation[rh]) (m, destval, amt);
	store_data_word(m, destoffset, destval);
	break;

    case 3:			/* register to register */
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, ",CL\n");
	TRACE_REGS(m);
	SINGLE_STEP(m);
	*destreg = (*opcD1_word_operation[rh]) (m, *destreg, amt);
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd4 */
void
x86op_aam (SysEnv *m)
{
    u8 a;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AAM\n");
    a = fetch_byte_imm(m);	/* this is a stupid encoding. */
    if (a != 10) {
	DECODE_PRINTF(m, "ERROR DECODING AAM\n");
	TRACE_REGS(m);
	halt_sys(m);
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);

    /* note the type change here --- returning AL and AH in AX. */
    m->x86.R_AX = aam_word(m, m->x86.R_AL);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd5 */
void
x86op_aad (SysEnv *m)
{
    u8 a;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "AAD\n");
    a = fetch_byte_imm(m);
    TRACE_REGS(m);
    SINGLE_STEP(m);

    m->x86.R_AX = aad_word(m, m->x86.R_AX);

    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* opcode=0xd6 ILLEGAL OPCODE */


/* opcode=0xd7 */
void
x86op_xlat (SysEnv *m)
{
    u16 addr;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "XLAT\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    addr = m->x86.R_BX + (u8) m->x86.R_AL;
    m->x86.R_AL = fetch_data_byte(m, addr);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/* instuctions  D8 .. DF are in i87_ops.c */


/*opcode=0xe0 */
void
x86op_loopne (SysEnv *m)
{
    s16 ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LOOPNE\t");
    ip = (s8) fetch_byte_imm(m);
    ip += (s16) m->x86.R_IP;
    DECODE_PRINTF2(m, "%04x\n", ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX -= 1;
    if (m->x86.R_CX != 0 && !ACCESS_FLAG(m, F_ZF))	/* CX != 0 and !ZF */
	m->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe1 */
void
x86op_loope (SysEnv *m)
{
    s16 ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LOOPE\t");
    ip = (s8) fetch_byte_imm(m);
    ip += (s16) m->x86.R_IP;
    DECODE_PRINTF2(m, "%04x\n", ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX -= 1;
    if (m->x86.R_CX != 0 && ACCESS_FLAG(m, F_ZF))	/* CX != 0 and ZF */
	m->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe2 */
void
x86op_loop (SysEnv *m)
{
    s16 ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LOOP\t");
    ip = (s8) fetch_byte_imm(m);
    ip += (s16) m->x86.R_IP;
    DECODE_PRINTF2(m, "%04x\n", ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_CX -= 1;
    if (m->x86.R_CX != 0)
	m->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe3 */
void
x86op_jcxz (SysEnv *m)
{
    s16 offset, target;

    /* jump to byte offset if overflow flag is set */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JCXZ\t");
    offset = (s8) fetch_byte_imm(m);	/* sign extended ??? */
    target = (s16) m->x86.R_IP + offset;
    DECODE_PRINTF2(m, "%x\n", (u16) target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    if (m->x86.R_CX == 0)
	m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe4 */
void
x86op_in_byte_AL_IMM (SysEnv *m)
{
    u8 port;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "IN\t");
    port = (u8) fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x,AL\n", port);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = sys_inb(m, port);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe5 */
void
x86op_in_word_AX_IMM (SysEnv *m)
{
    u8 port;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "IN\t");
    port = (u8) fetch_byte_imm(m);
    DECODE_PRINTF2(m, "AX,%x\n", port);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = sys_inw(m, port);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe6 */
void
x86op_out_byte_IMM_AL (SysEnv *m)
{
    u8 port;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OUT\t");
    port = (u8) fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x,AL\n", port);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    sys_outb(m, port, m->x86.R_AL);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe7 */
void
x86op_out_word_IMM_AX (SysEnv *m)
{
    u8 port;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OUT\t");
    port = (u8) fetch_byte_imm(m);
    DECODE_PRINTF2(m, "%x,AX\n", port);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    sys_outw(m, port, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe8 */
void
x86op_call_near_IMM (SysEnv *m)
{
    s16 ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CALL\t");
    /* weird.  Thought this was a signed disp! */
    ip = (s16) fetch_word_imm(m);
    ip += (s16) m->x86.R_IP;	/* CHECK SIGN */
    DECODE_PRINTF2(m, "%04x\n", ip);
    CALL_TRACE(m, m->x86.saved_ip, m->x86.R_CS, ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    push_word(m, m->x86.R_IP);
    m->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xe9 */
void
x86op_jump_near_IMM (SysEnv *m)
{
    int ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JMP\t");
    /* weird.  Thought this was a signed disp too! */
    ip = (s16) fetch_word_imm(m);
    ip += (s16) m->x86.R_IP;	/* CHECK SIGN */
    DECODE_PRINTF2(m, "%04x\n", ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xea */
void
x86op_jump_far_IMM (SysEnv *m)
{
    u16 cs, ip;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JMP\tFAR ");
    ip = fetch_word_imm(m);
    cs = fetch_word_imm(m);
    DECODE_PRINTF2(m, "%04x:", cs);
    DECODE_PRINTF2(m, "%04x\n", ip);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = ip;
    m->x86.R_CS = cs;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xeb */
void
x86op_jump_byte_IMM (SysEnv *m)
{
    s8 offset;
    u16 target;

    START_OF_INSTR(m);
    DECODE_PRINTF(m, "JMP\t");
    offset = (s8) fetch_byte_imm(m);	/* CHECK */
    target = (s16) m->x86.R_IP + offset;
    DECODE_PRINTF2(m, "%04x\n", target);
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xec */
void
x86op_in_byte_AL_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "IN\tAL,DX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AL = sys_inb(m, m->x86.R_DX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xed */
void
x86op_in_word_AX_DX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "IN\tAX,DX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.R_AX = sys_inw(m, m->x86.R_DX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xee */
void
x86op_out_byte_DX_AL (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OUT\tDX,AL\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    sys_outb(m, m->x86.R_DX, m->x86.R_AL);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xef */
void
x86op_out_word_DX_AX (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "OUT\tDX,AX\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    sys_outw(m, m->x86.R_DX, m->x86.R_AX);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf0 */
void
x86op_lock (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "LOCK:\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf1 ILLEGAL OPERATION */

/*opcode=0xf2 */
void
x86op_repne (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "REPNE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_PREFIX_REPNE;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf3 */
void
x86op_repe (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "REPE\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->x86.mode |= SYSMODE_PREFIX_REPE;
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf4 */
void
x86op_halt (SysEnv *m)
{
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "HALT\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    halt_sys(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf5 */
void
x86op_cmc (SysEnv *m)
{
    /* complement the carry flag. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CMC\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    TOGGLE_FLAG(m, F_CF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf6 */
void
x86op_opcF6_byte_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u8 *destreg;
    u16 destoffset;
    u8 destval, srcval;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */

    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);
    switch (mod) {
    case 0:			/* mod=00 */
	switch (rh) {
	case 0:		/* test byte imm */
	    DECODE_PRINTF(m, "TEST\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_byte_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_byte(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_byte(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_byte(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_byte(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tBYTE PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_byte(m, destval);
	    break;
	}
	break;			/* end mod==00 */
    case 1:			/* mod=01 */
	switch (rh) {
	case 0:		/* test byte imm */
	    DECODE_PRINTF(m, "TEST\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_byte_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_byte(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=01 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_byte(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_byte(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_byte(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tBYTE PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_byte(m, destval);
	    break;
	}
	break;			/* end mod==01 */
    case 2:			/* mod=10 */
	switch (rh) {
	case 0:		/* test byte imm */
	    DECODE_PRINTF(m, "TEST\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_byte_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_byte(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=10 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_byte(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_byte(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_byte(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tBYTE PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_byte(m, destval);
	    break;
	}
	break;			/* end mod==10 */
    case 3:			/* mod=11 */
	switch (rh) {
	case 0:		/* test byte imm */
	    DECODE_PRINTF(m, "TEST\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_byte_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_byte(m, *destreg, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = not_byte(m, *destreg);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = neg_byte(m, *destreg);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_byte(m, *destreg);	/*!!!  */
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_byte(m, *destreg);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_byte(m, *destreg);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\t");
	    destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_byte(m, *destreg);
	    break;
	}
	break;			/* end mod==11 */
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf7 */
void
x86op_opcF7_word_RM (SysEnv *m)
{
    u16 mod, rl, rh;
    u16 *destreg;
    u16 destoffset;
    u16 destval, srcval;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */

    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);
    switch (mod) {
    case 0:			/* mod=00 */
	switch (rh) {
	case 0:		/* test word imm */
	    DECODE_PRINTF(m, "TEST\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_word_imm(m);
	    DECODE_PRINTF2(m, "%04x\n", srcval);
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_word(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=00 RH=01 OP=F7\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_word(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_word(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_word(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tWORD PTR ");
	    destoffset = decode_rm00_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_word(m, destval);
	    break;
	}
	break;			/* end mod==00 */
    case 1:			/* mod=01 */
	switch (rh) {
	case 0:		/* test word imm */
	    DECODE_PRINTF(m, "TEST\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_word_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_word(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=01 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_word(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_word(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_word(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tWORD PTR ");
	    destoffset = decode_rm01_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_word(m, destval);
	    break;
	}
	break;			/* end mod==01 */
    case 2:			/* mod=10 */
	switch (rh) {
	case 0:		/* test word imm */
	    DECODE_PRINTF(m, "TEST\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_word_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_word(m, destval, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=10 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = not_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = neg_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_word(m, destval);
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_word(m, destval);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_word(m, destval);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\tWORD PTR ");
	    destoffset = decode_rm10_address(m, rl);
	    DECODE_PRINTF(m, "\n");
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_word(m, destval);
	    break;
	}
	break;			/* end mod==10 */
    case 3:			/* mod=11 */
	switch (rh) {
	case 0:		/* test word imm */
	    DECODE_PRINTF(m, "TEST\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, ",");
	    srcval = fetch_word_imm(m);
	    DECODE_PRINTF2(m, "%02x\n", srcval);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    test_word(m, *destreg, srcval);
	    break;
	case 1:
	    DECODE_PRINTF(m, "ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	    halt_sys(m);
	    break;
	case 2:
	    DECODE_PRINTF(m, "NOT\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = not_word(m, *destreg);
	    break;
	case 3:
	    DECODE_PRINTF(m, "NEG\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = neg_word(m, *destreg);
	    break;
	case 4:
	    DECODE_PRINTF(m, "MUL\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    mul_word(m, *destreg);	/*!!!  */
	    break;
	case 5:
	    DECODE_PRINTF(m, "IMUL\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    imul_word(m, *destreg);
	    break;
	case 6:
	    DECODE_PRINTF(m, "DIV\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    div_word(m, *destreg);
	    break;
	case 7:
	    DECODE_PRINTF(m, "IDIV\t");
	    destreg = DECODE_RM_WORD_REGISTER(m, rl);
	    DECODE_PRINTF(m, "\n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    idiv_word(m, *destreg);
	    break;
	}
	break;			/* end mod==11 */
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf8 */
void
x86op_clc (SysEnv *m)
{
    /* clear the carry flag. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CLC\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    CLEAR_FLAG(m, F_CF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xf9 */
void
x86op_stc (SysEnv *m)
{
    /* set the carry flag. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "STC\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    SET_FLAG(m, F_CF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xfa */
void
x86op_cli (SysEnv *m)
{
    /* clear interrupts. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CLI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    CLEAR_FLAG(m, F_IF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xfb */
void
x86op_sti (SysEnv *m)
{
    /* enable  interrupts. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "STI\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    SET_FLAG(m, F_IF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xfc */
void
x86op_cld (SysEnv *m)
{
    /* clear interrupts. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "CLD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    CLEAR_FLAG(m, F_DF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xfd */
void
x86op_std (SysEnv *m)
{
    /* clear interrupts. */
    START_OF_INSTR(m);
    DECODE_PRINTF(m, "STD\n");
    TRACE_REGS(m);
    SINGLE_STEP(m);
    SET_FLAG(m, F_DF);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xfe */
void
x86op_opcFE_byte_RM (SysEnv *m)
{
    /* Yet another damned special case instruction. */

    u16 mod, rh, rl;
    u8 destval;
    u16 destoffset;
    u8 *destreg;

    /* ARRGH, ANOTHER GODDAMN SPECIAL CASE INSTRUCTION!!! */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);
#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "INC\t");
	    break;
	case 1:
	    DECODE_PRINTF(m, "DEC\t");
	    break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	    DECODE_PRINTF2(m, "ILLEGAL OP MAJOR OP 0xFE MINOR OP %x \n", mod);
	    halt_sys(m);
	    break;
	}
    }
#endif

    switch (mod) {
    case 0:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:		/* inc word ptr ... */
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 1:		/* dec word ptr ... */
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	}
	break;


    case 1:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 1:
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	}
	break;

    case 2:
	DECODE_PRINTF(m, "BYTE PTR ");
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	case 1:
	    destval = fetch_data_byte(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_byte(m, destval);
	    store_data_byte(m, destoffset, destval);
	    break;
	}
	break;

    case 3:
	destreg = DECODE_RM_BYTE_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = inc_byte(m, *destreg);
	    break;
	case 1:
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = dec_byte(m, *destreg);
	    break;
	}
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*opcode=0xff */
void
x86op_opcFF_word_RM (SysEnv *m)
{
    u16 mod, rh, rl;
    u16 destval, destval2;
    u16 destoffset;
    u16 *destreg;

    /* ANOTHER DAMN SPECIAL CASE INSTRUCTION!!! */
    START_OF_INSTR(m);
    FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
    if (DEBUG_DECODE(m)) {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh) {
	case 0:
	    DECODE_PRINTF(m, "INC\tWORD PTR ");
	    break;
	case 1:
	    DECODE_PRINTF(m, "DEC\tWORD PTR ");
	    break;
	case 2:
	    DECODE_PRINTF(m, "CALL\t ");
	    break;
	case 3:
	    DECODE_PRINTF(m, "CALL\tFAR ");
	    break;
	case 4:
	    DECODE_PRINTF(m, "JMP\t");
	    break;
	case 5:
	    DECODE_PRINTF(m, "JMP\tFAR ");
	    break;
	case 6:
	    DECODE_PRINTF(m, "PUSH\t");
	    break;
	case 7:
	    DECODE_PRINTF(m, "ILLEGAL DECODING OF OPCODE FF\t");
	    halt_sys(m);
	    break;
	}
    }
#endif

    switch (mod) {
    case 0:
	destoffset = decode_rm00_address(m, rl);
	DECODE_PRINTF(m, "\n");

	switch (rh) {
	case 0:		/* inc word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 1:		/* dec word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 2:		/* call word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;
	case 3:		/* call far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_CS);
	    m->x86.R_CS = destval2;
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;
	case 4:		/* jmp word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    break;
	case 5:		/* jmp far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    m->x86.R_CS = destval2;
	    break;

	case 6:		/*  push word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, destval);
	    break;
	}
	break;

    case 1:
	destoffset = decode_rm01_address(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 1:
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 2:		/* call word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;
	case 3:		/* call far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_CS);
	    m->x86.R_CS = destval2;
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;
	case 4:		/* jmp word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    break;
	case 5:		/* jmp far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    m->x86.R_CS = destval2;
	    break;
	case 6:		/*  push word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, destval);
	    break;
	}
	break;

    case 2:
	destoffset = decode_rm10_address(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = inc_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 1:
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    destval = dec_word(m, destval);
	    store_data_word(m, destoffset, destval);
	    break;
	case 2:		/* call word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;

	case 3:		/* call far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_CS);
	    m->x86.R_CS = destval2;
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = destval;
	    break;

	case 4:		/* jmp word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    break;
	case 5:		/* jmp far ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    destval2 = fetch_data_word(m, destoffset + 2);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = destval;
	    m->x86.R_CS = destval2;
	    break;
	case 6:		/*  push word ptr ... */
	    destval = fetch_data_word(m, destoffset);
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, destval);
	    break;
	}
	break;

    case 3:
	destreg = DECODE_RM_WORD_REGISTER(m, rl);
	DECODE_PRINTF(m, "\n");
	switch (rh) {
	case 0:
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = inc_word(m, *destreg);
	    break;
	case 1:
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    *destreg = dec_word(m, *destreg);
	    break;
	case 2:		/* call word ptr ... */
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, m->x86.R_IP);
	    m->x86.R_IP = *destreg;
	    break;
	case 3:		/* jmp far ptr ... */
	    DECODE_PRINTF(m, "OPERATION UNDEFINED 0XFF \n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    halt_sys(m);
	    break;

	case 4:		/* jmp  ... */
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    m->x86.R_IP = (u16) (*destreg);
	    break;
	case 5:		/* jmp far ptr ... */
	    DECODE_PRINTF(m, "OPERATION UNDEFINED 0XFF \n");
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    halt_sys(m);
	    break;
	case 6:
	    TRACE_REGS(m);
	    SINGLE_STEP(m);
	    push_word(m, *destreg);
	    break;
	}
	break;
    }
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


/*
 * Operation Table:
 */
void (*x86_optab[256])(SysEnv *m) =
{
/*  0x00 */ x86op_add_byte_RM_R,
/*  0x01 */ x86op_add_word_RM_R,
/*  0x02 */ x86op_add_byte_R_RM,
/*  0x03 */ x86op_add_word_R_RM,
/*  0x04 */ x86op_add_byte_AL_IMM,
/*  0x05 */ x86op_add_word_AX_IMM,
/*  0x06 */ x86op_push_ES,
/*  0x07 */ x86op_pop_ES,

/*  0x08 */ x86op_or_byte_RM_R,
/*  0x09 */ x86op_or_word_RM_R,
/*  0x0a */ x86op_or_byte_R_RM,
/*  0x0b */ x86op_or_word_R_RM,
/*  0x0c */ x86op_or_byte_AL_IMM,
/*  0x0d */ x86op_or_word_AX_IMM,
/*  0x0e */ x86op_push_CS,
/*  0x0f */ x86op_two_byte,

/*  0x10 */ x86op_adc_byte_RM_R,
/*  0x11 */ x86op_adc_word_RM_R,
/*  0x12 */ x86op_adc_byte_R_RM,
/*  0x13 */ x86op_adc_word_R_RM,
/*  0x14 */ x86op_adc_byte_AL_IMM,
/*  0x15 */ x86op_adc_word_AX_IMM,
/*  0x16 */ x86op_push_SS,
/*  0x17 */ x86op_pop_SS,

/*  0x18 */ x86op_sbb_byte_RM_R,
/*  0x19 */ x86op_sbb_word_RM_R,
/*  0x1a */ x86op_sbb_byte_R_RM,
/*  0x1b */ x86op_sbb_word_R_RM,
/*  0x1c */ x86op_sbb_byte_AL_IMM,
/*  0x1d */ x86op_sbb_word_AX_IMM,
/*  0x1e */ x86op_push_DS,
/*  0x1f */ x86op_pop_DS,

/*  0x20 */ x86op_and_byte_RM_R,
/*  0x21 */ x86op_and_word_RM_R,
/*  0x22 */ x86op_and_byte_R_RM,
/*  0x23 */ x86op_and_word_R_RM,
/*  0x24 */ x86op_and_byte_AL_IMM,
/*  0x25 */ x86op_and_word_AX_IMM,
/*  0x26 */ x86op_segovr_ES,
/*  0x27 */ x86op_daa,

/*  0x28 */ x86op_sub_byte_RM_R,
/*  0x29 */ x86op_sub_word_RM_R,
/*  0x2a */ x86op_sub_byte_R_RM,
/*  0x2b */ x86op_sub_word_R_RM,
/*  0x2c */ x86op_sub_byte_AL_IMM,
/*  0x2d */ x86op_sub_word_AX_IMM,
/*  0x2e */ x86op_segovr_CS,
/*  0x2f */ x86op_das,

/*  0x30 */ x86op_xor_byte_RM_R,
/*  0x31 */ x86op_xor_word_RM_R,
/*  0x32 */ x86op_xor_byte_R_RM,
/*  0x33 */ x86op_xor_word_R_RM,
/*  0x34 */ x86op_xor_byte_AL_IMM,
/*  0x35 */ x86op_xor_word_AX_IMM,
/*  0x36 */ x86op_segovr_SS,
/*  0x37 */ x86op_aaa,

/*  0x38 */ x86op_cmp_byte_RM_R,
/*  0x39 */ x86op_cmp_word_RM_R,
/*  0x3a */ x86op_cmp_byte_R_RM,
/*  0x3b */ x86op_cmp_word_R_RM,
/*  0x3c */ x86op_cmp_byte_AL_IMM,
/*  0x3d */ x86op_cmp_word_AX_IMM,
/*  0x3e */ x86op_segovr_DS,
/*  0x3f */ x86op_aas,

/*  0x40 */ x86op_inc_AX,
/*  0x41 */ x86op_inc_CX,
/*  0x42 */ x86op_inc_DX,
/*  0x43 */ x86op_inc_BX,
/*  0x44 */ x86op_inc_SP,
/*  0x45 */ x86op_inc_BP,
/*  0x46 */ x86op_inc_SI,
/*  0x47 */ x86op_inc_DI,

/*  0x48 */ x86op_dec_AX,
/*  0x49 */ x86op_dec_CX,
/*  0x4a */ x86op_dec_DX,
/*  0x4b */ x86op_dec_BX,
/*  0x4c */ x86op_dec_SP,
/*  0x4d */ x86op_dec_BP,
/*  0x4e */ x86op_dec_SI,
/*  0x4f */ x86op_dec_DI,

/*  0x50 */ x86op_push_AX,
/*  0x51 */ x86op_push_CX,
/*  0x52 */ x86op_push_DX,
/*  0x53 */ x86op_push_BX,
/*  0x54 */ x86op_push_SP,
/*  0x55 */ x86op_push_BP,
/*  0x56 */ x86op_push_SI,
/*  0x57 */ x86op_push_DI,

/*  0x58 */ x86op_pop_AX,
/*  0x59 */ x86op_pop_CX,
/*  0x5a */ x86op_pop_DX,
/*  0x5b */ x86op_pop_BX,
/*  0x5c */ x86op_pop_SP,
/*  0x5d */ x86op_pop_BP,
/*  0x5e */ x86op_pop_SI,
/*  0x5f */ x86op_pop_DI,

/*  0x60 */ x86op_push_all,
/*  0x61 */ x86op_pop_all,
/*  0x62 */ x86op_illegal_op,
/*  0x63 */ x86op_illegal_op,
/*  0x64 */ x86op_illegal_op,
/*  0x65 */ x86op_illegal_op,
/*  0x66 */ x86op_prefix_data,
/*  0x67 */ x86op_illegal_op,

/*  0x68 */ x86op_illegal_op,
/*  0x69 */ x86op_illegal_op,
/*  0x6a */ x86op_illegal_op,
/*  0x6b */ x86op_illegal_op,
/*  0x6c */ x86op_ins_byte,
/*  0x6d */ x86op_ins_word,
/*  0x6e */ x86op_outs_byte,
/*  0x6f */ x86op_outs_word,

/*  0x70 */ x86op_jump_near_O,
/*  0x71 */ x86op_jump_near_NO,
/*  0x72 */ x86op_jump_near_B,
/*  0x73 */ x86op_jump_near_NB,
/*  0x74 */ x86op_jump_near_Z,
/*  0x75 */ x86op_jump_near_NZ,
/*  0x76 */ x86op_jump_near_BE,
/*  0x77 */ x86op_jump_near_NBE,

/*  0x78 */ x86op_jump_near_S,
/*  0x79 */ x86op_jump_near_NS,
/*  0x7a */ x86op_jump_near_P,
/*  0x7b */ x86op_jump_near_NP,
/*  0x7c */ x86op_jump_near_L,
/*  0x7d */ x86op_jump_near_NL,
/*  0x7e */ x86op_jump_near_LE,
/*  0x7f */ x86op_jump_near_NLE,

/*  0x80 */ x86op_opc80_byte_RM_IMM,
/*  0x81 */ x86op_opc81_word_RM_IMM,
/*  0x82 */ x86op_opc82_byte_RM_IMM,
/*  0x83 */ x86op_opc83_word_RM_IMM,
/*  0x84 */ x86op_test_byte_RM_R,
/*  0x85 */ x86op_test_word_RM_R,
/*  0x86 */ x86op_xchg_byte_RM_R,
/*  0x87 */ x86op_xchg_word_RM_R,

/*  0x88 */ x86op_mov_byte_RM_R,
/*  0x89 */ x86op_mov_word_RM_R,
/*  0x8a */ x86op_mov_byte_R_RM,
/*  0x8b */ x86op_mov_word_R_RM,
/*  0x8c */ x86op_mov_word_RM_SR,
/*  0x8d */ x86op_lea_word_R_M,
/*  0x8e */ x86op_mov_word_SR_RM,
/*  0x8f */ x86op_pop_RM,

/*  0x90 */ x86op_nop,
/*  0x91 */ x86op_xchg_word_AX_CX,
/*  0x92 */ x86op_xchg_word_AX_DX,
/*  0x93 */ x86op_xchg_word_AX_BX,
/*  0x94 */ x86op_xchg_word_AX_SP,
/*  0x95 */ x86op_xchg_word_AX_BP,
/*  0x96 */ x86op_xchg_word_AX_SI,
/*  0x97 */ x86op_xchg_word_AX_DI,

/*  0x98 */ x86op_cbw,
/*  0x99 */ x86op_cwd,
/*  0x9a */ x86op_call_far_IMM,
/*  0x9b */ x86op_wait,
/*  0x9c */ x86op_pushf_word,
/*  0x9d */ x86op_popf_word,
/*  0x9e */ x86op_sahf,
/*  0x9f */ x86op_lahf,

/*  0xa0 */ x86op_mov_AL_M_IMM,
/*  0xa1 */ x86op_mov_AX_M_IMM,
/*  0xa2 */ x86op_mov_M_AL_IMM,
/*  0xa3 */ x86op_mov_M_AX_IMM,
/*  0xa4 */ x86op_movs_byte,
/*  0xa5 */ x86op_movs_word,
/*  0xa6 */ x86op_cmps_byte,
/*  0xa7 */ x86op_cmps_word,
/*  0xa8 */ x86op_test_AL_IMM,
/*  0xa9 */ x86op_test_AX_IMM,
/*  0xaa */ x86op_stos_byte,
/*  0xab */ x86op_stos_word,
/*  0xac */ x86op_lods_byte,
/*  0xad */ x86op_lods_word,
/*  0xac */ x86op_scas_byte,
/*  0xad */ x86op_scas_word,


/*  0xb0 */ x86op_mov_byte_AL_IMM,
/*  0xb1 */ x86op_mov_byte_CL_IMM,
/*  0xb2 */ x86op_mov_byte_DL_IMM,
/*  0xb3 */ x86op_mov_byte_BL_IMM,
/*  0xb4 */ x86op_mov_byte_AH_IMM,
/*  0xb5 */ x86op_mov_byte_CH_IMM,
/*  0xb6 */ x86op_mov_byte_DH_IMM,
/*  0xb7 */ x86op_mov_byte_BH_IMM,

/*  0xb8 */ x86op_mov_word_AX_IMM,
/*  0xb9 */ x86op_mov_word_CX_IMM,
/*  0xba */ x86op_mov_word_DX_IMM,
/*  0xbb */ x86op_mov_word_BX_IMM,
/*  0xbc */ x86op_mov_word_SP_IMM,
/*  0xbd */ x86op_mov_word_BP_IMM,
/*  0xbe */ x86op_mov_word_SI_IMM,
/*  0xbf */ x86op_mov_word_DI_IMM,


/*  0xc0 */ x86op_opcC0_byte_RM_MEM,
/*  0xc1 */ x86op_opcC1_word_RM_MEM,
/*  0xc2 */ x86op_ret_near_IMM,
/*  0xc3 */ x86op_ret_near,
/*  0xc4 */ x86op_les_R_IMM,
/*  0xc5 */ x86op_lds_R_IMM,
/*  0xc6 */ x86op_mov_byte_RM_IMM,
/*  0xc7 */ x86op_mov_word_RM_IMM,
/*  0xc8 */ x86op_illegal_op,
/*  0xc9 */ x86op_illegal_op,
/*  0xca */ x86op_ret_far_IMM,
/*  0xcb */ x86op_ret_far,
/*  0xcc */ x86op_int3,
/*  0xcd */ x86op_int_IMM,
/*  0xce */ x86op_into,
/*  0xcf */ x86op_iret,

/*  0xd0 */ x86op_opcD0_byte_RM_1,
/*  0xd1 */ x86op_opcD1_word_RM_1,
/*  0xd2 */ x86op_opcD2_byte_RM_CL,
/*  0xd3 */ x86op_opcD3_word_RM_CL,
/*  0xd4 */ x86op_aam,
/*  0xd5 */ x86op_aad,
/*  0xd6 */ x86op_illegal_op,
/*  0xd7 */ x86op_xlat,
/*  0xd8 */ x86op_esc_coprocess_d8,
/*  0xd9 */ x86op_esc_coprocess_d9,
/*  0xda */ x86op_esc_coprocess_da,
/*  0xdb */ x86op_esc_coprocess_db,
/*  0xdc */ x86op_esc_coprocess_dc,
/*  0xdd */ x86op_esc_coprocess_dd,
/*  0xde */ x86op_esc_coprocess_de,
/*  0xdf */ x86op_esc_coprocess_df,

/*  0xe0 */ x86op_loopne,
/*  0xe1 */ x86op_loope,
/*  0xe2 */ x86op_loop,
/*  0xe3 */ x86op_jcxz,
/*  0xe4 */ x86op_in_byte_AL_IMM,
/*  0xe5 */ x86op_in_word_AX_IMM,
/*  0xe6 */ x86op_out_byte_IMM_AL,
/*  0xe7 */ x86op_out_word_IMM_AX,

/*  0xe8 */ x86op_call_near_IMM,
/*  0xe9 */ x86op_jump_near_IMM,
/*  0xea */ x86op_jump_far_IMM,
/*  0xeb */ x86op_jump_byte_IMM,
/*  0xec */ x86op_in_byte_AL_DX,
/*  0xed */ x86op_in_word_AX_DX,
/*  0xee */ x86op_out_byte_DX_AL,
/*  0xef */ x86op_out_word_DX_AX,

/*  0xf0 */ x86op_lock,
/*  0xf1 */ x86op_illegal_op,
/*  0xf2 */ x86op_repne,
/*  0xf3 */ x86op_repe,
/*  0xf4 */ x86op_halt,
/*  0xf5 */ x86op_cmc,
/*  0xf6 */ x86op_opcF6_byte_RM,
/*  0xf7 */ x86op_opcF7_word_RM,

/*  0xf8 */ x86op_clc,
/*  0xf9 */ x86op_stc,
/*  0xfa */ x86op_cli,
/*  0xfb */ x86op_sti,
/*  0xfc */ x86op_cld,
/*  0xfd */ x86op_std,
/*  0xfe */ x86op_opcFE_byte_RM,
/*  0xff */ x86op_opcFF_word_RM,
};


int x86_optab_len = sizeof(x86_optab) / sizeof(x86_optab);
