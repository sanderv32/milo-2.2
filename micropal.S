/*****************************************************************************

       Copyright © 1993, 1994 Digital Equipment Corporation,
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

#if 0
.data
.asciz "$Id: micropal.s,v 1.13 1995/12/14 00:17:49 fdh Exp $"
#endif
.text

/*
**  MODULE DESCRIPTION:
**
**	Huffman decompression start up file.
**
**	This program runs without the benefit of PALcode, and so it must
**	be careful when accessing memory.  There are no DTB misses as
**	this code establishes an environment that is 8 MB.  Two TBs are
**	set up that map addresses 0 through 8 MB contiguously with 512
**	page entries.  This code runs in PALmode, but data stream misses
**	are normal.
**
**	If necessary, this program will relocate itself so that as we
**	decompress we don't step on ourselves.
**
**	After relocating, we then do the actual decompression.  The
**	compressed image contains a header describing where in memory
**	the decompressed image really wants to live.  This code
**	relocates itself up to 300000 which gives some amount of stack
**	space and plenty of room for expansion.  It actually relocates
**	itself up or down in memory to where the label, Start, is
**	pointing.  Care must be taken not to over write ones self.
**	This code assumes a stack size of 64K, a base heap of 32K, and
**	a work area of 32K.
**
**
** $Log: micropal.s,v $
** Revision 1.13  1995/12/14  00:17:49  fdh
** Added LEDWRREG macro that is used to print register
** contents to the LED card.
**
** Revision 1.12  1995/12/08  13:34:56  fdh
** Delay setting PALbase until setting up to call decompression code.
** Modified the LEDWRITE macro and removed some associated pvc labels.
**
** Revision 1.11  1995/12/07  07:41:10  fdh
** Modified to accept return value from decompression code
** as the base of the decompressed image.
**
** Revision 1.10  1995/12/07  07:13:10  fdh
** Modified to use HFCOMP header structure... includes header.h.
** The decompressed image destination now comes from this header
** instead of the previous hardcoded value.
**
** Removed the restore from the PALbase register to reestablish
** the pointer to the Data area for restoring register state
** because I can figure it out with a branch instead.
**
** Moved some general MACRO definitions to header.h.
**
** Added some additional debug tracing when compiled with DEBUG defined.
**
** Revision 1.9  1995/12/02  21:19:16  fdh
** Use VMS PALcode cserve (cserve$jump_to_arc) to restart
** in PALmode when booted by the SRM console.
**
** Revision 1.8  1995/11/28  23:16:36  fdh
** If we've been SRM console "booted", and therefore running
** virtual, and not in PALmode, we drop back into PAL mode.
** Otherwise, we've been started in PALmode, just as if we
** were started from the serial ROM.
**
** Revision 1.7  1995/11/16  15:26:34  fdh
** Added the capability for the image to relocate itself
** if necessary before calling the decompression code.
** If the starting location and the desired destination
** causes the image to overlap then the image will be moved
** to an intermediate location first so that it is not
** corrupted in the process.
**
** Fixed up copyright header and log information formats.
** Added defaults for compile-time variables.
**
** Prints out additional information whe built with the
** DEBUG variable defined.
**
** Revision 1.6  1995/05/19  20:24:11  fdh
** Consolidated code duplication.
** Made uncompressed image entry point a compile
** time parameter throught the DECOMP_DESTINATION macro.
**
** Revision 1.5  1995/04/18  20:15:43  fdh
** Separated DC21064 and DC21164 conditionals around
** the #include "dc21x64.h" lines to allow makedepend
** to work properly.
**
** Revision 1.4  1995/03/20  20:48:39  cruz
** Added code for EV5 support.
**
** Revision 1.3  1995/03/05  04:13:31  fdh
** Modified to save state of all gpr's.  Only pt0 state is lost.
**
** Revision 1.2  1995/02/01  19:21:45  curewitz
** added pvc labels for pvc checking
**
** Revision 1.1  1995/02/01  16:40:56  curewitz
** Initial revision
*/

	.set noat

#ifdef DEBUG

#ifdef DC21164
#define DEBUG_ALCOR
#endif /* DC21164 */

#ifdef DC21064
#define DEBUG_APECS
#endif /* DC21064 */

#ifdef DC21066
#define DEBUG_21066
#endif

#endif /* DEBUG */

#ifdef DC21164
#include	"dc21164.h"		/* Alpha 21164 specific definitions */
#ifndef STALL
#define STALL mfpr $31, pt0
#endif
#endif

