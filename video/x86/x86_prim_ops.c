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
#include "x86_regs.h"
#include "x86_decode.h"
#include "x86_prim_ops.h"
#include "x86_debug.h"


u32 x86_parity_tab[8] = {
	0x96696996,
	0x69969669,
	0x69969669,
	0x96696996,
	0x69969669,
	0x96696996,
	0x96696996,
	0x69969669,
};

#define XOR2(x)	(((x) ^ ((x)>>1)) & 0x1)


/*
 * Carry Chain Calculation
 *
 * This represents a somewhat expensive calculation which is
 * apparently required to emulate the setting of the OF and AF flag.
 * The latter is not so important, but the former is.  The overflow
 * flag is the XOR of the top two bits of the carry chain for an
 * addition (similar for subtraction).  Since we do not want to
 * simulate the addition in a bitwise manner, we try to calculate the
 * carry chain given the two operands and the result.
 *
 * So, given the following table, which represents the addition of two
 * bits, we can derive a formula for the carry chain.
 *
 * a   b   cin   r     cout
 * 0   0   0     0     0
 * 0   0   1     1     0
 * 0   1   0     1     0
 * 0   1   1     0     1 
 * 1   0   0     1     0
 * 1   0   1     0     1
 * 1   1   0     0     1  
 * 1   1   1     1     1
 * 
 * Construction of table for cout:
 * 
 * ab 
 * r  \  00   01   11  10 
 * |------------------
 * 0  |   0    1    1   1
 * 1  |   0    0    1   0
 * 
 * By inspection, one gets:  cc = ab +  r'(a + b)
 * 
 * That represents alot of operations, but NO CHOICE.... 
 * 
 * Borrow Chain Calculation.
 *
 * The following table represents the subtraction of two bits, from
 * which we can derive a formula for the borrow chain.
 * 
 * a   b   bin   r     bout
 * 0   0   0     0     0
 * 0   0   1     1     1
 * 0   1   0     1     1
 * 0   1   1     0     1 
 * 1   0   0     1     0
 * 1   0   1     0     0
 * 1   1   0     0     0  
 * 1   1   1     1     1
 * 
 * Construction of table for cout:
 * 
 * ab 
 * r  \  00   01   11  10 
 * |------------------
 * 0  |   0    1    0   0
 * 1  |   1    1    1   0
 * 
 * By inspection, one gets:  bc = a'b +  r(a' + b)
 */


