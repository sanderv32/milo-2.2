
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
 * This file includes subroutines which are related to instruction
 * decoding and accessess of immediate data via IP.  etc.
 */

#include "x86_regs.h"
#include "x86_decode.h"
#include "x86_debug.h"
#include "x86_ops.h"
#include "x86_prim_ops.h"

#define NULL 0

static void x86_intr_handle(SysEnv * m)
{
	u16 tmp;
	u8 intno;

	if (m->x86.intr & INTR_SYNCH) {
		intno = m->x86.intno;

		tmp = m->x86.R_FLG;
		push_word(m, tmp);
		CLEAR_FLAG(m, F_IF);
		CLEAR_FLAG(m, F_TF);
		push_word(m, m->x86.R_CS);
		tmp = mem_access_word(m, intno * 4);
		m->x86.R_CS = tmp;
		push_word(m, m->x86.R_IP);
		tmp = mem_access_word(m, intno * 4 + 2);
		m->x86.R_IP = tmp;
	}
}


void x86_intr_raise(SysEnv * m, u8 intrnum)
{
	m->x86.intno = intrnum;
	m->x86.intr |= INTR_SYNCH;
}


/*
 * Main Execution Loop.
 */
void x86_exec(SysEnv * m)
{
	u8 op1;

	m->x86.intr = 0;
	for (;;) {

#ifdef DEBUG
		if (CHECK_IP_FETCH(m))
			x86_check_ip_access(m);
#endif

		/* if debugging, save the IP and CS values. */
		SAVE_IP_CS(m, m->x86.R_CS, m->x86.R_IP);
		INC_DECODED_INST_LEN(m, 1);

		if (m->x86.intr) {
			if (m->x86.intr & INTR_HALTED) {
				x86_debug_printf(m, "halted\n");
#ifdef DEBUG
				x86_trace_regs(m);
#endif
				return;
			}
			if (((m->x86.intr & INTR_SYNCH) &&
			     (m->x86.intno == 0 || m->x86.intno == 2)) ||
			    !ACCESS_FLAG(m, F_IF)) {
				/*
				 * i.e. either not maskable (intr 0 or 2) or the IF
				 * flag not set so interrupts are not masked.
				 */
				x86_intr_handle(m);
			}
		}
		op1 =
		    sys_rdb(m, ((u32) m->x86.R_CS << 4) + (m->x86.R_IP++));
		(*x86_optab[op1]) (m);
	}
}


void halt_sys(SysEnv * m)
{
	m->x86.intr |= INTR_HALTED;
}


void fetch_decode_modrm(SysEnv * m, u16 * mod, u16 * regh, u16 * regl)
{
	u8 fetched;

#ifdef DEBUG
	if (CHECK_IP_FETCH(m))
		x86_check_ip_access(m);
#endif

	/*
	 * Do the fetch in real mode.  Shift the CS segment register over
	 * by 4 bits, and add in the IP register.  Index into the system
	 * memory.
	 */
	fetched = sys_rdb(m, (m->x86.R_CS << 4) + (m->x86.R_IP++));
	INC_DECODED_INST_LEN(m, 1);
	*mod = (fetched >> 6) & 0x03;
	*regh = (fetched >> 3) & 0x07;
	*regl = (fetched >> 0) & 0x07;
}

/*
 * Return a pointer to the register given by the R/RM field of the
 * modrm byte, for byte operands.  Also enables the decoding of
 * instructions.
 */
u8 *decode_rm_byte_register(SysEnv * m, int reg)
{
	switch (reg) {
	case 0:
		DECODE_PRINTF(m, "AL");
		return &m->x86.R_AL;
		break;
	case 1:
		DECODE_PRINTF(m, "CL");
		return &m->x86.R_CL;
		break;
	case 2:
		DECODE_PRINTF(m, "DL");
		return &m->x86.R_DL;
		break;
	case 3:
		DECODE_PRINTF(m, "BL");
		return &m->x86.R_BL;
		break;
	case 4:
		DECODE_PRINTF(m, "AH");
		return &m->x86.R_AH;
		break;
	case 5:
		DECODE_PRINTF(m, "CH");
		return &m->x86.R_CH;
		break;
	case 6:
		DECODE_PRINTF(m, "DH");
		return &m->x86.R_DH;
		break;
	case 7:
		DECODE_PRINTF(m, "BH");
		return &m->x86.R_BH;
		break;
	}
	halt_sys(m);
	return NULL;		/* NOT REACHED OR REACHED ON ERROR */
}