#if defined(DC21064) || defined(DC21066)
#include	"dc21064.h"		/* Alpha 21064 specific definitions */
#ifndef STALL
#define STALL mtpr $31, 0
#endif
#endif

#ifdef DEBUG_ALCOR
#define LEDPORT		0x80
#define IO_SHIFT 	5
#define PCI_SPARSE 	0x0858
#define BASE_SHIFT	28
#define WaitDelay	0x800
#endif /* DEBUG_ALCOR */

#ifdef DEBUG_APECS
#define LEDPORT		0x80
#define IO_SHIFT 	5
#define PCI_SPARSE 	0x1C
#define BASE_SHIFT	28
#define WaitDelay	0x400
#endif /* DEBUG_APECS */

#ifdef DEBUG_21066
#define LEDPORT		0x80
#define IO_SHIFT 	5
#define PCI_SPARSE 	0x1C
#define BASE_SHIFT	28
#define WaitDelay	0x400
#endif /* DEBUG_21066 */

#if defined (DEBUG_ALCOR) || defined (DEBUG_APECS) || defined(DEBUG_21066)
#define LEDWRITE(value, reg1, reg2)			\
    lda		reg1, PCI_SPARSE($31);			\
    sll 	reg1, (BASE_SHIFT-IO_SHIFT), reg1;	\
    lda 	reg1, LEDPORT(reg1);			\
    sll 	reg1, IO_SHIFT, reg1;			\
    bis 	$31, value, reg2; 			\
    stl_p	reg2, 0(reg1);				\
    mb;							\
    ldah	reg1, WaitDelay($31);			\
0:  subq	reg1, 1, reg1;				\
    bne		reg1, 0b;

#define LEDWRREG(inreg, reg1, reg2, reg3, reg4)	\
	LEDWRITE(0xEE, reg1, reg2)	/* Signal that 8 bytes coming	*/\
	bis	$31, 8, reg3;		/* Load up byte counter		*/\
	bis	inreg, $31, reg4;	/* Move Address into reg4	*/\
1:	LEDWRITE(reg4,reg1,reg2)	/* Write it out to the port	*/\
	subq	reg3, 1, reg3;		/* Decrement counter		*/\
	srl	reg4, 8, reg4;		/* Shift next byte down		*/\
	bne	reg3, 1b;		/* Are we done yet?		*/\
	LEDWRITE(0xEE, reg1, reg2)	/* Output Done Signal		*/
#else
#define LEDWRITE(value, reg1, reg2)
#define LEDWRREG(inreg, reg1, reg2, reg3, reg4)
#endif

#define ldq_p		hw_ldq/p
#define stq_p		hw_stq/p

	.globl Start

	.text	0
Start:
	bis	$31, $31, $31		/* NOP */
	bis	$31, $31, $31		/* NOP */
/*
**	Since we can't be guaranteed to be running in PALmode at this
**	point general purpose register state can not be saved to
**	PALtemp registers with the mtpr instructions. We do not yet
**	know if there is PALcode present that can provide cserve
**	functions to do what is needed.  Therefore, by the time that
**	we are guaranteed to be in PALmode the original state of
**	registers R27, R28, R30 and R29 could have been lost.
*/
	br	r27, CheckMode		/* Branch over static data. */

	.align 3			/* Ensure quadword alignment */
Data:
	.quad	DECOMP_PALBASE		/* location of this code */
	.quad	0			/* 0 */
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0			/* 10 */
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0			/* 20 */
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0			/* 30 */
	.quad	0			/* 31 */
	.quad	0                       /* 32 */

	.align 3			/* Ensure quadword alignment */
CheckMode:
	addq	$27, 4, $27		/* Skip any alignment padding */
	bic	$27, 7, $27		/* $27 now points to the data area */
	lda	$27, (Start-Data)($27)	/* $27 now points to the Start */

/*
**	If we've been SRM console "booted", and therefore running
**	virtual, and not in PALmode, we drop back into PAL mode.
**	Otherwise, we've been started in PALmode, just as if we
**	were started from the serial ROM.
**
**	If we're at any of the following addresses, we assume we're
**	virtual.  All other addreses we assume we're physical:
**
**	00000000	virtual, loaded by APB
**	20000000	virtual, standard SRM boot
**	all else	assume physical (PALmode)
*/
#if 0
	beq	$27, RunningVirtual	/* check for 00000000 */
#endif
	ldah	$28, 0x2000($31)	/* check for 20000000 */
	subq	$27, $28, $28
	bne	$28, PALContinue