u16 aad_word(SysEnv * m, u16 d)
{
	u16 l;
	u8 hb, lb;

	hb = (d >> 8) & 0xff;
	lb = (d & 0xff);

	l = lb + 10 * hb;

	CONDITIONAL_SET_FLAG(l & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(l == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(l & 0xff), m, F_PF);
	return l;
}



u16 aam_word(SysEnv * m, u8 d)
{
	u16 h, l;

	h = d / 10;
	l = d % 10;
	l |= (h << 8);

	CONDITIONAL_SET_FLAG(l & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(l == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(l & 0xff), m, F_PF);
	return l;
}


u8 adc_byte(SysEnv * m, u8 d, u8 s)
{
	register u16 res;	/* all operands in native machine order */
	register u16 cc;

	if (ACCESS_FLAG(m, F_CF))
		res = 1 + d + s;
	else
		res = d + s;

	CONDITIONAL_SET_FLAG(res & 0x100, m, F_CF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = (s & d) | ((~res) & (s | d));

	CONDITIONAL_SET_FLAG(XOR2(cc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}

u16 adc_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 cc;

	if (ACCESS_FLAG(m, F_CF))
		res = 1 + d + s;
	else
		res = d + s;

	/* set the carry flag to be bit 8 */

	CONDITIONAL_SET_FLAG(res & 0x10000, m, F_CF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = (s & d) | ((~res) & (s | d));
	CONDITIONAL_SET_FLAG(XOR2(cc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}

/* Given   flags=f,  and bytes  d (dest)  and  s (source) 
   perform the add and set the flags and the result back to
   *d.   USE NATIVE MACHINE ORDER...
 */

u8 add_byte(SysEnv * m, u8 d, u8 s)
{
	register u16 res;	/* all operands in native machine order */
	register u16 cc;

	res = d + s;

	/* set the carry flag to be bit 8 */
	CONDITIONAL_SET_FLAG(res & 0x100, m, F_CF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = (s & d) | ((~res) & (s | d));
	CONDITIONAL_SET_FLAG(XOR2(cc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}


/* Given   flags=f,  and bytes  d (dest)  and  s (source) 
   perform the add and set the flags and the result back to
   *d.   USE NATIVE MACHINE ORDER...
 */

u16 add_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 cc;


	res = d + s;

	/* set the carry flag to be bit 8 */
	CONDITIONAL_SET_FLAG(res & 0x10000, m, F_CF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = (s & d) | ((~res) & (s | d));
	CONDITIONAL_SET_FLAG(XOR2(cc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}


/* 
   Flags m->x86.R_FLG,  dest *d,  source  *s,  do a bitwise and of the 
   source and destination, and then store back to the 
   destination.  Size=byte.  

 */
u8 and_byte(SysEnv * m, u8 d, u8 s)
{
	register u8 res;	/* all operands in native machine order */

	res = d & s;

	/* set the flags  */
	CLEAR_FLAG(m, F_OF);
	CLEAR_FLAG(m, F_CF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res), m, F_PF);

	return res;
}


/* 
   Flags m->x86.R_FLG,  dest *d,  source  *s,  do a bitwise and of the 
   source and destination, and then store back to the 
   destination.  Size=byte.  

 */
u16 and_word(SysEnv * m, u16 d, u16 s)
{
	register u16 res;	/* all operands in native machine order */

	res = d & s;

	/* set the flags  */
	CLEAR_FLAG(m, F_OF);
	CLEAR_FLAG(m, F_CF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	return res;
}



u8 cmp_byte(SysEnv * m, u8 d, u8 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - s;

	CLEAR_FLAG(m, F_CF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */

	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x80, m, F_CF);

	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return d;		/* long story why this is needed.  Look at opcode 
				   0x80 in ops.c, for an idea why this is necessary. */
}

u16 cmp_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x8000, m, F_CF);
	CONDITIONAL_SET_FLAG(XOR2(bc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return d;
}


u32 cmp_long(SysEnv * m, u32 d, u32 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x80000000, m, F_CF);
	CONDITIONAL_SET_FLAG(XOR2(bc >> 30), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return d;
}


u8 dec_byte(SysEnv * m, u8 d)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - 1;

	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	/* based on sub_byte, uses s==1.  */

	bc = (res & (~d | 1)) | (~d & 1);
	/* carry flag unchanged */
	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res;
}

u16 dec_word(SysEnv * m, u16 d)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - 1;

	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	/* based on the sub_byte routine, with s==1 */
	bc = (res & (~d | 1)) | (~d & 1);
	/* carry flag unchanged */
	CONDITIONAL_SET_FLAG(XOR2(bc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res;
}


/* Given   flags=f,  and byte  d (dest)  
   perform the inc and set the flags and the result back to
   d.   USE NATIVE MACHINE ORDER...
 */

u8 inc_byte(SysEnv * m, u8 d)
{
	register u32 res;	/* all operands in native machine order */
	register u32 cc;

	res = d + 1;

	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = ((1 & d) | (~res)) & (1 | d);
	CONDITIONAL_SET_FLAG(XOR2(cc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}


/* Given   flags=f,  and byte  d (dest)  
   perform the inc and set the flags and the result back to
   *d.   USE NATIVE MACHINE ORDER...
 */

u16 inc_word(SysEnv * m, u16 d)
{
	register u32 res;	/* all operands in native machine order */
	register u32 cc;

	res = d + 1;

	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the carry chain  SEE NOTE AT TOP. */
	cc = (1 & d) | ((~res) & (1 | d));
	CONDITIONAL_SET_FLAG(XOR2(cc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(cc & 0x8, m, F_AF);

	return res;
}


u8 or_byte(SysEnv * m, u8 d, u8 s)
{
	register u8 res;	/* all operands in native machine order */

	res = d | s;

	CLEAR_FLAG(m, F_OF);
	CLEAR_FLAG(m, F_CF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res), m, F_PF);

	return res;
}


u16 or_word(SysEnv * m, u16 d, u16 s)
{
	register u16 res;	/* all operands in native machine order */

	res = d | s;

	/* set the carry flag to be bit 8 */
	CLEAR_FLAG(m, F_OF);
	CLEAR_FLAG(m, F_CF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	return res;
}



u8 neg_byte(SysEnv * m, u8 s)
{
	register u8 res;
	register u8 bc;


	CONDITIONAL_SET_FLAG(s != 0, m, F_CF);

	res = -s;

	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res), m, F_PF);
	/* calculate the borrow chain --- modified such that d=0.
	   substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
	   (the one used for sub) and simplifying, since ~d=0xff..., 
	   ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
	   ~d&s == s.  So the simplified result is: */
	bc = res | s;
	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res;

}

u16 neg_word(SysEnv * m, u16 s)
{
	register u16 res;
	register u16 bc;


	CONDITIONAL_SET_FLAG(s != 0, m, F_CF);

	res = -s;

	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain --- modified such that d=0.
	   substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
	   (the one used for sub) and simplifying, since ~d=0xff..., 
	   ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
	   ~d&s == s.  So the simplified result is: */
	bc = res | s;
	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res;

}


u8 not_byte(SysEnv * m, u8 s)
{
	return ~s;
}

u16 not_word(SysEnv * m, u16 s)
{
	return ~s;
}


/* access stuff from absolute location in memory.  
   no segment registers are involved. 
 */

u16 mem_access_word(SysEnv * m, int addr)
{
#ifdef DEBUG
	if (CHECK_MEM_ACCESS(m))
		x86_check_mem_access(m, addr);
#endif
	/* Load in two steps.  Native byte order independent */
	return sys_rdw(m, addr);
}



/*  given the register_set  r, and memory descriptor m,  
   and word w, push w onto the stack.
   w ASSUMED IN NATIVE MACHINE ORDER.  Doesn't matter in this case???
 */


void push_word(SysEnv * m, u16 w)
{

#ifdef DEBUG
	if (CHECK_SP_ACCESS(m))
		x86_check_sp_access(m);
#endif

	m->x86.R_SP -= 2;
	sys_wrw(m, (m->x86.R_SS << 4) + m->x86.R_SP, w);
}



void push_long(SysEnv * m, u32 w)
{

#ifdef DEBUG
	if (CHECK_SP_ACCESS(m))
		x86_check_sp_access(m);
#endif

	m->x86.R_SP -= 4;
	sys_wrl(m, (m->x86.R_SS << 4) + m->x86.R_SP, w);
}



/*  given the  memory descriptor m,  
   and word w, pop word from the stack.

 */

u16 pop_word(SysEnv * m)
{
	register u16 res;

#ifdef DEBUG
	if (CHECK_SP_ACCESS(m))
		x86_check_sp_access(m);
#endif
	res = sys_rdw(m, (m->x86.R_SS << 4) + m->x86.R_SP);
	m->x86.R_SP += 2;
	return res;
}


u32 pop_long(SysEnv * m)
{
	register u32 res;

#ifdef DEBUG
	if (CHECK_SP_ACCESS(m))
		x86_check_sp_access(m);
#endif
	res = sys_rdl(m, (m->x86.R_SS << 4) + m->x86.R_SP);
	m->x86.R_SP += 4;
	return res;
}


/*****************************************************************

   BEGIN region consisting of bit shifts and rotates, 
   much of which may be wrong.  Large hirsute factor. 

*****************************************************************/


u8 rcl_byte(SysEnv * m, u8 d, u8 s)
{
	register unsigned int res, cnt, mask, cf;

	/* s is the rotate distance.  It varies from 0 - 8. */
	/* have 

	   CF  B_7 B_6 B_5 B_4 B_3 B_2 B_1 B_0 

	   want to rotate through the carry by "s" bits.  We could 
	   loop, but that's inefficient.  So the width is 9,
	   and we split into three parts:

	   The new carry flag   (was B_n)
	   the stuff in B_n-1 .. B_0
	   the stuff in B_7 .. B_n+1

	   The new rotate is done mod 9, and given this,
	   for a rotation of n bits (mod 9) the new carry flag is
	   then located n bits from the MSB.  The low part is 
	   then shifted up cnt bits, and the high part is or'd
	   in.  Using CAPS for new values, and lowercase for the 
	   original values, this can be expressed as:

	   IF n > 0 
	   1) CF <-  b_(8-n)
	   2) B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_0
	   3) B_(n-1) <- cf
	   4) B_(n-2) .. B_0 <-  b_7 .. b_(8-(n-1))

	   I think this is correct.  

	 */

	res = d;

	if ((cnt = s % 9) != 0) {
		/* extract the new CARRY FLAG. */
		/* CF <-  b_(8-n)             */
		cf = (d >> (8 - cnt)) & 0x1;

		/* get the low stuff which rotated 
		   into the range B_7 .. B_cnt */
		/* B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_0  */
		/* note that the right hand side done by the mask */
		res = (d << cnt) & 0xff;

		/* now the high stuff which rotated around 
		   into the positions B_cnt-2 .. B_0 */
		/* B_(n-2) .. B_0 <-  b_7 .. b_(8-(n-1)) */
		/* shift it downward, 7-(n-2) = 9-n positions. 
		   and mask off the result before or'ing in. 
		 */
		mask = (1 << (cnt - 1)) - 1;
		res |= (d >> (9 - cnt)) & mask;

		/* if the carry flag was set, or it in.  */
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			/*  B_(n-1) <- cf */
			res |= 1 << (cnt - 1);
		}
		/* set the new carry flag, based on the variable "cf" */
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		/* parenthesized this expression since it appears to
		   be causing OF to be misset */
		CONDITIONAL_SET_FLAG(cnt == 1
				     && XOR2(cf + ((res >> 6) & 0x2)), m,
				     F_OF);

	}
	return res & 0xff;
}



u16 rcl_word(SysEnv * m, u16 d, u16 s)
{
	register unsigned int res, cnt, mask, cf;

	/* see analysis above. */
	/* width here is 16 bits + carry bit */

	res = d;

	if ((cnt = s % 17) != 0) {
		/* extract the new CARRY FLAG. */
		/* CF <-  b_(16-n)             */
		cf = (d >> (16 - cnt)) & 0x1;

		/* get the low stuff which rotated 
		   into the range B_15 .. B_cnt */
		/* B_(15) .. B_(n)  <-  b_(16-(n+1)) .. b_0  */
		/* note that the right hand side done by the mask */
		res = (d << cnt) & 0xffff;

		/* now the high stuff which rotated around 
		   into the positions B_cnt-2 .. B_0 */
		/* B_(n-2) .. B_0 <-  b_15 .. b_(16-(n-1)) */
		/* shift it downward, 15-(n-2) = 17-n positions. 
		   and mask off the result before or'ing in. 
		 */
		mask = (1 << (cnt - 1)) - 1;
		res |= (d >> (17 - cnt)) & mask;

		/* if the carry flag was set, or it in.  */
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			/*  B_(n-1) <- cf */
			res |= 1 << (cnt - 1);
		}
		/* set the new carry flag, based on the variable "cf" */
		CONDITIONAL_SET_FLAG(cf, m, F_CF);

		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. 
		   Note that we're forming a 2 bit word here to index 
		   into the table. The expression cf+(res>>14)&0x2  
		   represents the two bit word b_15 CF.
		 */
		/* parenthesized following expression... */
		CONDITIONAL_SET_FLAG(cnt == 1
				     && XOR2(cf + ((res >> 14) & 0x2)), m,
				     F_OF);

	}
	return res & 0xffff;
}


u32 rcl_long(SysEnv * m, u32 d, u32 s)
{
	register u32 res, cnt, mask, cf;

	res = d;
	if ((cnt = s % 33) != 0) {
		cf = (d >> (32 - cnt)) & 0x1;
		res = (d << cnt) & 0xffff;
		mask = (1 << (cnt - 1)) - 1;
		res |= (d >> (33 - cnt)) & mask;
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			res |= 1 << (cnt - 1);
		}
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		CONDITIONAL_SET_FLAG(cnt == 1
				     && XOR2(cf + ((res >> 30) & 0x2)), m,
				     F_OF);
	}
	return res;
}



u8 rcr_byte(SysEnv * m, u8 d, u8 s)
{
	u8 res, cnt;
	u8 mask, cf, ocf = 0;

	/* rotate right through carry */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   CF  B_7 B_6 B_5 B_4 B_3 B_2 B_1 B_0 

	   The new rotate is done mod 9, and given this,
	   for a rotation of n bits (mod 9) the new carry flag is
	   then located n bits from the LSB.  The low part is 
	   then shifted up cnt bits, and the high part is or'd
	   in.  Using CAPS for new values, and lowercase for the 
	   original values, this can be expressed as:

	   IF n > 0 
	   1) CF <-  b_(n-1)
	   2) B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n)
	   3) B_(8-n) <- cf
	   4) B_(7) .. B_(8-(n-1)) <-  b_(n-2) .. b_(0)

	   I think this is correct.  

	 */

	res = d;

	if ((cnt = s % 9) != 0) {
		/* extract the new CARRY FLAG. */
		/* CF <-  b_(n-1)              */
		if (cnt == 1) {
			cf = d & 0x1;
			/* note hackery here.  Access_flag(..) evaluates to either
			   0 if flag not set
			   non-zero if flag is set.
			   doing access_flag(..) != 0 casts that into either 
			   0..1 in any representation of the flags register
			   (i.e. packed bit array or unpacked.)
			 */
			ocf = ACCESS_FLAG(m, F_CF) != 0;
		} else
			cf = (d >> (cnt - 1)) & 0x1;

		/* B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_n  */
		/* note that the right hand side done by the mask
		   This is effectively done by shifting the 
		   object to the right.  The result must be masked,
		   in case the object came in and was treated 
		   as a negative number.  Needed??? */

		mask = (1 << (8 - cnt)) - 1;
		res = (d >> cnt) & mask;

		/* now the high stuff which rotated around 
		   into the positions B_cnt-2 .. B_0 */
		/* B_(7) .. B_(8-(n-1)) <-  b_(n-2) .. b_(0) */
		/* shift it downward, 7-(n-2) = 9-n positions. 
		   and mask off the result before or'ing in. 
		 */
		res |= (d << (9 - cnt));

		/* if the carry flag was set, or it in.  */
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			/*  B_(8-n) <- cf */
			res |= 1 << (8 - cnt);
		}
		/* set the new carry flag, based on the variable "cf" */
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		/* parenthesized... */
		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 6) & 0x2)),
					     m, F_OF);
		}
	}
	return res;
}



