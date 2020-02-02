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
#ifndef x86_debug_h
#define x86_debug_h

#include "sysenv.h"

/* checks to be enabled for "runtime" */

#define CHECK_IP_FETCH_F		0x1
#define CHECK_SP_ACCESS_F		0x2
#define CHECK_MEM_ACCESS_F	        0x4	/*using regular linear pointer */
#define CHECK_DATA_ACCESS_F         	0x8	/*using segment:offset */

#ifdef DEBUG
# define CHECK_IP_FETCH(m)		((m)->x86.check & CHECK_IP_FETCH_F)
# define CHECK_SP_ACCESS(m)		((m)->x86.check & CHECK_SP_ACCESS_F)
# define CHECK_MEM_ACCESS(m)		((m)->x86.check & CHECK_MEM_ACCESS_F)
# define CHECK_DATA_ACCESS(m)     	((m)->x86.check & CHECK_DATA_ACCESS_F)
#else
# define CHECK_IP_FETCH(m)
# define CHECK_SP_ACCESS(m)
# define CHECK_MEM_ACCESS(m)
# define CHECK_DATA_ACCESS(m)
#endif

/* debug options */

#define DEBUG_DECODE_F		0x0001	/* print decoded instruction  */
#define DEBUG_TRACE_F		0x0002	/* dump regs before/after execution */
#define DEBUG_STEP_F		0x0004
#define DEBUG_DISASSEMBLE_F	0x0008
#define DEBUG_BREAK_F		0x0010
#define DEBUG_SVC_F		0x0020
#define DEBUG_FS_F		0x0080
#define DEBUG_PROC_F		0x0100
#define DEBUG_SYSINT_F		0x0200	/* bios system interrupts. */
#define DEBUG_TRACECALL_F	0x0400
#define DEBUG_INSTRUMENT_F	0x0800
#define DEBUG_MEM_TRACE_F	0x1000
#define DEBUG_IO_TRACE_F	0x2000
#define DEBUG_SYS_F		(DEBUG_SVC_F|DEBUG_FS_F|DEBUG_PROC_F)

#ifdef DEBUG
# define DEBUG_INSTRUMENT(m)	((m)->x86.debug & DEBUG_INSTRUMENT_F)
# define DEBUG_DECODE(m)	((m)->x86.debug & DEBUG_DECODE_F)
# define DEBUG_TRACE(m)		((m)->x86.debug & DEBUG_TRACE_F)
# define DEBUG_STEP(m)		((m)->x86.debug & DEBUG_STEP_F)
# define DEBUG_DISASSEMBLE(m)	((m)->x86.debug & DEBUG_DISASSEMBLE_F)
# define DEBUG_BREAK(m)		((m)->x86.debug & DEBUG_BREAK_F)
# define DEBUG_SVC(m)		((m)->x86.debug & DEBUG_SVC_F)
# define DEBUG_FS(m)		((m)->x86.debug & DEBUG_FS_F)
# define DEBUG_PROC(m)		((m)->x86.debug & DEBUG_PROC_F)
# define DEBUG_SYSINT(m)	((m)->x86.debug & DEBUG_SYSINT_F)
# define DEBUG_TRACECALL(m)	((m)->x86.debug & DEBUG_TRACECALL_F)
# define DEBUG_SYS(m)		((m)->x86.debug & DEBUG_SYS_F)
# define DEBUG_MEM_TRACE(m)	((m)->x86.debug & DEBUG_MEM_TRACE_F)
# define DEBUG_IO_TRACE(m)	((m)->x86.debug & DEBUG_IO_TRACE_F)
#else
# define DEBUG_INSTRUMENT(m)	0
# define DEBUG_DECODE(m)	0
# define DEBUG_TRACE(m)		0
# define DEBUG_STEP(m)		0
# define DEBUG_DISASSEMBLE(m)	0
# define DEBUG_BREAK(m)		0
# define DEBUG_SVC(m)		0
# define DEBUG_FS(m)		0
# define DEBUG_PROC(m)		0
# define DEBUG_SYSINT(m)	0
# define DEBUG_TRACECALL(m)	0
# define DEBUG_SYS(m)		0
# define DEBUG_MEM_TRACE(m)	0
# define DEBUG_IO_TRACE(m)	0
#endif