/* Use VMS call_pal to read virtual page table base address */
#define PAL_FUNC_MFPR_VPTBASE	(41)
#define mfpr_vptb	call_pal	PAL_FUNC_MFPR_VPTBASE

/* Offset from Start to the memsize field of the parameters block */
#define PARAM_MEMSIZE	(DECOMP_PARAMS - DECOMP_PALBASE + 768)

RunningVirtual:
	bis	r0, $31, r29		/* Save r0 into scratch r29 */
	/* Check if the memory size has been passed in the parameters block */
	ldl	r0, (PARAM_MEMSIZE+4)(r27)	/* memory size in Mb */
	bne	r0, 2f

	/* This ought to be (Data-Start+8)(r27), but the buggy gas
	   wants $at for that!  We use 32 instead of 24, because it doesn't
	   matter which slot to use. */
	stq	r1, 32(r27)		/* Save r1 */
	/* Calculate memory using HWRPB */
	ldah	r28, 0x1000(r31)	/* Virtual address of HWRPB */
	ldq	r30, 200(r28)		/* Offset to MDDT */
	addq	r28, r30, r28		/* memory data descriptor table */
	ldq	r30, 16(r28)		/* number of clusters */
1:
	ldq	r1, 32(r28)		/* number of pages in this cluster */
	subq	r30, 1, r30
	addq	r28, 56, r28		/* next cluster */
	addq	r0, r1, r0		/* total number of pages */
	bne	r30, 1b
	/* Here we assume that the page size is 8192.  At the moment Linux
	 * can't work with other page sizes anyway. */
	srl	r0, 7, r0		/* size in Mb */
	sll	r0, 32, r0
	lda	r0, 0x494d(r0)
	ldah	r0, 0x4f4c(r0)		/* attach "MILO" signature */
	stq	r0, PARAM_MEMSIZE(r27)

	ldq	r1, 32(r27)		/* Restore r1 (oh gas) */
2:
pvc$huf16$1002:
	mfpr_vptb     			/* R0 = virtual page table base address. */

	sll	$27, (64-43), $28	/* get rid of sign extension */
	srl	$28, ((64-43)+10), $28	/* make bits 32:0 of va of pte */
	bic	$28, 7, $28		/* quad word align */
	bis	r0, $28, r0		/* make the real address */

	ldq	r30, 00(r0)		/* pick up the pte. */

	srl	r30, 32, r30		/* Convert PTE to PFN */
	sll	r30, 13, r30		/* and then to Physical Address */

#define PAL_FUNC_CSERVE		(9)

#define USE_VMS_JUMP_TO_ARC
#ifdef USE_VMS_JUMP_TO_ARC
	bis	r16, $31, $28		/* Save r16 into scratch $28 */
	bis	r17, $31, $27		/* Save r17 into scratch $27 */

#define cserve_jump_to_arc	(69)

	bis	r30, 1, r17
	lda	r16, cserve_jump_to_arc($31)
#else
	bis	r16, $31, $28		/* Save r16 into scratch $28 */
	bis	r18, $31, $27		/* Save r18 into scratch $27 */

#define CSERVE_K_JTOPAL		(9)

	bis	r30, r31, r16
	lda	r18, CSERVE_K_JTOPAL($31)
#endif

pvc$huf15$1002:
	call_pal PAL_FUNC_CSERVE	/* Load the new EXC_ADDR, return in PAL */
	/* br	$31, PALContinue	-- I have no idea what it is for. */
	halt				/* Let's just halt instead. */

#ifdef DC21164
/*
**	DC21164 PALcode entry point for DTB miss flow
*/
	. = 0x200

PalDtbMiss:

	mtpr	r0, pt6			/* Save r0 */
	mtpr	r1, pt7			/* Save r1 */

	mfpr	r0, va			/* Get VA of target reference */
	srl	r0, 13, r1		/* Get rid of byte-within-page bits. */
	sll	r1, 32, r1		/* Fabricate PFN */
	lda	r1, 0x1101(r1)		/* Fabricate protection bits */

	mtpr	r1, dtbPte		/* Write PTE into TB and set valid */
	STALL
	mtpr	r0, dtbTag		/* Set up the tag (writes PTE and tag simultaneously) */

	mfpr	r0, pt6			/* Restore scratch registers */
	mfpr	r1, pt7			/* */
	STALL				/* Stall due to tbTag. */
	hw_rei				/* Return and try access again */