u16 rcr_word(SysEnv * m, u16 d, u16 s)
{
	u16 res, cnt;
	u16 mask, cf, ocf = 0;

	/* rotate right through carry */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   CF  B_15   ...   B_0 

	   The new rotate is done mod 17, and given this,
	   for a rotation of n bits (mod 17) the new carry flag is
	   then located n bits from the LSB.  The low part is 
	   then shifted up cnt bits, and the high part is or'd
	   in.  Using CAPS for new values, and lowercase for the 
	   original values, this can be expressed as:

	   IF n > 0 
	   1) CF <-  b_(n-1)
	   2) B_(16-(n+1)) .. B_(0)  <-  b_(15) .. b_(n)
	   3) B_(16-n) <- cf
	   4) B_(15) .. B_(16-(n-1)) <-  b_(n-2) .. b_(0)

	   I think this is correct.  

	 */
	res = d;
	if ((cnt = s % 17) != 0) {

		/* extract the new CARRY FLAG. */
		/* CF <-  b_(n-1)              */
		if (cnt == 1) {
			cf = d & 0x1;
			/* see note above on teh byte version */
			ocf = ACCESS_FLAG(m, F_CF) != 0;
		} else
			cf = (d >> (cnt - 1)) & 0x1;

		/* B_(16-(n+1)) .. B_(0)  <-  b_(15) .. b_n  */
		/* note that the right hand side done by the mask
		   This is effectively done by shifting the 
		   object to the right.  The result must be masked,
		   in case the object came in and was treated 
		   as a negative number.  Needed??? */
		mask = (1 << (16 - cnt)) - 1;
		res = (d >> cnt) & mask;

		/* now the high stuff which rotated around 
		   into the positions B_cnt-2 .. B_0 */
		/* B_(15) .. B_(16-(n-1)) <-  b_(n-2) .. b_(0) */
		/* shift it downward, 15-(n-2) = 17-n positions. 
		   and mask off the result before or'ing in. 
		 */
		res |= (d << (17 - cnt));

		/* if the carry flag was set, or it in.  */
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			/*  B_(16-n) <- cf */
			res |= 1 << (16 - cnt);
		}
		/* set the new carry flag, based on the variable "cf" */
		CONDITIONAL_SET_FLAG(cf, m, F_CF);

		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 14) & 0x2)),
					     m, F_OF);
		}
	}
	return res;
}