#ifdef DEBUG

# define DECODE_PRINTF(m,x)	if (DEBUG_DECODE(m)) x86_decode_printf(m,x)
# define DECODE_PRINTF2(m,x,y)	if (DEBUG_DECODE(m)) x86_decode_printf2(m,x,y)

/*
 * The following allow us to look at the bytes of an instruction.  The
 * first INCR_INSTRN_LEN, is called everytime bytes are consumed in
 * the decoding process.  The SAVE_IP_CS is called initially when the
 * major opcode of the instruction is accessed.
 */
#define INC_DECODED_INST_LEN(m,x)			\
    if (DEBUG_DECODE(m))				\
	x86_inc_decoded_inst_len(m,x)

#define SAVE_IP_CS(m,x,y)				\
    if (DEBUG_DECODE(m)|DEBUG_TRACECALL(m)) {		\
	(m)->x86.saved_cs = x;				\
	(m)->x86.saved_ip = y;				\
    }
#else
# define INC_DECODED_INST_LEN(m,x)
# define DECODE_PRINTF(m,x)
# define DECODE_PRINTF2(m,x,y)
# define SAVE_IP_CS(m,x,y)
#endif

#ifdef DEBUG

#define TRACE_REGS(x)					\
    if (DEBUG_DISASSEMBLE(x)) {				\
	x86_just_disassemble(x);			\
	goto EndOfTheInstructionProcedure;		\
    }							\
    if (DEBUG_TRACE(x) || DEBUG_DECODE(x)) x86_trace_regs(x)
#else
# define TRACE_REGS(x)
#endif

#ifdef DEBUG
# define SINGLE_STEP(x)		if (DEBUG_STEP(x)) x86_single_step(x)
#else
# define SINGLE_STEP(x)
#endif

#ifdef DEBUG
# define START_OF_INSTR(m)
# define END_OF_INSTR(m)	EndOfTheInstructionProcedure: x86_end_instr(m);
#else
# define START_OF_INSTR(m)
# define END_OF_INSTR(m)
#endif

#ifdef DEBUG
# define  CALL_TRACE(m,u,v,w)					\
    if (DEBUG_TRACECALL(m))					\
	x86_debug_printf(m, "%x:%x: CALL %x:%x\n", v, u ,v, w);
# define RETURN_TRACE(m,u)					\
    if (DEBUG_TRACECALL(m))					\
	x86_debug_printf(m, "%x: RET\n",u);
#else
# define CALL_TRACE(m,u,v,w)
# define RETURN_TRACE(m,u)
#endif

extern void x86_debug_printf(SysEnv * m, const char *format, ...);
extern void x86_inc_decoded_inst_len(SysEnv * m, int x);
extern void x86_decode_printf(SysEnv * m, char *x);
extern void x86_decode_printf2(SysEnv * m, char *x, int y);
extern void x86_just_disassemble(SysEnv * m);
extern void x86_trace_regs(SysEnv * m);
extern void x86_single_step(SysEnv * m);
extern void x86_end_instr(SysEnv * m);
extern void x86_dump_regs(SysEnv * m);
extern void x86_dump_memory(SysEnv * m, u16 seg, u16 off, u32 amt);
extern void x86_print_int_vect(SysEnv * m, u16 iv);
extern void x86_instrument_instruction(SysEnv * m);
extern void x86_check_ip_access(SysEnv * m);
extern void x86_check_sp_access(SysEnv * m);
extern void x86_check_mem_access(SysEnv * m, u32 p);
extern void x86_check_data_access(SysEnv * m, u16 s, u16 o);

#endif				/* x86_debug_h */