#endif /* DC21164 */


	.align 3			/* Ensure quadword alignment */
PALContinue:
/*
**	Save the General Purpose Registers in memory
*/
	lda	$27, (Data-Start)($27)	/* $27 again points to the data area */
	stq_p	$0,  1*8($27)		/* save gpr's */
	stq_p	$1,  2*8($27)		/* save gpr's */
	stq_p	$2,  3*8($27)
	stq_p	$3,  4*8($27)
	stq_p	$4,  5*8($27)
	stq_p	$5,  6*8($27)
	stq_p	$6,  7*8($27)
	stq_p	$7,  8*8($27)
	stq_p	$8,  9*8($27)
	stq_p	$9,  10*8($27)
	stq_p	$10, 11*8($27)
	stq_p	$11, 12*8($27)
	stq_p	$12, 13*8($27)
	stq_p	$13, 14*8($27)
	stq_p	$14, 15*8($27)
	stq_p	$15, 16*8($27)
	stq_p	$16, 17*8($27)
	stq_p	$17, 18*8($27)
	stq_p	$18, 19*8($27)
	stq_p	$19, 20*8($27)
	stq_p	$20, 21*8($27)
	stq_p	$21, 22*8($27)
	stq_p	$22, 23*8($27)
	stq_p	$23, 24*8($27)
	stq_p	$24, 25*8($27)
	stq_p	$25, 26*8($27)
	stq_p	$26, 27*8($27)
	stq_p	$27, 28*8($27)		/* Probably trashed */
	stq_p	$28, 29*8($27)		/* Probably trashed */
	stq_p	$29, 30*8($27)		/* Probably trashed */
	stq_p	$30, 31*8($27)
	mb
	mb

	LEDWRITE(0xc0,r16,r17)


CheckLocation:
	ldq_p	$29, 0($27)		/* r29 = image destination */
	lda	$6, (Start-Data)($27)	/* r6 = current image location */

	LEDWRREG(r6,r16,r17,r19,r24)	/* Print out current location to LED Port */

	cmpeq	$6, r29, r7		/* If r29 != r6 then move. */
	blbs	$7, SkipMove		/* else don't move */

	LEDWRITE(0xc1,r16,r17)
	bis	$29, $31, r19		/* r19 Gets updated during copy */
	ldah	$15, 0x40($31)		/* Image size counter. Assume 4MB for now */
	subq	$29, r6, r5		/* r5 = Destination-Source */
	subq	$31, r5, r4		/* Load neg(Destination-Source) */
	cmovlt	$5, r4, r5		/* r5 = abs(Destination-Source) */
	cmple	$15, r5, r3		/* ImageSize <= abs(Destination-Source) */
	blbs	$3, NoOverlap		/* Will source & destination overlap */
	cmplt	$6, r29, r3		/* r3 = (Source < Destination) */
	subq	$6, r15, r4		/* Move down from source? */
	subq	$29, r15, r19		/* Move down from destination? */
	cmovlbs	$3, r4, r19		/* Use the lower address */
	bge	$19, NoOverlap		/* Throw away destination if < 0 */
	addq	$6, r15, r4		/* Move up from source? */
	addq	$29, r15, r19		/* Move up from destination? */
	cmovlbc	$3, r4, r19		/* Use the higher address */

NoOverlap:
	LEDWRITE(0xc2,r16,r17)
	bis	$19, $31, r29		/* r29 = intermediate image destination */
Move:	ldq_p	$18, 00(r6)		/* Copy from address in r6 */
	subq	$15, 8, r15		/* Decrement image size counter */
	stq_p	$18, 00(r19)		/* Copy to address in r19 */
	addq	$6, 8, r6		/* increment source pointer */
	addq	$19, 8, r19		/* increment destination pointer */
	bge	$15, Move		/* Are we done yet? */
	br	$31, RestoreState	/* Go restore and continue at new destination */

SkipMove:
/*
**	Setup to call decompression code.
**	$30 is stack pointer.
**	$26 is Return Address.
*/
	mb
	mb
	mtpr	$29, palBase		/* set up palbase for dtb miss code */
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL

	LEDWRITE(0xc3,r16,r17)

	bis	$29, $31, r30		/* Init stack pointer to be the PAL_BASE  */
	br	$20, CallInfo		/* Branch over static data */
	.align 3			/* Ensure quadword alignment */
	.quad	DECOMP_BASE		/* address of decompressor */