u32 rcr_long(SysEnv * m, u32 d, u32 s)
{
	u32 res, cnt;
	u32 mask, cf, ocf = 0;

	/* rotate right through carry */
	res = d;
	if ((cnt = s % 33) != 0) {
		if (cnt == 1) {
			cf = d & 0x1;
			ocf = ACCESS_FLAG(m, F_CF) != 0;
		} else
			cf = (d >> (cnt - 1)) & 0x1;
		mask = (1 << (32 - cnt)) - 1;
		res = (d >> cnt) & mask;
		res |= (d << (33 - cnt));
		if (ACCESS_FLAG(m, F_CF)) {	/* carry flag is set */
			res |= 1 << (32 - cnt);
		}
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 30) & 0x2)),
					     m, F_OF);
		}
	}
	return res;
}


u8 rol_byte(SysEnv * m, u8 d, u8 s)
{
	register unsigned int res, cnt, mask;

	/* rotate left */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   CF  B_7 ... B_0 

	   The new rotate is done mod 8.
	   Much simpler than the "rcl" or "rcr" operations.

	   IF n > 0 
	   1) B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_(0)
	   2) B_(n-1) .. B_(0) <-  b_(7) .. b_(8-n)

	   I think this is correct.  

	 */
	res = d;

	if ((cnt = s % 8) != 0) {
		/* B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_(0) */
		res = (d << cnt);

		/* B_(n-1) .. B_(0) <-  b_(7) .. b_(8-n) */
		mask = (1 << cnt) - 1;
		res |= (d >> (8 - cnt)) & mask;

		/* set the new carry flag, Note that it is the low order 
		   bit of the result!!!                               */
		CONDITIONAL_SET_FLAG(res & 0x1, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		CONDITIONAL_SET_FLAG(cnt == 1 &&
				     XOR2((res & 0x1) +
					  ((res >> 6) & 0x2)), m, F_OF);

	}
	return res & 0xff;
}


