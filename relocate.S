/*****************************************************************************

       Copyright © 1995, 1996 Digital Equipment Corporation,
                       Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, provided  
that the copyright notice and this permission notice appear in all copies  
of software and supporting documentation, and that the name of Digital not  
be used in advertising or publicity pertaining to distribution of the software 
without specific, written prior permission. Digital grants this permission 
provided that you prominently mark, as not part of the original, any 
modifications made to this software or documentation.

Digital Equipment Corporation disclaims all warranties and/or guarantees  
with regard to this software, including all implied warranties of fitness for 
a particular purpose and merchantability, and makes no representations 
regarding the use of, or the results of the use of, the software and 
documentation in terms of correctness, accuracy, reliability, currentness or
otherwise; and you rely on the software, documentation and results solely at 
your own risk. 

******************************************************************************/
#include <alpha/regdef.h>
#include <asm/system.h>
#include <linux/config.h>

#define ldq_p		hw_ldq/p
#define stq_p		hw_stq/p
#define IPR_V_ABX	6
#define IPR_M_ABX	1 << IPR_V_ABX
#define flushIc		IPR_M_ABX + 0x15

	.align 7
	.globl	__relocate
	.ent	__relocate
__relocate:
	bis	zero,zero,zero		# spacer
	br	t0, 1f
	.quad	0x200000;  		# final address
	.quad	0x020000;               # size of image (in quadwords)
1:					# t0 now contains address of
					# data block
	# get the arguments from the data block
	ldq_p	t1, 0*8(t0)	
	ldq_p	t2, 1*8(t0)

	# figure out where the base of this code is
	subq	t0, 8, t0
	addq	t0, 4*32, t0	 	# the size of this stripped image, believe me	
	bis	t1, zero, t5

	# t0 = current image address, t1 = future image address, t2 = size in quads
	# t5 = saved future image address.
2:
	ldq_p	t3, 0(t0)		# get next data quadword
	stq_p	t3, 0(t1)		# save it
	lda	t0, 8(t0)		# increment from pointer
	lda	t1, 8(t1)		# increment to pointer
	subq	t2, 1, t2		# decrement the quadword count
	bgt	t2, 2b

	# OK, we've put the image someplace.  Now get it out to memory
	# and make sure that the I-cache is in a fit state.  That is, flush
	# it so that no old (valid) instructions are in it.
	mb
	mb
	hw_mtpr	zero, flushIc
	jsr	zero, (t5)		# and jump to it.

	.end	__relocate