CallInfo:
	addq	$20, 4, r20		/* Skip any alignment padding */
	bic	$20, 7, r20
	ldq_p	$21, 0(r20)		/* r21 = Pointer to decompression code  */

	LEDWRITE(0xc4,r16,r17)

pvc$huf1$2001:
	jsr	$26, (r21)		/* jump to (r21) decompression code */
pvc$huf4$2002.1:
	bis	$0, $31, r29		/* Load Decompressed image location */

	LEDWRITE(0xc5,r16,r17)

RestoreState:
	bis	$29, 0x1, r29		/* Enable palmode */
	LEDWRREG(r29,r16,r17,r19,r24)	/* Print out destination to LED Port  */
	mtpr	$29, excAddr
	STALL
	STALL
	STALL
	STALL

	br	$27, Reload
Reload:	lda	$27, (Data-Reload)($27)	/* $27 again points to the data area  */
	LEDWRITE(0xc6,r16,r17)

	ldq_p	$0, 1*8($27)		/* begin restore registers */
	ldq_p	$1, 2*8($27)		/* begin restore registers  */
	ldq_p	$2, 3*8($27)
	ldq_p	$3, 4*8($27)
	ldq_p	$4, 5*8($27)
	ldq_p	$5, 6*8($27)
	ldq_p	$6, 7*8($27)
	ldq_p	$7, 8*8($27)
	ldq_p	$8, 9*8($27)
	ldq_p	$9, 10*8($27)
	ldq_p	$10,11*8($27)
	ldq_p	$11,12*8($27)
	ldq_p	$12,13*8($27)
	ldq_p	$13,14*8($27)
	ldq_p	$14,15*8($27)
	ldq_p	$15,16*8($27)
	ldq_p	$16,17*8($27)
	ldq_p	$17,18*8($27)
	ldq_p	$18,19*8($27)
	ldq_p	$19,20*8($27)
	ldq_p	$20,21*8($27)
	ldq_p	$21,22*8($27)
	ldq_p	$22,23*8($27)
	ldq_p	$23,24*8($27)
	ldq_p	$24,25*8($27)
	ldq_p	$25,26*8($27)
	ldq_p	$26,27*8($27)
	/* Don't restore $27.  It's been trashed anyway */
	ldq_p	$28,29*8($27)		/* Probably trashed */
	ldq_p	$29,30*8($27)		/* Probably trashed */
	ldq_p	$30,31*8($27)		/* end restore registers */



FlushIcache:
	mb
	mb
	STALL
	STALL

#if defined(DC21064) || defined(DC21066)
	mtpr	$31, flushIc		/* Flush the I-cache */
	mtpr	$31, xtbZap		/* Flush the TBs */
#endif /* DC21064 */

#ifdef DC21164
	mtpr	$31, icFlush		/* Flush the I-cache */
	mtpr	$31, itbIa		/* Flush the TBs */
	mtpr	$31, dtbIa		/* Flush the TBs */
	mtpr	$31, ips		/* set current mode to kernel */
	mtpr	$31, dtbCm		/* set current mode to kernel */
	mtpr	$31, dtbAsn		/* set asn to 0 */
	mfpr	$31, va			/* unlock va */
#endif /* DC21164 */

	mb
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	hw_rei


#ifdef PVCCHECK
/*
**	Dummy routine to simulate decompression code for pvc checking.
*/
dummy1:
pvc$huf2$2001.1:
pvc$huf3$2002:
	ret	$31, (r26)
#endif


#if defined(DC21064) || defined(DC21066)
/*
**	DC21064 PALcode entry point for DTB miss flow
*/
	. = 0x9E0

PalDtbMiss:

	mtpr	$0, pt6			/* Save r0 */
	mtpr	$1, pt7			/* Save r1 */
	mfpr	$0, va			/* Get VA of target reference */
	mtpr	$0, tbTag		/* Set up the tag */
	sll	$0, (32-13), r0		/* Fabricate PFN */
	lda	$1, 0xFF7($31)		/* Fabricate protection bits */
	sll	$1, 4, r1
	bis	$1, r0, r1		/* Form PTE */
	mfpr	$0, excAddr		/* Save exception address */
	mtpr	$1, dtbCtl		/* Select large or small page size */
	mtpr	$1, dtbPte		/* Write PTE into TB and set valid */
	mtpr	$0, excAddr		/* Restore exception address */
	mfpr	$0, pt6			/* Restore scratch registers */
	mfpr	$1, pt7
	hw_rei				/* Return and try access again */
#endif /* DC21064 */