/*
 * Return a pointer to the register given by the R/RM field of the
 * modrm byte, for word operands.  Also enables the decoding of
 * instructions.
 */
u16 *decode_rm_word_register(SysEnv * m, int reg)
{
	switch (reg) {
	case 0:
		DECODE_PRINTF(m, "AX");
		return &m->x86.R_AX;
		break;
	case 1:
		DECODE_PRINTF(m, "CX");
		return &m->x86.R_CX;
		break;
	case 2:
		DECODE_PRINTF(m, "DX");
		return &m->x86.R_DX;
		break;
	case 3:
		DECODE_PRINTF(m, "BX");
		return &m->x86.R_BX;
		break;
	case 4:
		DECODE_PRINTF(m, "SP");
		return &m->x86.R_SP;
		break;
	case 5:
		DECODE_PRINTF(m, "BP");
		return &m->x86.R_BP;
		break;
	case 6:
		DECODE_PRINTF(m, "SI");
		return &m->x86.R_SI;
		break;
	case 7:
		DECODE_PRINTF(m, "DI");
		return &m->x86.R_DI;
		break;
	}
	halt_sys(m);
	return NULL;		/* NOTREACHED OR REACHED ON ERROR */
}


u32 *decode_rm_long_register(SysEnv * m, int reg)
{
	switch (reg) {
	case 0:
		DECODE_PRINTF(m, "EAX");
		return &m->x86.R_EAX;
		break;
	case 1:
		DECODE_PRINTF(m, "ECX");
		return &m->x86.R_ECX;
		break;
	case 2:
		DECODE_PRINTF(m, "EDX");
		return &m->x86.R_EDX;
		break;
	case 3:
		DECODE_PRINTF(m, "EBX");
		return &m->x86.R_EBX;
		break;
	case 4:
		DECODE_PRINTF(m, "ESP");
		return &m->x86.R_ESP;
		break;
	case 5:
		DECODE_PRINTF(m, "EBP");
		return &m->x86.R_EBP;
		break;
	case 6:
		DECODE_PRINTF(m, "ESI");
		return &m->x86.R_ESI;
		break;
	case 7:
		DECODE_PRINTF(m, "EDI");
		return &m->x86.R_EDI;
		break;
	}
	halt_sys(m);
	return NULL;		/* NOTREACHED OR REACHED ON ERROR */
}


/*
 * Return a pointer to the register given by the R/RM field of the
 * modrm byte, for word operands, modified from above for the weirdo
 * special case of segreg operands.  Also enables the decoding of
 * instructions.
 */
u16 *decode_rm_seg_register(SysEnv * m, int reg)
{
	switch (reg) {
	case 0:
		DECODE_PRINTF(m, "ES");
		return &m->x86.R_ES;
		break;
	case 1:
		DECODE_PRINTF(m, "CS");
		return &m->x86.R_CS;
		break;
	case 2:
		DECODE_PRINTF(m, "SS");
		return &m->x86.R_SS;
		break;
	case 3:
		DECODE_PRINTF(m, "DS");
		return &m->x86.R_DS;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		DECODE_PRINTF(m, "ILLEGAL SEGREG");
		break;
	}
	halt_sys(m);
	return NULL;		/* NOT REACHED OR REACHED ON ERROR */
}


/*
 * Once the instruction is fetched, an optional byte follows which has
 * 3 fields encoded in it.  This routine fetches the byte and breaks
 * into the three fields.
 */
u8 fetch_byte_imm(SysEnv * m)
{
	u8 fetched;

#ifdef DEBUG
	if (CHECK_IP_FETCH(m))
		x86_check_ip_access(m);
#endif

	/* do the fetch in real mode.  Shift the CS segment register
	   over by 4 bits, and add in the IP register.  Index into 
	   the system memory.  
	 */

	fetched = sys_rdb(m, ((u32) m->x86.R_CS << 4) + (m->x86.R_IP++));
	INC_DECODED_INST_LEN(m, 1);
	return fetched;
}