u16 rol_word(SysEnv * m, u16 d, u16 s)
{
	register unsigned int res, cnt, mask;

	/* rotate left */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   CF  B_15 ... B_0 

	   The new rotate is done mod 8.
	   Much simpler than the "rcl" or "rcr" operations.

	   IF n > 0 
	   1) B_(15) .. B_(n)  <-  b_(16-(n+1)) .. b_(0)
	   2) B_(n-1) .. B_(0) <-  b_(16) .. b_(16-n)

	   I think this is correct.  

	 */
	res = d;

	if ((cnt = s % 16) != 0) {
		/* B_(16) .. B_(n)  <-  b_(16-(n+1)) .. b_(0) */
		res = (d << cnt);

		/* B_(n-1) .. B_(0) <-  b_(15) .. b_(16-n) */
		mask = (1 << cnt) - 1;
		res |= (d >> (16 - cnt)) & mask;

		/* set the new carry flag, Note that it is the low order 
		   bit of the result!!!                               */
		CONDITIONAL_SET_FLAG(res & 0x1, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		CONDITIONAL_SET_FLAG(cnt == 1 &&
				     XOR2((res & 0x1) +
					  ((res >> 14) & 0x2)), m, F_OF);

	}
	return res & 0xffff;
}


u32 rol_long(SysEnv * m, u32 d, u32 s)
{
	register u32 res, cnt, mask;

	res = d;
	if ((cnt = s % 32) != 0) {
		res = (d << cnt);
		mask = (1 << cnt) - 1;
		res |= (d >> (32 - cnt)) & mask;
		CONDITIONAL_SET_FLAG(res & 0x1, m, F_CF);
		CONDITIONAL_SET_FLAG(cnt == 1 &&
				     XOR2((res & 0x1) +
					  ((res >> 30) & 0x2)), m, F_OF);
	}
	return res;
}


u8 ror_byte(SysEnv * m, u8 d, u8 s)
{
	register unsigned int res, cnt, mask;

	/* rotate right */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   B_7 ... B_0 

	   The rotate is done mod 8.

	   IF n > 0 
	   1) B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n)
	   2) B_(7) .. B_(8-n) <-  b_(n-1) .. b_(0)



	 */
	res = d;

	if ((cnt = s % 8) != 0) {	/* not a typo, do nada if cnt==0 */
		/* B_(7) .. B_(8-n) <-  b_(n-1) .. b_(0) */
		res = (d << (8 - cnt));

		/* B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n) */
		mask = (1 << (8 - cnt)) - 1;
		res |= (d >> (cnt)) & mask;

		/* set the new carry flag, Note that it is the low order 
		   bit of the result!!!                               */
		CONDITIONAL_SET_FLAG(res & 0x80, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of the two most significant bits.  Blecck. */
		CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(res >> 6), m, F_OF);

	}
	return res & 0xff;
}


u16 ror_word(SysEnv * m, u16 d, u16 s)
{
	register unsigned int res, cnt, mask;

	/* rotate right */
	/* 
	   s is the rotate distance.  It varies from 0 - 8.
	   d is the byte object rotated.  

	   have 

	   B_15 ... B_0 

	   The rotate is done mod 16.

	   IF n > 0 
	   1) B_(16-(n+1)) .. B_(0)  <-  b_(15) .. b_(n)
	   2) B_(15) .. B_(16-n) <-  b_(n-1) .. b_(0)


	   I think this is correct.  

	 */
	res = d;

	if ((cnt = s % 16) != 0) {
		/* B_(15) .. B_(16-n) <-  b_(n-1) .. b_(0) */
		res = (d << (16 - cnt));

		/* B_(16-(n+1)) .. B_(0)  <-  b_(15) .. b_(n) */
		mask = (1 << (16 - cnt)) - 1;
		res |= (d >> (cnt)) & mask;

		/* set the new carry flag, Note that it is the low order 
		   bit of the result!!!                               */
		CONDITIONAL_SET_FLAG(res & 0x8000, m, F_CF);
		/* OVERFLOW is set *IFF* cnt==1, then it is the 
		   xor of CF and the most significant bit.  Blecck. */
		CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(res >> 14), m, F_OF);
	}
	return res & 0xffff;
}


u32 ror_long(SysEnv * m, u32 d, u32 s)
{
	register u32 res, cnt, mask;

	res = d;
	if ((cnt = s % 32) != 0) {
		res = (d << (32 - cnt));
		mask = (1 << (32 - cnt)) - 1;
		res |= (d >> (cnt)) & mask;
		CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_CF);
		CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(res >> 30), m, F_OF);
	}
	return res;
}


