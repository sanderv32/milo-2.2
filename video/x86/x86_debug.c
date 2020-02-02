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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "sysenv.h"
#include "host.h"

#include "x86_regs.h"
#include "x86_decode.h"
#include "x86_ops.h"
#include "x86_debug.h"

extern int printk(const char *fmt, ...);
extern void cons_gets(char *buf);
#ifdef DEBUG

static void print_encoded_bytes(SysEnv * m, u16 s, u16 o);
static void print_decoded_instruction(SysEnv * m);
static void print_ivectab(SysEnv * m);
static void disassemble_forward(SysEnv * m, u16 seg, u16 off, int n);
static int parse_line(char *s, int *ps, int *n);


/* should look something like debug's output. */
void x86_trace_regs(SysEnv * m)
{
	if (DEBUG_TRACE(m)) {
		x86_dump_regs(m);
	}
	if (DEBUG_DECODE(m)) {
		x86_debug_printf(m, "%04x:%04x ", m->x86.saved_cs,
				 m->x86.saved_ip);
		print_encoded_bytes(m, m->x86.saved_cs, m->x86.saved_ip);
		print_decoded_instruction(m);
	}
}


void x86_just_disassemble(SysEnv * m)
{
	/*
	 * This routine called if the flag DEBUG_DISASSEMBLE is set kind
	 * of a hack!
	 */
	x86_debug_printf(m, "%04x:%04x ", m->x86.saved_cs,
			 m->x86.saved_ip);
	print_encoded_bytes(m, m->x86.saved_cs, m->x86.saved_ip);
	print_decoded_instruction(m);
}


static void disassemble_forward(SysEnv * m, u16 seg, u16 off, int n)
{
	SysEnv tregs;
	int i, op1;
	/*
	 * hack, hack, hack.  What we do is use the exact machinery set up
	 * for execution, except that now there is an additional state
	 * flag associated with the "execution", and we are using a copy
	 * of the register struct.  All the major opcodes, once fully
	 * decoded, have the following two steps: TRACE_REGS(r,m);
	 * SINGLE_STEP(r,m); which disappear if DEBUG is not defined to
	 * the preprocessor.  The TRACE_REGS macro expands to:
	 *
	 * if (debug&DEBUG_DISASSEMBLE) 
	 *     {just_disassemble(); goto EndOfInstruction;}
	 *     if (debug&DEBUG_TRACE) trace_regs(r,m);
	 *
	 * ......  and at the last line of the routine. 
	 *
	 * EndOfInstruction: end_instr();
	 *
	 * Up to the point where TRACE_REG is expanded, NO modifications
	 * are done to any register EXCEPT the IP register, for fetch and
	 * decoding purposes.
	 *
	 * This was done for an entirely different reason, but makes a
	 * nice way to get the system to help debug codes.
	 */
	tregs = *m;
	tregs.x86.R_IP = off;
	tregs.x86.R_CS = seg;

	/* reset the decoding buffers */
	tregs.x86.enc_str_pos = 0;
	tregs.x86.enc_pos = 0;

	/* turn on the "disassemble only, no execute" flag */
	tregs.x86.debug |= DEBUG_DISASSEMBLE_F;

	/* DUMP NEXT n instructions to screen in straight_line fashion */
	/*
	 * This looks like the regular instruction fetch stream, except
	 * that when this occurs, each fetched opcode, upon seeing the
	 * DEBUG_DISASSEMBLE flag set, exits immediately after decoding
	 * the instruction.  XXX --- CHECK THAT MEM IS NOT AFFECTED!!!
	 * Note the use of a copy of the register structure...
	 */
	for (i = 0; i < n; i++) {
		op1 =
		    sys_rdb(&tregs,
			    ((u32) m->x86.R_CS << 4) + (m->x86.R_IP++));
		(x86_optab[op1]) (&tregs);
	}
	/* end major hack mode. */
}


void x86_check_ip_access(SysEnv * m)
{
	/* NULL as of now */
}


void x86_check_sp_access(SysEnv * m)
{
}


void x86_check_mem_access(SysEnv * m, u32 p)
{
	/*  check bounds, etc */
}