u16 fetch_word_imm(SysEnv * m)
{
	u16 fetched;

#ifdef DEBUG
	if (CHECK_IP_FETCH(m))
		x86_check_ip_access(m);
#endif

	/* do the fetch in real mode.  Shift the CS segment register
	   over by 4 bits, and add in the IP register.  Index into 
	   the system SysEnvory.  
	 */
	fetched = sys_rdw(m, ((u32) m->x86.R_CS << 4) + (m->x86.R_IP));
	m->x86.R_IP += 2;
	INC_DECODED_INST_LEN(m, 2);

	return fetched;
}


u32 fetch_long_imm(SysEnv * m)
{
	u32 fetched;

#ifdef DEBUG
	if (CHECK_IP_FETCH(m))
		x86_check_ip_access(m);
#endif

	/* do the fetch in real mode.  Shift the CS segment register
	   over by 4 bits, and add in the IP register.  Index into 
	   the system SysEnvory.  
	 */
	fetched = sys_rdl(m, ((u32) m->x86.R_CS << 4) + (m->x86.R_IP));
	m->x86.R_IP += 4;
	INC_DECODED_INST_LEN(m, 4);

	return fetched;
}


/*
 * Return the offset given by mod=00 addressing.  Also enables the
 * decoding of instructions.
 */
u16 decode_rm00_address(SysEnv * m, int rm)
{
	u16 offset;

	/* note the code which specifies the corresponding segment (ds vs ss)
	   below in the case of [BP+..].  The assumption here is that at the 
	   point that this subroutine is called, the bit corresponding to
	   SYSMODE_SEG_DS_SS will be zero.  After every instruction 
	   except the segment override instructions, this bit (as well
	   as any bits indicating segment overrides) will be clear.  So
	   if a SS access is needed, set this bit.  Otherwise, DS access 
	   occurs (unless any of the segment override bits are set).
	 */
	switch (rm) {
	case 0:
		DECODE_PRINTF(m, "[BX+SI]");
		return (s16) m->x86.R_BX + (s16) m->x86.R_SI;
		break;
	case 1:
		DECODE_PRINTF(m, "[BX+DI]");
		return (s16) m->x86.R_BX + (s16) m->x86.R_DI;
		break;
	case 2:
		DECODE_PRINTF(m, "[BP+SI]");
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_SI;
		break;
	case 3:
		DECODE_PRINTF(m, "[BP+DI]");
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_DI;
		break;
	case 4:
		DECODE_PRINTF(m, "[SI]");
		return m->x86.R_SI;
		break;
	case 5:
		DECODE_PRINTF(m, "[DI]");
		return m->x86.R_DI;
		break;
	case 6:
		offset = (s16) fetch_word_imm(m);
		DECODE_PRINTF2(m, "[%04x]", offset);
		return offset;
		break;
	case 7:
		DECODE_PRINTF(m, "[BX]");
		return m->x86.R_BX;
	}
	halt_sys(m);
	return 0;
}


/*
 * Return the offset given by mod=01 addressing.  Also enables the
 * decoding of instructions.
 */