u8 shl_byte(SysEnv * m, u8 d, u8 s)
{
	unsigned int cnt, res, cf;

	if (s < 8) {
		cnt = s % 8;

		/* last bit shifted out goes into carry flag */

		if (cnt > 0) {
			res = d << cnt;
			cf = d & (1 << (8 - cnt));
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = (u8) d;
		}

		if (cnt == 1) {
			/* Needs simplification. */
			CONDITIONAL_SET_FLAG(
					     (((res & 0x80) == 0x80) ^
					      (ACCESS_FLAG(m, F_CF) != 0)),
					     /* was (m->x86.R_FLG&F_CF)==F_CF)), */
					     m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}
	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
		SET_FLAG(m, F_ZF);
	}
	return res & 0xff;
}


u16 shl_word(SysEnv * m, u16 d, u16 s)
{
	unsigned int cnt, res, cf;

	if (s < 16) {

		cnt = s % 16;
		if (cnt > 0) {
			res = d << cnt;
			/* last bit shifted out goes into carry flag */
			cf = d & (1 << (16 - cnt));
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = (u16) d;
		}

		if (cnt == 1) {
			/* Needs simplification. */
			CONDITIONAL_SET_FLAG(
					     (((res & 0x8000) == 0x8000) ^
					      (ACCESS_FLAG(m, F_CF) != 0)),
					     /*((m&F_CF)==F_CF)), */
					     m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}
	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		SET_FLAG(m, F_ZF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
	}
	return res & 0xffff;
}



u32 shl_long(SysEnv * m, u32 d, u32 s)
{
	unsigned int cnt, res, cf;

	if (s < 32) {
		cnt = s % 32;
		if (cnt > 0) {
			res = d << cnt;
			/* last bit shifted out goes into carry flag */
			cf = d & (1 << (32 - cnt));
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, m,
					     F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = d;
		}
		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(
					     (((res
						& 0x80000000) ==
					       0x80000000) ^
					      (ACCESS_FLAG(m, F_CF) != 0)),
					     m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}
	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		SET_FLAG(m, F_ZF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
	}
	return res;
}


u8 shr_byte(SysEnv * m, u8 d, u8 s)
{
	unsigned int cnt, res, cf, mask;

	if (s < 8) {
		cnt = s % 8;

		if (cnt > 0) {

			mask = (1 << (8 - cnt)) - 1;
			cf = d & (1 << (cnt - 1));
			res = (d >> cnt) & mask;
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = (u8) d;
		}

		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(res >> 6), m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}
	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		SET_FLAG(m, F_ZF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
	}
	return res & 0xff;
}


u16 shr_word(SysEnv * m, u16 d, u16 s)
{
	unsigned int cnt, res, cf, mask;

	res = d;

	if (s < 16) {
		cnt = s % 16;

		if (cnt > 0) {
			mask = (1 << (16 - cnt)) - 1;
			cf = d & (1 << (cnt - 1));
			res = (d >> cnt) & mask;
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = d;
		}


		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(res >> 14), m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}

	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		SET_FLAG(m, F_ZF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
	}
	return res & 0xffff;
}


u32 shr_long(SysEnv * m, u32 d, u32 s)
{
	unsigned int cnt, res, cf, mask;

	res = d;
	if (s < 32) {
		cnt = s % 32;
		if (cnt > 0) {
			mask = (1 << (32 - cnt)) - 1;
			cf = d & (1 << (cnt - 1));
			res = (d >> cnt) & mask;
			CONDITIONAL_SET_FLAG(cf, m, F_CF);
			CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, m,
					     F_ZF);
			CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_SF);
			CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		} else {
			res = d;
		}
		if (cnt == 1) {
			CONDITIONAL_SET_FLAG(XOR2(res >> 30), m, F_OF);
		} else {
			CLEAR_FLAG(m, F_OF);
		}
	} else {
		res = 0;
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
		SET_FLAG(m, F_ZF);
		CLEAR_FLAG(m, F_SF);
		CLEAR_FLAG(m, F_PF);
	}
	return res;
}


/* XXXX ??? flags may be wrong??? */
u8 sar_byte(SysEnv * m, u8 d, u8 s)
{
	unsigned int cnt, res, cf, mask, sf;

	res = d;

	sf = d & 0x80;
	cnt = s % 8;

	if (cnt > 0 && cnt < 8) {
		mask = (1 << (8 - cnt)) - 1;
		cf = d & (1 << (cnt - 1));
		res = (d >> cnt) & mask;
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		if (sf) {
			res |= ~mask;
		}
		CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
		CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
		CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	} else if (cnt >= 8) {
		if (sf) {
			res = 0xff;
			SET_FLAG(m, F_CF);
			CLEAR_FLAG(m, F_ZF);
			SET_FLAG(m, F_SF);
			SET_FLAG(m, F_PF);
		} else {
			res = 0;
			CLEAR_FLAG(m, F_CF);
			SET_FLAG(m, F_ZF);
			CLEAR_FLAG(m, F_SF);
			CLEAR_FLAG(m, F_PF);
		}
	}
	return res & 0xff;
}


u16 sar_word(SysEnv * m, u16 d, u16 s)
{
	unsigned int cnt, res, cf, mask, sf;

	sf = d & 0x8000;
	cnt = s % 16;

	res = d;

	if (cnt > 0 && cnt < 16) {
		mask = (1 << (16 - cnt)) - 1;
		cf = d & (1 << (cnt - 1));
		res = (d >> cnt) & mask;
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		if (sf) {
			res |= ~mask;
		}
		CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
		CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
		CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	} else if (cnt >= 16) {
		if (sf) {
			res = 0xffff;
			SET_FLAG(m, F_CF);
			CLEAR_FLAG(m, F_ZF);
			SET_FLAG(m, F_SF);
			SET_FLAG(m, F_PF);
		} else {
			res = 0;
			CLEAR_FLAG(m, F_CF);
			SET_FLAG(m, F_ZF);
			CLEAR_FLAG(m, F_SF);
			CLEAR_FLAG(m, F_PF);
		}
	}
	return res & 0xffff;
}


u32 sar_long(SysEnv * m, u32 d, u32 s)
{
	u32 cnt, res, cf, mask, sf;

	sf = d & 0x80000000;
	cnt = s % 32;

	res = d;

	if (cnt > 0 && cnt < 32) {
		mask = (1 << (32 - cnt)) - 1;
		cf = d & (1 << (cnt - 1));
		res = (d >> cnt) & mask;
		CONDITIONAL_SET_FLAG(cf, m, F_CF);
		if (sf) {
			res |= ~mask;
		}
		CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, m, F_ZF);
		CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_SF);
		CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	} else if (cnt >= 32) {
		if (sf) {
			res = 0xffffffff;
			SET_FLAG(m, F_CF);
			CLEAR_FLAG(m, F_ZF);
			SET_FLAG(m, F_SF);
			SET_FLAG(m, F_PF);
		} else {
			res = 0;
			CLEAR_FLAG(m, F_CF);
			SET_FLAG(m, F_ZF);
			CLEAR_FLAG(m, F_SF);
			CLEAR_FLAG(m, F_PF);
		}
	}
	return res;
}


