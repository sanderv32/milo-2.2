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
#include "autoconf.h"

/* Here are some VMS PALcode functions that we need in order to
 * switch to OSF PALcode...
 */

#define PAL_mfpr_pcbb    0x12
#define PAL_ldqp	 0x03
#define PAL_stqp	 0x04
#define PAL_halt_entry	 0x00
#define PAL_cserve_entry 0x09


	.set	noreorder

#if defined(__linux__) && !defined(__ELF__)
	.set singlegp
#endif

	.globl	__start
	.ent	__start
__start:
/*
 *  Get the gp register set up correctly.
 */
#if 0
1:	bis	zero,zero,zero		# spacer
	br	t0,2f			# offset 4
	.quad	_gp			# offset 8
2:	ldq	gp,0(t0)		# offset 16
#else
1:	br	t0,2f
2:	ldgp	gp,0(t0)
#endif
	lda	sp,-8(sp)		# create a stack frame
	stq     ra,  0(sp)		# Place return address on the stack
/*
 *  make sure that we're in kseg.
 */
	jsr	ra, __turn_on_kseg

/*
 *  zero out data at the end of the image
 */
	jsr	ra, doinitdata

/*
 *  Zero a few arguments and jump into boot_main.
 */
	bis	zero,zero,a0
	bis	zero,zero,a1
	bis	zero,zero,a2

	jsr	ra, __main

	ldq     ra, 0(sp)		# restore return address
	lda	sp, 8(sp)		# prune back the stack
	ret	zero, (ra)		# Back from whence we came
	.end	__start

/*
 *  Entry point for when we've jumped to the new palcode inside the
 *  miniloader.
 */
	.globl	__start_again
	.ent	__start_again
__start_again:
#if 0
	br	t0,3f			# offset 4
	.quad	_gp			# offset 8
3:	ldq	gp,0(t0)		# offset 16
#else
	br	t0,3f
3:	ldgp	gp,0(t0)
#endif

/*
 *  Zero a few arguments and jump into boot_main_again().
 */
	bis	zero,zero,a0
	bis	zero,zero,a1
	bis	zero,zero,a2

	jsr	ra, boot_main_cont

	call_pal PAL_halt
	.end 	__start_again

	.globl	panic
	.ent	panic 2
panic:
	.option O1
	.frame sp, 0, ra

	bis a4, zero, a5
	bis a3, zero, a4
	bis a2, zero, a3
	bis a1, zero, a2
	bis a0, zero, a1
	bis $26, zero, a0
	jsr zero, __panic
	.end 	panic

/*
 * Where compilers cannot dereference quadword addresses
 * (ie NT), then we must have a assembler routines for this.
 */
	.globl	ReadQ
	.ent	ReadQ 2
ReadQ:
	.option	O1
	.frame	sp, 0, ra

	ldq	v0, 0(a0)
	ret	zero, (ra)
	.end	ReadQ

	.globl	WriteQ
	.ent	WriteQ 2
WriteQ:
	.option	O1
	.frame	sp, 0, ra

	stq	a1, 0(a0)
	ret	zero, (ra)
	.end	WriteQ

/*
 * read_cpureg - Read a CPU register
 */

	.globl	read_cpureg_long
	.ent	read_cpureg_long
read_cpureg_long:
	ldl	v0,0(a0)
	ret	zero,(ra)
	.end	read_cpureg_long

/*
 * write_cpureg_long - Write a CPU register
 */
	.globl	write_cpureg_long
	.ent	write_cpureg_long
write_cpureg_long:
	stl	a0,0(a1)
	mb
	ret	zero,(ra)
	.end	write_cpureg_long

/*
 * read_cpureg_long - Read a CPU register
 */

	.globl	read_cpureg_quad
	.ent	read_cpureg_quad
read_cpureg_quad:
	ldq	v0,0(a0)
	ret	zero,(ra)
	.end	read_cpureg_quad

/*
 * write_cpureg_quad - Write a CPU register
 */
	.globl	write_cpureg_quad
	.ent	write_cpureg_quad
write_cpureg_quad:
	stq	a0,0(a1)
	mb
	ret	zero,(ra)
	.end	write_cpureg_quad


	.globl	wrfen
	.ent	wrfen
wrfen:
	lda a0, 1(zero)
	call_pal PAL_wrfen
	ret  	zero, (ra)
	.end wrfen


	.globl	swppal
	.ent	swppal
swppal: call_pal PAL_swppal
	.end	swppal

/*
 *  Convert to running in OSF KSEG (directly mapped address space).
 */
	.globl 	__turn_on_kseg
	.ent	__turn_on_kseg
__turn_on_kseg:
/*
 * First, the return address (which wasn't KSEG).
 */
	lda	t1, -32768(zero)
	sll	t1, 27, t1
	bis	ra, t1, ra
/*
 * Now the stack pointer.
 */
	bis 	sp, t1, sp

	ret	zero,(ra)
	.end	__turn_on_kseg

	.align 3
	.globl	amask
	.ent	amask
amask:
	lda $2,259
	.long 0x47e20c20     /* amask r2,r0 */
	ret ($26)
	.end amask

	.align 3
	.globl	implver
	.ent	implver
implver:
	.long 0x47e03d80     /* implver 1,r0 */
	ret ($26)
	.end implver


        .globl  cServe
        .ent    cServe 2
cServe:
        .option O1
        .frame  sp, 0, ra
        call_pal PAL_cserve_entry
        ret     zero, (ra)
        .end    cServe


        #
        # The following two functions are needed for supporting SRM PALcode
        # on the PC164 (at least), since that PALcode manages the interrupt
        # masking, and we cannot duplicate the effort without causing problems
        #

        .align 3
        .globl  cserve_ena
        .ent    cserve_ena
cserve_ena:
        .prologue 0
        bis     $16,$16,$17
        lda     $16,52($31)
        call_pal PAL_cserve
        ret     ($26)
        .end    cserve_ena

        .align 3
        .globl  cserve_dis
        .ent    cserve_dis
cserve_dis:
        .prologue 0
        bis     $16,$16,$17
        lda     $16,53($31)
        call_pal PAL_cserve
        ret     ($26)
        .end    cserve_dis


        .globl  halt
.ent    halt 2
halt:
.option O1
        .frame  sp, 0, ra
        call_pal PAL_halt
        ret     zero, (ra)
        .end    halt   

/* Set the current interrupt priority level, returning the previous IPL
 *
 * Calling sequence:
 *
 *      old_ipl = ipl(new_ipl)
 *
 * The SWPIPL pal function does this all for us 8-)
 */

        .globl  ipl
        .ent    ipl
ipl:
        call_pal        PAL_swpipl
        ret             zero,(ra)
        .end    ipl

        #
        # Having the delay loop out of line guarantees that we wont
        # run into weird alignment conditions (on new processors)
        # that vary the speed of the loop.
        #

        .align 5
        .globl  ___delay
        .ent    ___delay
___delay:
        .set noat
        .frame  $30,0,$28,0
        .prologue 0
1:      subq    $0,1,$0
        bge     $0,1b
        ret     $31,($28),0
        .set at
        .end    ___delay