void x86_check_data_access(SysEnv * m, u16 s, u16 o)
{
	/*  check bounds, etc */
}


void x86_inc_decoded_inst_len(SysEnv * m, int x)
{
	m->x86.enc_pos += x;
}


void x86_decode_printf(SysEnv * m, char *x)
{
	sprintf(m->x86.decoded_buf + m->x86.enc_str_pos, "%s", x);
	m->x86.enc_str_pos += strlen(x);
}


void x86_decode_printf2(SysEnv * m, char *x, int y)
{
	char temp[100];
	sprintf(temp, x, y);
	sprintf(m->x86.decoded_buf + m->x86.enc_str_pos, "%s", temp);
	m->x86.enc_str_pos += strlen(temp);
}


void x86_end_instr(SysEnv * m)
{
	m->x86.enc_str_pos = 0;
	m->x86.enc_pos = 0;
}


static void print_encoded_bytes(SysEnv * m, u16 s, u16 o)
{
	int i;
	char buf1[64];
	for (i = 0; i < m->x86.enc_pos; i++) {
		sprintf(buf1 + 2 * i, "%02x",
			fetch_data_byte_abs(m, s, o + i));
	}
	x86_debug_printf(m, "%-20s", buf1);
}


static void print_decoded_instruction(SysEnv * m)
{
	x86_debug_printf(m, "%s", m->x86.decoded_buf);
}


static void print_ivectab(SysEnv * m)
{
	int iv;
	u32 i, j;

	iv = 0;
	for (i = 0; i < 256 / 4; i++) {
		for (j = 0; j < 4; j++, iv++)
			x86_print_int_vect(m, iv);
		x86_debug_printf(m, "\n");
	}
}


void x86_print_int_vect(SysEnv * m, u16 iv)
{
	u16 seg, off;

	if (iv > 256)
		return;
	seg = fetch_data_word_abs(m, 0, iv * 4);
	off = fetch_data_word_abs(m, 0, iv * 4 + 2);
	x86_debug_printf(m, "%04x:%04x ", seg, off);
}


void x86_dump_memory(SysEnv * m, u16 seg, u16 off, u32 amt)
{
	u32 start = off & 0xfffffff0;
	u32 end = (off + 16) & 0xfffffff0;
	u32 i;
	u32 current;

	current = start;
	while (end <= off + amt) {
		x86_debug_printf(m, "%04x:%04x ", seg, start);
		for (i = start; i < off; i++)
			x86_debug_printf(m, "   ");
		for (; i < end; i++)
			x86_debug_printf(m, "%02x ",
					 fetch_data_byte_abs(m, seg, i));
		x86_debug_printf(m, "\n");
		start = end;
		end = start + 16;
	}
}


void x86_single_step(SysEnv * m)
{
	char s[1024];
	int ps[10];
	int ntok;
	int cmd;
	int done;
	int offset;
	static int breakpoint;
	char *p;

	if (DEBUG_BREAK(m))
		if (m->x86.saved_ip != breakpoint)
			return;
		else {
			m->x86.debug |= DEBUG_TRACE_F;
			m->x86.debug &= ~DEBUG_BREAK_F;
			x86_trace_regs(m);
		}
	done = 0;
	offset = m->x86.saved_ip;
	while (!done) {
		printk("-");
		p = cons_gets(s);
		cmd = parse_line(s, ps, &ntok);
		switch (cmd) {
		case 'u':
			disassemble_forward(m, m->x86.saved_cs, offset,
					    10);
			break;
		case 'd':
			if (ntok == 2) {
				offset = ps[1];
				x86_dump_memory(m, m->x86.saved_cs, offset,
						16);
				offset += 16;
			} else {
				x86_dump_memory(m, m->x86.saved_cs, offset,
						16);
				offset += 16;
			}
			break;
		case 'c':
			m->x86.debug ^= DEBUG_TRACECALL_F;
			break;
		case 's':
			m->x86.debug ^=
			    DEBUG_SVC_F | DEBUG_SYS_F | DEBUG_SYSINT_F;
			break;
		case 'r':
			x86_trace_regs(m);
			break;
		case 'g':
			if (ntok == 2) {
				breakpoint = ps[1];
				m->x86.debug &= ~DEBUG_TRACE_F;
				m->x86.debug |= DEBUG_BREAK_F;
				done = 1;
			}
			break;
		case 'q':
			return(1);
		case 't':
		case 0:
			done = 1;
			break;
		}
	}
}