u8 sbb_byte(SysEnv * m, u8 d, u8 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	if (ACCESS_FLAG(m, F_CF))
		res = d - s - 1;
	else
		res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x80, m, F_CF);
	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res & 0xff;
}

u16 sbb_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	if (ACCESS_FLAG(m, F_CF))
		res = d - s - 1;
	else
		res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x8000, m, F_CF);
	CONDITIONAL_SET_FLAG(XOR2(bc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res & 0xffff;
}



u8 sub_byte(SysEnv * m, u8 d, u8 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */

	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x80, m, F_CF);

	CONDITIONAL_SET_FLAG(XOR2(bc >> 6), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res & 0xff;
}


u16 sub_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */
	register u32 bc;

	res = d - s;

	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);

	/* calculate the borrow chain.  See note at top */
	bc = (res & (~d | s)) | (~d & s);
	CONDITIONAL_SET_FLAG(bc & 0x8000, m, F_CF);
	CONDITIONAL_SET_FLAG(XOR2(bc >> 14), m, F_OF);
	CONDITIONAL_SET_FLAG(bc & 0x8, m, F_AF);

	return res & 0xffff;
}



void test_byte(SysEnv * m, u8 d, u8 s)
{
	register u32 res;	/* all operands in native machine order */

	res = d & s;

	CLEAR_FLAG(m, F_OF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	/* AF == dont care */
	CLEAR_FLAG(m, F_CF);
}


void test_word(SysEnv * m, u16 d, u16 s)
{
	register u32 res;	/* all operands in native machine order */

	res = d & s;

	CLEAR_FLAG(m, F_OF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	/* AF == dont care */
	CLEAR_FLAG(m, F_CF);
}


u8 xor_byte(SysEnv * m, u8 d, u8 s)
{
	register u8 res;	/* all operands in native machine order */

	res = d ^ s;

	CLEAR_FLAG(m, F_OF);
	CONDITIONAL_SET_FLAG(res & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res), m, F_PF);
	CLEAR_FLAG(m, F_CF);

	return res;
}


u16 xor_word(SysEnv * m, u16 d, u16 s)
{
	register u16 res;	/* all operands in native machine order */

	res = d ^ s;

	/* set the carry flag to be bit 8 */
	CLEAR_FLAG(m, F_OF);
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), m, F_PF);
	CLEAR_FLAG(m, F_CF);

	return res;
}


void imul_byte(SysEnv * m, u8 s)
{
	s16 res = (s8) m->x86.R_AL * (s8) s;

	m->x86.R_AX = res;

	/* Undef --- Can't hurt */
	CONDITIONAL_SET_FLAG(res & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);

	if (m->x86.R_AH == 0 || m->x86.R_AH == 0xff) {
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
	} else {
		SET_FLAG(m, F_CF);
		SET_FLAG(m, F_OF);
	}
}


void imul_word(SysEnv * m, u16 s)
{
	s32 res = (s16) m->x86.R_AX * (s16) s;

	m->x86.R_AX = res & 0xffff;
	m->x86.R_DX = (res >> 16) & 0xffff;

	/* Undef --- Can't hurt */
	CONDITIONAL_SET_FLAG(res & 0x80000000, m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);

	if (m->x86.R_DX == 0 || m->x86.R_DX == 0xffff) {
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
	} else {
		SET_FLAG(m, F_CF);
		SET_FLAG(m, F_OF);
	}
}


void mul_byte(SysEnv * m, u8 s)
{
	u16 res = m->x86.R_AL * s;

	m->x86.R_AX = res;

	/* Undef --- Can't hurt */
	CLEAR_FLAG(m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);

	if (m->x86.R_AH == 0) {
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
	} else {
		SET_FLAG(m, F_CF);
		SET_FLAG(m, F_OF);
	}
}


void mul_word(SysEnv * m, u16 s)
{

	u32 res = m->x86.R_AX * s;

	/* Undef --- Can't hurt */
	CLEAR_FLAG(m, F_SF);
	CONDITIONAL_SET_FLAG(res == 0, m, F_ZF);

	m->x86.R_AX = res & 0xffff;
	m->x86.R_DX = (res >> 16) & 0xffff;

	if (m->x86.R_DX == 0) {
		CLEAR_FLAG(m, F_CF);
		CLEAR_FLAG(m, F_OF);
	} else {
		SET_FLAG(m, F_CF);
		SET_FLAG(m, F_OF);
	}
}


void idiv_byte(SysEnv * m, u8 s)
{
	s32 dvd, div, mod;

	dvd = (s16) m->x86.R_AX;


	if (s == 0) {
		x86_intr_raise(m, 0);
		return;
	}
	div = dvd / (s8) s;
	mod = dvd % (s8) s;

	if (abs(div) > 0x7f) {
		x86_intr_raise(m, 0);
		return;
	}
	/* Undef --- Can't hurt */
	CONDITIONAL_SET_FLAG(div & 0x80, m, F_SF);
	CONDITIONAL_SET_FLAG(div == 0, m, F_ZF);

	m->x86.R_AL = (s8) div;
	m->x86.R_AH = (s8) mod;
}