u16 decode_rm01_address(SysEnv * m, int rm)
{
	s8 displacement;

	/* note comment on decode_rm00_address above */

	displacement = (s8) fetch_byte_imm(m);	/* !!!! Check this */

	switch (rm) {
	case 0:
		DECODE_PRINTF2(m, "%d[BX+SI]", (int) displacement);
		return (s16) m->x86.R_BX + (s16) m->x86.R_SI +
		    displacement;
		break;
	case 1:
		DECODE_PRINTF2(m, "%d[BX+DI]", (int) displacement);
		return (s16) m->x86.R_BX + (s16) m->x86.R_DI +
		    displacement;
		break;
	case 2:
		DECODE_PRINTF2(m, "%d[BP+SI]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_SI +
		    displacement;
		break;
	case 3:
		DECODE_PRINTF2(m, "%d[BP+DI]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_DI +
		    displacement;
		break;
	case 4:
		DECODE_PRINTF2(m, "%d[SI]", (int) displacement);
		return (s16) m->x86.R_SI + displacement;
		break;
	case 5:
		DECODE_PRINTF2(m, "%d[DI]", (int) displacement);
		return (s16) m->x86.R_DI + displacement;
		break;
	case 6:
		DECODE_PRINTF2(m, "%d[BP]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + displacement;
		break;
	case 7:
		DECODE_PRINTF2(m, "%d[BX]", (int) displacement);
		return (s16) m->x86.R_BX + displacement;
		break;
	}
	halt_sys(m);
	return 0;		/* SHOULD NOT HAPPEN */
}


/*
 * Return the offset given by mod=01 addressing.  Also enables the
 * decoding of instructions.
 */
u16 decode_rm10_address(SysEnv * m, int rm)
{
	s16 displacement;

	/* note comment on decode_rm00_address above */

	displacement = (s16) fetch_word_imm(m);

	switch (rm) {
	case 0:
		DECODE_PRINTF2(m, "%d[BX+SI]", (int) displacement);
		return (s16) m->x86.R_BX + (s16) m->x86.R_SI +
		    displacement;
		break;
	case 1:
		DECODE_PRINTF2(m, "%d[BX+DI]", (int) displacement);
		return (s16) m->x86.R_BX + (s16) m->x86.R_DI +
		    displacement;
		break;
	case 2:
		DECODE_PRINTF2(m, "%d[BP+SI]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_SI +
		    displacement;
		break;
	case 3:
		DECODE_PRINTF2(m, "%d[BP+DI]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + (s16) m->x86.R_DI +
		    displacement;
		break;
	case 4:
		DECODE_PRINTF2(m, "%d[SI]", (int) displacement);
		return (s16) m->x86.R_SI + displacement;
		break;
	case 5:
		DECODE_PRINTF2(m, "%d[DI]", (int) displacement);
		return (s16) m->x86.R_DI + displacement;
		break;
	case 6:
		DECODE_PRINTF2(m, "%d[BP]", (int) displacement);
		m->x86.mode |= SYSMODE_SEG_DS_SS;
		return (s16) m->x86.R_BP + displacement;
		break;
	case 7:
		DECODE_PRINTF2(m, "%d[BX]", (int) displacement);
		return (s16) m->x86.R_BX + displacement;
		break;
	}
	halt_sys(m);
	return 0;
	/*NOTREACHED */
}


/*
 * Fetch a byte of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
u8 fetch_data_byte(SysEnv * m, u16 offset)
{
	register u8 value;

	/*
	 * This code was originally completely broken, and never showed up
	 * since the DS segments === SS segment in all test cases.  It had
	 * been originally assumed, that all access to data would involve
	 * the DS register unless there was a segment override.  Not so.
	 * Address modes such as -3[BP] or 10[BP+SI] all refer to
	 * addresses relative to the SS.  So, at the minimum, all
	 * decodings of addressing modes would have to set/clear a bit
	 * describing whether the access is relative to DS or SS.  That is
	 * the function of the cpu-state-varible m->x86.mode.  There are
	 * several potential states:
	 *
	 * repe prefix seen  (handled elsewhere)
	 * repne prefix seen  (ditto)
	 *
	 * cs segment override
	 * ds segment override
	 * es segment override
	 * ss segment override
	 *
	 * ds/ss select (in absense of override)
	 *
	 * Each of the above 7 items are handled with a bit in the mode
	 * field.
	 *
	 * The latter 5 can be implemented as a simple state machine:
	 */
	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdb(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdb(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		value = sys_rdb(m, ((u32) m->x86.R_CS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdb(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif

		value = sys_rdb(m, ((u32) m->x86.R_ES << 4) + offset);
		break;

	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdb(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen: multiple overrides.\n");
		value = 0;
		halt_sys(m);
	}
	return value;
}


/*
 * Fetch a byte of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
u8 fetch_data_byte_abs(SysEnv * m, u16 segment, u16 offset)
{
	register u8 value;

#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif

	/* note, cannot change this, since we do not know the ID of the segment. */

	value = sys_rdb(m, (segment << 4) + offset);
	return value;
}


/*
 * Fetch a byte of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
u16 fetch_data_word(SysEnv * m, u16 offset)
{
	u16 value;
	/* See note above in fetch_data_byte. */

	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_CS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_ES << 4) + offset);
		break;


	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdw(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen: multiple overrides.\n");
		value = 0;
		halt_sys(m);
	}
	return value;
}


/*
 * Fetch a byte of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
u16 fetch_data_word_abs(SysEnv * m, u16 segment, u16 offset)
{
	u16 value;

#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif
	value = sys_rdw(m, (segment << 4) + offset);
	return value;
}


u32 fetch_data_long(SysEnv * m, u16 offset)
{
	u32 value;
	/* See note above in fetch_data_byte. */

	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_CS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_DS << 4) + offset);
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_ES << 4) + offset);
		break;

	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		value = sys_rdl(m, ((u32) m->x86.R_SS << 4) + offset);
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen: multiple overrides.\n");
		value = 0;
		halt_sys(m);
	}
	return value;
}