int x86_trace_on(SysEnv * m)
{
	return m->x86.debug |=
	    DEBUG_STEP_F | DEBUG_DECODE_F | DEBUG_TRACE_F;
}


int x86_trace_off(SysEnv * m)
{
	return m->x86.debug &=
	    ~(DEBUG_STEP_F | DEBUG_DECODE_F | DEBUG_TRACE_F);
}


static int parse_line(char *s, int *ps, int *n)
{
	int cmd;

	*n = 0;
	while (*s == ' ' || *s == '\t')
		s++;
	ps[*n] = *s;
	switch (*s) {
	case '\n':
		*n += 1;
		return 0;
	default:
		cmd = *s;
		*n += 1;
	}

	while (*s != ' ' && *s != '\t' && *s != '\n')
		s++;

	if (*s == '\n')
		return cmd;

	while (*s == ' ' || *s == '\t')
		s++;

	ps[*n]=atoi(s);
	*n += 1;
	return cmd;
}

#endif				/* DEBUG */

void x86_dump_regs(SysEnv * m)
{
	x86_debug_printf(m, "AX=%04x  ", m->x86.R_AX);
	x86_debug_printf(m, "BX=%04x  ", m->x86.R_BX);
	x86_debug_printf(m, "CX=%04x  ", m->x86.R_CX);
	x86_debug_printf(m, "DX=%04x  ", m->x86.R_DX);
	x86_debug_printf(m, "SP=%04x  ", m->x86.R_SP);
	x86_debug_printf(m, "BP=%04x  ", m->x86.R_BP);
	x86_debug_printf(m, "SI=%04x  ", m->x86.R_SI);
	x86_debug_printf(m, "DI=%04x\n", m->x86.R_DI);
	x86_debug_printf(m, "DS=%04x  ", m->x86.R_DS);
	x86_debug_printf(m, "ES=%04x  ", m->x86.R_ES);
	x86_debug_printf(m, "SS=%04x  ", m->x86.R_SS);
	x86_debug_printf(m, "CS=%04x  ", m->x86.R_CS);
	x86_debug_printf(m, "IP=%04x   ", m->x86.R_IP);
	if (ACCESS_FLAG(m, F_OF))
		x86_debug_printf(m, "OV ");	/* CHECKED... */
	else
		x86_debug_printf(m, "NV ");
	if (ACCESS_FLAG(m, F_DF))
		x86_debug_printf(m, "DN ");
	else
		x86_debug_printf(m, "UP ");
	if (ACCESS_FLAG(m, F_IF))
		x86_debug_printf(m, "EI ");
	else
		x86_debug_printf(m, "DI ");
	if (ACCESS_FLAG(m, F_SF))
		x86_debug_printf(m, "NG ");
	else
		x86_debug_printf(m, "PL ");
	if (ACCESS_FLAG(m, F_ZF))
		x86_debug_printf(m, "ZR ");
	else
		x86_debug_printf(m, "NZ ");
	if (ACCESS_FLAG(m, F_AF))
		x86_debug_printf(m, "AC ");
	else
		x86_debug_printf(m, "NA ");
	if (ACCESS_FLAG(m, F_PF))
		x86_debug_printf(m, "PE ");
	else
		x86_debug_printf(m, "PO ");
	if (ACCESS_FLAG(m, F_CF))
		x86_debug_printf(m, "CY ");
	else
		x86_debug_printf(m, "NC ");
	x86_debug_printf(m, "\n");
}


void x86_debug_printf();	/* reset prototype */

void
x86_debug_printf(SysEnv * m, const char *format, long a0, long a1, long a2,
		 long a3, long a4, long a5)
{
	/*
	 * Ugly hack to make this work both with printk.  vfprintf would be
	 * better, but don't want to have it in the kernel...
	 */
	printk(format, a0, a1, a2, a3, a4, a5);
}