void idiv_word(SysEnv * m, u16 s)
{
	s32 dvd, dvs, div, mod;

	dvd = m->x86.R_DX;
	dvd = (dvd << 16) | m->x86.R_AX;


	if (s == 0) {
		x86_intr_raise(m, 0);
		return;
	}
	dvs = (s16) s;
	div = dvd / dvs;
	mod = dvd % dvs;

	if (abs(div) > 0x7fff) {
		x86_intr_raise(m, 0);
		return;
	}
	/* Undef --- Can't hurt */
	CONDITIONAL_SET_FLAG(div & 0x8000, m, F_SF);
	CONDITIONAL_SET_FLAG(div == 0, m, F_ZF);

	m->x86.R_AX = div;
	m->x86.R_DX = mod;
}



void div_byte(SysEnv * m, u8 s)
{
	u32 dvd, dvs, div, mod;

	dvs = s;

	dvd = m->x86.R_AX;
	if (s == 0) {
		x86_intr_raise(m, 0);
		return;
	}
	div = dvd / dvs;
	mod = dvd % dvs;

	if (abs(div) > 0xff) {
		x86_intr_raise(m, 0);
		return;
	}
	/* Undef --- Can't hurt */
	CLEAR_FLAG(m, F_SF);
	CONDITIONAL_SET_FLAG(div == 0, m, F_ZF);


	m->x86.R_AL = (u8) div;
	m->x86.R_AH = (u8) mod;
}


void div_word(SysEnv * m, u16 s)
{
	u32 dvd, dvs, div, mod;

	dvd = m->x86.R_DX;
	dvd = (dvd << 16) | m->x86.R_AX;
	dvs = s;

	if (dvs == 0) {
		x86_intr_raise(m, 0);
		return;
	}
	div = dvd / dvs;
	mod = dvd % dvs;

	if (abs(div) > 0xffff) {
		x86_intr_raise(m, 0);
		return;
	}
	/* Undef --- Can't hurt */
	CLEAR_FLAG(m, F_SF);
	CONDITIONAL_SET_FLAG(div == 0, m, F_ZF);

	m->x86.R_AX = div;
	m->x86.R_DX = mod;
}


void ins(SysEnv * m, int size)
{
	int inc = size;

	if (ACCESS_FLAG(m, F_DF)) {
		inc = -size;
	}
	if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
		/* dont care whether REPE or REPNE */
		/* in until CX is ZERO. */
		u32 count = ((m->x86.mode & SYSMODE_PREFIX_DATA) ?
			     m->x86.R_ECX : m->x86.R_CX);
		switch (size) {
		case 1:
			while (count--) {
				store_data_byte_abs(m, m->x86.R_ES,
						    m->x86.R_DI, sys_inb(m,
									 m->
									 x86.
									 R_DX));
				m->x86.R_DI += inc;
			}
			break;

		case 2:
			while (count--) {
				store_data_word_abs(m, m->x86.R_ES,
						    m->x86.R_DI, sys_inw(m,
									 m->
									 x86.
									 R_DX));
				m->x86.R_DI += inc;
			}
			break;
		case 4:
			while (count--) {
				store_data_long_abs(m, m->x86.R_ES,
						    m->x86.R_DI, sys_inl(m,
									 m->
									 x86.
									 R_DX));
				m->x86.R_DI += inc;
				break;
			}
		}
		m->x86.R_CX = 0;
		if (m->x86.mode & SYSMODE_PREFIX_DATA) {
			m->x86.R_ECX = 0;
		}
		m->x86.mode &=
		    ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
	} else {
		switch (size) {
		case 1:
			store_data_byte_abs(m, m->x86.R_ES, m->x86.R_DI,
					    sys_inb(m, m->x86.R_DX));
			break;
		case 2:
			store_data_word_abs(m, m->x86.R_ES, m->x86.R_DI,
					    sys_inw(m, m->x86.R_DX));
			break;
		case 4:
			store_data_long_abs(m, m->x86.R_ES, m->x86.R_DI,
					    sys_inl(m, m->x86.R_DX));
			break;
		}
		m->x86.R_DI += inc;
	}
}


void outs(SysEnv * m, int size)
{
	int inc = size;

	if (ACCESS_FLAG(m, F_DF)) {
		inc = -size;
	}
	if (m->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
		/* dont care whether REPE or REPNE */
		/* out until CX is ZERO. */
		u32 count = ((m->x86.mode & SYSMODE_PREFIX_DATA) ?
			     m->x86.R_ECX : m->x86.R_CX);
		switch (size) {
		case 1:
			while (count--) {
				sys_outb(m, m->x86.R_DX,
					 fetch_data_byte_abs(m,
							     m->x86.R_ES,
							     m->x86.R_SI));
				m->x86.R_SI += inc;
			}
			break;

		case 2:
			while (count--) {
				sys_outw(m, m->x86.R_DX,
					 fetch_data_word_abs(m,
							     m->x86.R_ES,
							     m->x86.R_SI));
				m->x86.R_SI += inc;
			}
			break;
		case 4:
			while (count--) {
				sys_outl(m, m->x86.R_DX,
					 fetch_data_long_abs(m,
							     m->x86.R_ES,
							     m->x86.R_SI));
				m->x86.R_SI += inc;
				break;
			}
		}
		m->x86.R_CX = 0;
		if (m->x86.mode & SYSMODE_PREFIX_DATA) {
			m->x86.R_ECX = 0;
		}
		m->x86.mode &=
		    ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
	} else {
		switch (size) {
		case 1:
			sys_outb(m, m->x86.R_DX,
				 fetch_data_byte_abs(m, m->x86.R_ES,
						     m->x86.R_SI));
			break;
		case 2:
			sys_outw(m, m->x86.R_DX,
				 fetch_data_word_abs(m, m->x86.R_ES,
						     m->x86.R_SI));
			break;
		case 4:
			sys_outl(m, m->x86.R_DX,
				 fetch_data_long_abs(m, m->x86.R_ES,
						     m->x86.R_SI));
			break;
		}
		m->x86.R_SI += inc;
	}
}