/*
 * Fetch a long of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
u32 fetch_data_long_abs(SysEnv * m, u16 segment, u16 offset)
{
	u32 value;

#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif
	value = sys_rdl(m, (segment << 4) + offset);
	return value;
}


/*
 * Store a byte of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
void store_data_byte(SysEnv * m, u16 offset, u8 val)
{
	/* See note above in fetch_data_byte. */
	u32 addr;
	register u16 segment;

	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;


	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		segment = m->x86.R_CS;
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif
		segment = m->x86.R_ES;
		break;

	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen: multiple overrides.\n");
		segment = 0;
		halt_sys(m);
	}
	addr = ((u32) segment << 4) + offset;
	sys_wrb(m, addr, val);
}


void store_data_byte_abs(SysEnv * m, u16 segment, u16 offset, u8 val)
{
	register u32 addr;
#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif
	addr = ((u32) segment << 4) + offset;
	sys_wrb(m, addr, val);
}


/*
 * Store a word of data, given an offset, the current register set,
 * and a descriptor for memory.
 */
void store_data_word(SysEnv * m, u16 offset, u16 val)
{
	register u32 addr;
	register u16 segment;
	/* See note above in fetch_data_byte. */

	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;

	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		segment = m->x86.R_CS;
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif
		segment = m->x86.R_ES;
		break;

	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen:  multiple overrides.\n");
		segment = 0;
		halt_sys(m);
	}
	addr = (segment << 4) + offset;
	sys_wrw(m, addr, val);
}


void store_data_long(SysEnv * m, u16 offset, u32 val)
{
	register u32 addr;
	register u16 segment;

	switch (m->x86.mode & SYSMODE_SEGMASK) {
	case 0:
		/* default case: use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEG_DS_SS:
		/* non-overridden, use ss register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;

	case SYSMODE_SEGOVR_CS:
		/* ds overridden */
	case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use cs register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_CS, offset);
#endif
		segment = m->x86.R_CS;
		break;

	case SYSMODE_SEGOVR_DS:
		/* ds overridden --- shouldn't happen, but hey. */
	case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ds register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_DS, offset);
#endif
		segment = m->x86.R_DS;
		break;

	case SYSMODE_SEGOVR_ES:
		/* ds overridden */
	case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		/* ss overridden, use es register */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_ES, offset);
#endif
		segment = m->x86.R_ES;
		break;

	case SYSMODE_SEGOVR_SS:
		/* ds overridden */
	case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		/* ss overridden, use ss register === should not happen */
#ifdef DEBUG
		if (CHECK_DATA_ACCESS(m))
			x86_check_data_access(m, m->x86.R_SS, offset);
#endif
		segment = m->x86.R_SS;
		break;

	default:
		x86_debug_printf(m,
				 "error: should not happen:  multiple overrides.\n");
		segment = 0;
		halt_sys(m);
	}
	addr = (segment << 4) + offset;
	sys_wrl(m, addr, val);
}


void store_data_word_abs(SysEnv * m, u16 segment, u16 offset, u16 val)
{
	register u32 addr;
#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif
	addr = (segment << 4) + offset;
	sys_wrw(m, addr, val);
}


void store_data_long_abs(SysEnv * m, u16 segment, u16 offset, u32 val)
{
	register u32 addr;
#ifdef DEBUG
	if (CHECK_DATA_ACCESS(m))
		x86_check_data_access(m, segment, offset);
#endif
	addr = (segment << 4) + offset;
	sys_wrl(m, addr, val);
}
