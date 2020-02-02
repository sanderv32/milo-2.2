/*
 *      VID: [T1.2] PT: [Fri Apr 21 14:23:01 1995] SF: [osfpal.s]
 *       TI: [/sae_users/fdh/bin/vice -iosfpal.s -l// -p# -DDC21064 -DEB64P -h -m -aeb64p ]
 */
/*
*****************************************************************************
**                                                                          *
**  Copyright © 1994							    *
**  by Digital Equipment Corporation, Maynard, Massachusetts.		    *
**                                                                          *
**  All Rights Reserved							    *
**                                                                          *
**  Permission  is  hereby  granted  to  use, copy, modify and distribute   *
**  this  software  and  its  documentation,  in  both  source  code  and   *
**  object  code  form,  and without fee, for the purpose of distribution   *
**  of this software  or  modifications  of this software within products   *
**  incorporating  an  integrated   circuit  implementing  Digital's  AXP   *
**  architecture,  regardless  of the  source of such integrated circuit,   *
**  provided that the  above copyright  notice and this permission notice   *
**  appear  in  all copies,  and  that  the  name  of  Digital  Equipment   *
**  Corporation  not  be  used  in advertising or publicity pertaining to   *
**  distribution of the  document  or  software without specific, written   *
**  prior permission.							    *
**                                                                          *
**  Digital  Equipment  Corporation   disclaims  all   warranties  and/or   *
**  guarantees  with  regard  to  this  software,  including  all implied   *
**  warranties of fitness for  a  particular purpose and merchantability,   *
**  and makes  no  representations  regarding  the use of, or the results   *
**  of the use of, the software and documentation in terms of correctness,  *
**  accuracy,  reliability,  currentness  or  otherwise;  and you rely on   *
**  the software, documentation and results solely at your own risk.	    *
**                                                                          *
**  AXP is a trademark of Digital Equipment Corporation.		    *
**                                                                          *
*****************************************************************************
**
**  FACILITY:	
**
**	DECchip 21064/21066 OSF/1 PALcode
**
**  MODULE:
**
**	osfpal.s
** 
**  MODULE DESCRIPTION:
** 
**	Common OSF/1 PALcode for the DECchip 21064/21066
** 
**  AUTHOR: ER
**
**  CREATION-DATE: 02-Feb-1994
** 
**  MODIFICATION HISTORY: 
**
**  $Log: osfpal.s,v $
**  Revision 2.12  1995/02/27  18:12:48  samberg
**  Both dc21064 and dc21066 crd logout flows save frame address to pt4
**
**  Revision 2.11  1995/02/01  16:11:00  samberg
**  In reset, save original t0 to pt0 before putting reason in t0
**
**  Revision 2.10  1995/01/10  18:42:53  samberg
**  (1) Use PTBR<63> as a physical mode indicator
**  (2) On reset, load HLT_K_RESET in t0 before going to enter_console
**
**  Revision 2.9  1995/01/10  15:31:45  ericr
**  Remove _edata gld defined label
**
**  Revision 2.8  1994/10/21  14:54:28  ericr
**  OR in BIU_CTL<OE> on reset and clear signature register a3
**
**  Revision 2.7  1994/08/10  17:06:26  samberg
**  Handle SIO NMI as system machine check
**  Load t4 with ptIntMask, so we don't load hier with junk
**
**  Revision 2.6  1994/06/30  15:14:37  samberg
**  Added srom revision and processor id parameters.
**  Write ptIntMask on swppal.
**
**  Revision 2.5  1994/06/16  14:51:24  samberg
**  For ANSI, changed $ to _, except for pvc labels
**
**  Revision 2.4  1994/05/19  17:47:51  samberg
**  Swap flag is now a 4 byte constant 'SWAP'
**
**  Revision 2.3  1994/05/19  15:34:47  samberg
**  In mchk, force ioc_stat and esr writes with double mb's
**
**  Revision 2.2  1994/04/20  18:17:22  jeffw
**  fix err_handler to increment PC before mchk decrement
**
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.16  1994/04/01  21:54:06  ericr
**  Added physical mode support on swppal and swpctx
**
**  Revision 1.15  1994/03/30  19:14:45  ericr
**  Removed TSIZE and DSIZE constants - replaced by _edata label
**
**  Revision 1.14  1994/03/30  16:35:51  ericr
**  Removed redundant loads of BUGCHECK error code in TB flows
**  Use LDLI macro to load expected firmware signature value
**
**  Revision 1.13  1994/03/29  20:54:08  ericr
**  Fixed ESR mask used for finding correctable errors
**
**  Revision 1.12  1994/03/29  19:33:19  ericr
**  Fixed restoring of PS and shadow during stack unwind on
**  fatal memory and I/O controller interrupts.
**
**  Revision 1.11  1994/03/28  19:44:02  ericr
**  Removed SAVE_PALS macro call from reset
**
**  Revision 1.10  1994/03/22  15:13:36  samberg
**  Incrementing PC past FETCH[_M] must be with addq, not addl
**
**  Revision 1.9  1994/03/18  17:42:09  samberg
**  Fix and converge dcStat manipulation in machine check
**
**  Revision 1.8  1994/03/15  23:50:27  ericr
**  Removed CALL_PALRSVD macros.
**  Moved BUILD_STACK_FRAME macro to osf.h
**
**  Revision 1.7  1994/03/14  20:51:02  ericr
**  Replaced code in reset that saves internal state.
**  Changed SWPPAL to allow swap to address 0.
**  Changed load_x_csr_base macros to use LOAD_REGION_BASE
**
**  Revision 1.6  1994/03/12  01:00:56  ericr
**  Changed line comment character to //
**
**  Revision 1.5  1994/03/10  15:13:38  ericr
**  Fixed ifdef conditional for writing BIU_CTL on reset
**
**  Revision 1.4  1994/03/09  22:57:41  ericr
**  Fixed numerous bugs related to ABOX_CTL and offsets into
**  the save state region of the impure scratch area
**
**  Revision 1.3  1994/03/02  21:55:26  ericr
**  Reworked hardware interrupt handling code
**
**  Revision 1.2  1994/02/28  22:26:58  jeffw
**  fix fen converted to opcdec to use pc+4
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
** 
*/
#if !defined(lint)
        .data
        .asciz "$Id: osfpal.s,v 2.12 1995/02/27 18:12:48 samberg Exp $"
#endif

	.set	noat

#include	"dc21064.h"	/* DECchip 21064 specific definitions	*/
#include	"osf.h"		/* OSF/1 specific definitions		*/
#include	"macros.h"	/* Common macro definitions		*/
#include        "impure.h"      /* Impure area data structure defs      */

#define PAL_K_MAJ       2       /* PALcode major version number         */
#define PAL_K_MIN       8       /* PALcode minor version number         */

#if !defined(PAL_K_REV)
#define PAL_K_REV       0       /* Platform specific revision number    */
#endif


/*======================================================================*/
/*                        LOCAL MACRO DEFINITIONS                       */
/*======================================================================*/

/*
** Default ICCSR and ABOX_CTL values
*/

#define ICCSR_K_INIT	(ICCSR_M_FPE | ICCSR_M_MAP | ICCSR_M_DI	 | \
			 ICCSR_M_BHE | ICCSR_M_JSE | ICCSR_M_BPE | \
			 ICCSR_M_PIPE)

#define	ABOX_K_INIT	(ABOX_M_DC_ENA | ABOX_M_SPE_2 | ABOX_M_IC_SBUF_EN)


/*======================================================================*/
/*                   EXTERNAL ENTRY POINT DECLARATIONS                  */
/*======================================================================*/

        .global pal_mchk_logout
	.global pal_mchk_kernel_mode_trap
	.global pal_interrupt_post
	.global	pal_lockCell
	.global pal_impure

	.global sys_mchk
	.global sys_mchk_logout
	.global sys_crd_logout
	.global sys_scrub_mem
	.global sys_interrupt
	.global	sys_cflush
	.global sys_cserve
	.global sys_enter_console
	.global sys_reset
	.global sys_reset_swppal


/*======================================================================*/
/*           DECCHIP 21064/21066 HARDWARE VECTOR ENTRY POINTS           */
/*======================================================================*/
/*
**                    Offset   Size
** Entry Point        (hex)   (bytes)	Why We're Here
** ---------------    ------  -------	---------------------------------
**
** Reset               0000      32	Reset request
**
** Machine Check       0020      64	Uncorrected hardware error
**
** Arithmetic Trap     0060     128	Arithmetic exception
**
** Interrupt           00E0     256	Interrupt, or corrected
**                                      hardware error
**
** D-stream Fault      01E0     512	ACV, FOR, FOW
**
** I-stream TB Miss    03E0    1024	ITB miss
**
** I-stream ACV        07E0     256	I-stream access violation
**
** D-stream TB Miss    08E0     256	DTB miss from native code
** (native)
**
** D-stream TB Miss    09E0     512	DTB miss from PAL code
** (PAL)
**
** Unaligned Data      11E0     512	Unaligned data access
**
** Illegal Opcode      13E0    1024	Reserved or privileged opcode
**
** Floating Point      17E0    2080	FP operation attempted with:
** Disabled
**					FP instructions disabled.
**
**                                      FP IEEE round to +/- infinity.
**
**					FP IEEE with data type other
**                                      than S,T,Q
**
** Call PAL            2000             CALL_PAL instruction.
**					128 locations based on
**					instruction bits <7> & <5:0>
*/
	.text	0
	. = 0x0000

pal_base:


/*
**
** FUNCTIONAL DESCRIPTION:
** 
** Reset performs the following functions:
**
**	- Saves the previous PAL base
**	- Establishes a new PAL base
**	- Disables all interrupts
**	- Performs chip specific initialization
**	- Clears the ITB and DTB
**	- Clears any existing exceptions
**	- Zeroes and restarts the cycle counter
**	- Clears the intr_flag
**	- Transfers to console/firmware in kernel mode
** 
** CALLING SEQUENCE:
** 
**	Vectored into by hardware-activated dispatch, when the
**	DECchip 21064/21066 reset line is pulled low, or via the 
**	SWPPAL privileged OSF/1 CALL_PAL instruction.
** 
** INPUT PARAMETERS:  
**
**   DECchip 21064 specific parameters:
**
**	t0 - aboxCtl
**	t1 - biuCtl
**
**   DECchip 21066 specific parameters:
**
**	t0 - aboxCtl
**	t1 - bankConfig[0..1] (bankConfig0 in lower 32 bits)
**	t2 - bankConfig[2..3] (bankConfig2 in lower 32 bits)
**	t3 - bankMask[0..1]   (bankMask 0 in lower 32 bits)
**	t4 - bankMask[2..3]   (bankMask 2 in lower 32 bits)
**
**   Firmware specific parameters:
**
**	s6 - Encoded srom.s RCS revision.
**	a0 - Processor identification (a la SRM).
**	a1 - Size of contiguous, good memory in bytes.
**	a2 - Cycle count in picoseconds.
**	a3 - Signature bits, and system revision id.
**	a4 - Active processor mask.
**	a5 - System context value.
** 
**   SWPPAL specific parameters:
** 
**	a1 - New PC
**	a2 - New PCBB
**	a3 - New VptPtr
**
** OUTPUT PARAMETERS:
** 
**      None
** 
** SIDE EFFECTS:
**
**	Registers t0 .. t7, and s0 are UNPREDICTABLE upon return.
** 
*/ 
	HDW_VECTOR(PAL_RESET_ENTRY)

pal_reset:
	mtpr	t0, pt0			/* Save t0. */
	br	t0, pal_reset_cont	/* Branch over static data. */

pvc$osf46$1003:				/* Ignore bogus PVC halt violation. */
pal_lockCell:
	.long	0			/* Lock cell for LDL/STC. */
	.long	(PAL_K_MAJ<<8) | (PAL_K_MIN<<0)
	.long	(PAL_K_REV<<0)
	.long	0	
	.long	pal_impure              /* Pointer to impure scratch area. */
        .long   0

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The machine check handler sorts out the reason for the error,
**	builds a logout frame in the impure scratch area, builds a 
**	stack frame on the kernel stack and dispatches to the kernel 
**      interrupt trap handler pointed to by the entInt operating 
**      system entry point. 
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a machine 
**	check occurs.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Code indicating machine check entry type
**	a1 - Platform-specific interrupt vector (SCB vector)
**	a2 - Kseg address of the logout area
** 
** SIDE EFFECTS:
** 
**	The current mode of the processor may be switched
**	from user to kernel.
**
*/ 
	HDW_VECTOR(PAL_MCHK_ENTRY)

pal_mchk:
/*
** It is possible that a MCHK was caused by multiple single bit 
** I-stream errors from which we can recover.  Flush the I-cache 
** now in case it is this type of error.
*/
	mtpr	zero, flushIc		/* flush the icache */
	STALL				/* stall for flush */
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL

	mtpr	zero, altMode		/* make sure alt_mode is KERNEL mode */
	br	zero, pal_mchk_cont	/* continue elsewhere */

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The arithmetic exception handler switches to kernel mode, builds
**	a stack frame on the kernel stack, loads the GP with the KGP, 
**	loads the exception summary flags into a0, loads the dirty register
**	mask into a1, and dispatches to the arithmetic trap handler 
**	pointed to by the entArith operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when an arithmetic 
**	exception occurs.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Exception summary register
**	a1 - Exception register write mask
**	a2 - UNPREDICTABLE
** 
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/ 
	HDW_VECTOR(PAL_ARITH_ENTRY)

pal_arith:
	mtpr	t0, pt0                 /* save t0 */
	mfpr	t0, excAddr		/* get pc */
	mtpr	t1, pt1                 /* save t1 */
	mfpr	t1, pt9			/* get shadow copy of ps */
	blbs	t0, pal_mchk_pal 	/* check for arith trap in PAL mode */
	subq	t0, 2, t0		/* start to get pc-4 when exc_addr<1>=0 */
	mtpr	t3, pt3			/* save t3 */
	bic	t0, 2, t0		/* finish getting pc-4 */
	mfpr	t3, va			/* unlock mbox */

	BUILD_STACK_FRAME(t1,t0)	/* build the stack frame */

 	mfpr	t0, excSum		/* get the exc_sum, plus F31 (always 0) */
	bis	zero, 63, t1		/* get a counter */
	mtpr	zero, excSum		/* clear so we just get MSK on read */

	bis	zero, zero, a1		/* clear register accumulator */
	srl	t0, EXC_V_SWC, a0	/* shift exception summary into a0 */

	bis	zero, zero, t0		/* F31 never traps so always 0 */
	br	zero, pal_arith_cont	/* continue flow elsewhere ... */

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The interrupt handler switches mode to kernel, builds a stack
**	frame on the kernel stack, and dispatches to the system specific 
**	interrupt handler.  Upon return from the system specific interrupt
**	handler, loads the new hier interrupt mask, loads the new IPL, 
**	loads the GP with the KGP, and dispatches to the kernel interrupt 
**	handler pointed to by the entInt operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when an interrupt 
**	occurs.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Interrupt type
**	a1 - Platform-specific interrupt vector (SCB vector)
**	a2 - UNPREDICTABLE
**
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel, and the current IPL raised to the priority
**	of the interrupt.
** 
*/ 
	HDW_VECTOR(PAL_INTERRUPT_ENTRY)

pal_interrupt:
	mtpr	t3, pt3                 /* Save t3. */
	mfpr	t3, hirr		/* Get pending interrupt requests. */
	mtpr	t1, pt1                 /* Save t1. */
	mfpr	t1, hier		/* Get hardware interrupt enables. */
	mtpr	t0, pt0                 /* Save t0 */
	mfpr	t0, pt9			/* Get shadow copy of PS. */

	and	t1, t3, t3		/* Only check requests that are enabled. */
	mfpr	t1, va			/* Unlock mbox (which is lower priorty). */
	beq	t3, 1f                  /* Check for a passive release. */
	mfpr	t1, excAddr		/* Get PC. */
	bic	t1, 3, t1		/* Clear PC<1:0> bits. */

	BUILD_STACK_FRAME(t0,t1)
/*
** Parse the HIRR value in t3 to handle the highest priority
** pending interrupt ...
*/
/*
** Check for a Correctable Read Data (CRD) interrupt.
*/
	and	t3, HIRR_M_CRR, t1
	bne	t1, pal_crd_handler
/*
** Check for a Performance Counter interrupt.
*/
        lda     t1, (HIRR_M_PC0 | HIRR_M_PC1)(zero)
        and     t3, t1, t1
        bne     t1, pal_pc_handler
/*
** Go check for any platform-specific interrupts.
*/
	br	zero, sys_interrupt
/*
** Passive release ...
*/
        ALIGN_BRANCH_TARGET

1:	mfpr	t3, pt3			/* Restore scratch registers. */
	mfpr	t1, pt1
	mfpr	t0, pt0			
	hw_rei				/* Dismiss the interrupt. */

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The D-stream TB fault handler switches mode to kernel, builds a stack
**	frame on the kernel stack, loads the GP with the KGP, loads the VA
**	of the faulting D-stream reference into a0, sorts out the reason for
**	the fault (ignoring faults on FETCH and FETCH_M instructions called 
**	from native mode), loads the appropriate memory management fault code 
**	into a1 (ACV or FOx), loads a value of either 0 or 1 into a2 (D-read
**	or D-write), and dispatches to the memory management fault handler
**	pointed to by the entMM operating system entry point.
** 
** CALLING SEQUENCE:
** 
**	Vectored into via hardware-activated dispatch when a D-stream
**	memory management fault occurs.
** 
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - VA of faulting reference
**	a1 - MM fault code (ACV or FOx)
**	a2 - Reference type (D-read or D-write)
** 
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/
	HDW_VECTOR(PAL_D_FAULT_ENTRY)

pal_dtb_fault:
	mtpr	t3, pt3			/* Save t3. */
	mtpr	t4, pt4			/* Save t4. */
	mtpr	t5, pt5			/* Save t5. */
	mtpr	t6, pt6			/* Save t6. */
	mfpr	t3, va			/* Get the VA of faulting reference. */
	mfpr	t4, mmcsr		/* Get the memory mgmt status. */
	mfpr	t6, excAddr		/* Get the PC of faulting instruction. */
	srl	t4, MMCSR_V_OPC, t5	/* Get opcode of faulting instruction. */
	blbs	t6, 2f			/* Check for faults in PAL mode. */
	cmpeq	t5, 0x18, t5		/* Is this a FETCH[_M] instruction? */
	bne	t5, 1f			/* If so, ignore. */
	mfpr	t5, pt9			/* Get the current processor status. */

	BUILD_STACK_FRAME(t5,t6)

	bis	zero, t3, a0		/* Move VA of faulting reference to a0. */
	and	t4, MMCSR_M_FAULT, t5	/* Isolate MM_CSR fault bits. */
  	srl	t5, 1, t5		/* Get fault bits into correct position. */
	and	t4, MMCSR_M_ACV, t3	/* Check for access violation. */
	cmovne	t3, MM_K_ACV, t5	/* Resolve fault - ACV overrides FOx. */
	bis	zero, t5, a1		/* Move MM fault code to a1. */
	and	t4, MMCSR_M_WR, a2	/* Establish read or write reference. */
	mfpr	t6, ptEntMM		/* Get kernel MM fault handler entry. */
	mfpr	t3, pt3			/* Restore scratch registers. */
	mfpr	t4, pt4
	mfpr	t5, pt5
	mtpr	t6, excAddr		/* Establish kernel entry point address. */
	mfpr	gp, ptKgp		/* Load the kernel global pointer. */
	mfpr	t6, pt6			/* Restore t6. */
	hw_rei				/* Transfer to kernel MM fault handler. */
 /*
** Ignore faults on FETCH[_M]. Make as if the instruction was 
** never executed.
**
** t5 - scratch
** t6 - address of faulting instruction
*/
	ALIGN_BRANCH_TARGET

1:	addq	t6, 4, t6		/* Increment past FETCH[_M] instruction. */
	mfpr	t5, va			/* Unlock VA. */
	mtpr	t6, excAddr		/* Load new PC address. */
 	mfpr	t3, pt3			/* Restore scratch registers. */
	mfpr	t4, pt4
	mfpr	t5, pt5
	mfpr	t6, pt6
	hw_rei				/* Resume execution. */
/*
** If we get here it's because a fault occured during one of the 
** following virtual references in PAL mode:
**
**	- fault when loading the PTE in a native TB miss handler
**	- fault when storing/loading to/from the kernel stack
**
** t5 - opcode of faulting instruction
**
** >>>>>>>>>>>>>>>>>>>>>>>>>> WARNING! <<<<<<<<<<<<<<<<<<<<<<<<<<
** This code discriminates between the two cases above solely on 
** the basis of the faulting instruction's opcode!  Care must be 
** taken to insure that a virtual HW_LD is not done anywhere else 
** other than in the native TB miss handlers, otherwise, how will 
** we tell the difference?
*/
	ALIGN_BRANCH_TARGET

2:	xor	t5, 0x1b, t5		/* Is this a HW_LD? */
	bne	t5, pal_mchk_ksp_not_valid /* If not, must be ksp problem. */
	br	zero, pal_mchk_pal	/* Page table ACV or FOx. */

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Fetch the PTE of the target virtual address using a virtual
**	access method.  If the PTE is valid then load it into the ITB
**	and continue, otherwise, take TNV, ACV, or FOE exception.
**
**      Physical mode may be enabled by setting VptPtr<0> thus allowing
**      a simple 1-to-1 va -> pa mapping to be used in place of the
**      page table translation.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when an I-stream
**	TB miss occurs.
**
**	Since I-stream memory mapping is disabled while in PAL mode, 
**	this routine can only be called as a result of a native (non-PAL) 
**	mode I-stream reference. 
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	t3 - VA of faulting reference
**	t4 - Scratch
**	t5 - PTE
**	t6 - Address of faulting instruction
**
** SIDE EFFECTS:
**
**	The virtual access method used to fetch the PTE of the
**	target reference may result in a trap to the PAL mode
**	DTB miss fault handler.
*/
	HDW_VECTOR(PAL_ITB_MISS_ENTRY)

pal_itb_miss:
	mtpr	t3, pt3			/* Save t3. */
 	mfpr	t3, excAddr		/* Get the va of target reference.*/
	mtpr	t4, pt4			/* Save t4.*/
	mfpr	t4, ptVptPtr		/* Get the virtual page table pointer.*/
	mtpr	t5, pt5			/* Save t5.*/
	sll	t3, 64-VA_S_SIZE, t5	/* Clear out sign extension bits of va.*/
	srl	t5, 64-VA_S_SIZE+VA_S_OFF-3, t5	/* Clear all but 3 offset bits.*/
        blbs    t4, 1f                  /* If physical mode, map va->pa 1-to-1.*/
	or	t5, t4, t5		/* Form virtual address of Level 3 PTE.*/
	bis	zero, zero, t4		/* Clear t4 to indicate I-stream.*/
	mtpr	t6, pt6			/* Save t6.*/
	mfpr	t6, excAddr		/* Save exception address.*/
	ldq_a	t5, 0(t5)		/* Read the Level 3 PTE (may fault).*/
	blbc	t5, pal_pte_invalid	/* Check for invalid PTE.*/
	and	t5, PTE_M_FOE, t4	/* Isolate the Fault On Execute bit.*/
	bne	t4, pal_itb_foe		/* Check for Fault On Execute.*/
	mtpr	t3, tbTag		/* Write the tag.*/
	mtpr	t5, tbCtl		/* Select large or small page size.*/
	mtpr	t6, excAddr		/* Restore the exception address.*/
	mtpr	t5, itbPte		/* Write the PTE into the TB.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	mfpr	t6, pt6
	hw_rei				/* Return and try access again.*/
/*
** Simple 1-to-1 va->pa mapping
*/
        ALIGN_BRANCH_TARGET        

1:	sll	t3, PTE_V_PFN-VA_S_OFF, t4 /* Fabricate PFN.*/
	lda	t5, 0x3371(zero)	/* Fabricate protection bits.*/
	or	t5, t4, t5		/* Form PTE.*/
	mtpr	t3, tbTag		/* Write the tag.	*/
	mtpr	t5, tbCtl		/* Select large or small page size.*/
	mtpr	t3, excAddr		/* Restore the exception address.*/
	mtpr	t5, itbPte		/* Write PTE into the TB and set valid.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	hw_rei				/* Return and try access again.*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The I-stream access violation handler switches mode to kernel,
**	builds a stack frame on the kernel stack, loads the GP with the 
**	KGP, loads the va of the faulting target I-stream reference into 
**	a0, loads access violation fault code into a1, loads the value -1 
**	into a2 (I-fetch), and dispatches to the memory management fault
**	handler pointed to by the entMM operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when an I-stream
**	access violation occurs.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - VA of faulting target reference
**	a1 - MM fault code (ACV)
**	a2 - Reference type (I-fetch)
**
** SIDE EFFECTS:
** 
**	The current mode of the processor may be switched from
**	user to kernel.
**
*/
	HDW_VECTOR(PAL_ITB_ACV_ENTRY)

pal_itb_acv:
	mtpr	t3, pt3			/* Save t3.*/
	mfpr	t3, pt9			/* Get the current processor status.*/
	mtpr	t4, pt4			/* Save t4.*/
	mfpr	t4, excAddr		/* Get the VA of target reference.*/
	bic	t4, 3, t4		/* Clean PC of faulting instruction.*/

	BUILD_STACK_FRAME(t3, t4)

	bis	zero, t4, a0		/* Move VA of faulting instruction to a0*/
	lda	a1, MM_K_ACV(zero)	/* Indicate ACV fault in a1.*/
	subq	zero, 1, a2		/* Indicate I-fetch reference in a2.*/
	mfpr	t4, ptEntMM		/* Get the kernel MM fault handler entry*/
	mfpr	t3, pt3			/* Restore t3.*/
	mtpr	t4, excAddr		/* Establish kernel entry point address.*/
	mfpr	gp, ptKgp		/* Load the kernel global pointer.*/
	mfpr	t4, pt4			/* Restore t4.*/
	hw_rei				/* Transfer to kernel MM fault handler.*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Fetch the PTE of the target virtual address using a virtual 
**	access method.  If the PTE is valid then load it into the DTB 
**	and continue, otherwise, take TNV or ACV exception.
**
**      Physical mode may be enabled by setting VptPtr<0> thus allowing
**      a simple 1-to-1 va -> pa mapping to be used in place of the
**      page table translation.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a D-stream
**	TB miss occurs as a result of a native (non-PAL) mode reference.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	t3 - va of target reference
**	t4 - MM_CSR or scratch
**	t5 - PTE
**	t6 - PC of faulting instruction
** 
** SIDE EFFECTS:
**
**	The virtual access method used to fetch the PTE of the
**	target reference may result in a trap to the PAL mode
**	DTB miss fault handler.
** 
*/ 
	HDW_VECTOR(PAL_NDTB_MISS_ENTRY)

pal_ndtb_miss:
	mtpr	t3, pt3			/* Save t3.*/
	mfpr	t3, va			/* Get the va of target reference.*/
	mtpr	t4, pt4			/* Save t4.*/
	mfpr	t4, ptVptPtr		/* Get the virtual page table pointer.*/
	mtpr	t5, pt5			/* Save t5.*/
	sll	t3, 64-VA_S_SIZE, t5	/* Clear out sign extension bits of va.*/
	srl	t5, 64-VA_S_SIZE+VA_S_OFF-3, t5 /* Clear all but 3 offset bits.*/
        blbs    t4, 1f        		/* If physical mode, map va->pa 1-to-1.*/
	mtpr	t6, pt6			/* Save t6.*/
	or	t5, t4, t5		/* Form virtual address of Level 3 PTE.*/
	mfpr	t4, mmcsr		/* Save MM_CSR in case we fault.*/
	mfpr	t6, excAddr		/* Save the exception address.*/
	ldq_a	t5, 0(t5)		/* Read the Level 3 PTE (may fault).*/
	blbc	t5, pal_pte_invalid	/* Check for TNV or ACV violation.*/
	mtpr	t6, excAddr		/* Restore the exception address.*/
	mtpr	t3, tbTag		/* Set up the tag.*/
	mtpr	t5, dtbCtl		/* Select large or small page size.*/
	mtpr	t5, dtbPte		/* Write PTE into the TB and set valid.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	mfpr	t6, pt6
	hw_rei				/* Return and try access again.*/
/*
** Simple 1-to-1 va->pa mapping
*/
        ALIGN_BRANCH_TARGET

1:	sll	t3, PTE_V_PFN-VA_S_OFF, t4 /* Fabricate PFN.*/
	lda	t5, 0x3371(zero)	/* Fabricate protection bits.*/
	or	t5, t4, t5		/* Form PTE.*/
	mtpr	t3, tbTag		/* Set up the tag.*/
	mtpr	t5, dtbCtl		/* Select large or small page size.*/
	mtpr	t5, dtbPte		/* Write PTE into the TB and set valid.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	hw_rei				/* Return and try access again.*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Fetch the PTE of the target virtual address using a three
**	level page table translation.  If the PTE fetched is valid 
**      then load it into the TB and continue, otherwise, force TNV 
**      or ACV exception.
**
**      Physical mode may be enabled by setting Ptbr<0> thus allowing
**      a simple 1-to-1 va -> pa mapping to be used in place of the
**      page table translation.
**
**	PALcode uses virtual access methods for the following:
**
**	   - Fetching the PTE in a native-mode TB miss handler
**	   - Storing/Loading to/from the kernel stack 
**
**	If the opcode of the faulting instruction was a HW_LD, then we 
**	got here by way of a PTE fetch in a native-mode TB miss handler.  
**	Else we got here because of a HW_ST to the kernel stack during 
**	a stack frame build, or a LDQ from the kernel stack during an 
**	rti or retSys privileged CALL_PAL function.  
**
**	>>>>>>>>>>>>>>>>>>>>>>>>> WARNING! <<<<<<<<<<<<<<<<<<<<<<<<< 
**      This code discriminates between the two cases above solely on 
**      the basis of the faulting instruction's opcode!  Care must be 
**	taken to insure that a virtual HW_LD is not done anywhere else
**      other than in the native TB miss handlers, otherwise, how will 
**      we tell the difference?
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a D-stream 
**	TB miss occurs as a result of a PAL mode reference.
**
** INPUT PARAMETERS:
** 
**	See OUTPUT PARAMETERS below
** 
** OUTPUT PARAMETERS:
**
**	NOTE: the following register usage is CRITICAL, it must be 
**	maintained in both the native ITB and DTB miss handlers:
**
**	t3 - va of original faulting reference.
**	t5 - target register of PTE fetch.
**
** SIDE EFFECTS:
**
** 
*/
	HDW_VECTOR(PAL_PDTB_MISS_ENTRY)

pal_pdtb_miss:
	mtpr	t7, pt23		/* Save t7.*/
	mfpr	t7, va			/* Get the va of faulting reference.*/
	mtpr	t8, pt8			/* Save t8.*/
	mfpr	t8, ptPtbr		/* Get the page table base.*/
	mtpr	t11, pt21		/* Save t11.*/
	blbs	t8, 3f			/* If physical mode, map va->pa 1-to-1.*/
/*
** Fetch Level 1 PTE
**
** The page table base register (PTBR) contains the physical page frame number 
** (PFN) of the base of the highest level (Level 1) page table.  VA<SEG1> bits 
** of the virtual address are used to index into the Level 1 page table to 
** obtain the page table entry (PTE) that maps the physical PFN of the base of 
** the second level (Level 2) page table.  The Level 1 PTE is obtained as 
** follows:
**
**	L1PTE <- ({PTBR * PageSize} + {8 * VA<SEG1>})
**
** NOTE:  The PAL temporary register, ptPtbr, contains the physical PFN of 
** the base of the Level 1 page table shifted into the correct bit position 
** (i.e. ptPtbr = PTBR * PageSize).  See the CALL_PAL swpctx function.
*/
	sll	t7, 64-(3*VA_S_SEG+VA_S_OFF), t11 /* Clear out upper bits of va.*/
	srl	t11, 61-VA_S_SEG, t11	/* Get 8 * VA<SEG1>.*/
	addq	t11, t8, t8		/* Form address of L1PTE.*/
	ldq_p	t8, 0(t8)		/* Read (physical) L1PTE.*/
	blbc	t8, 1f			/* Check if L1PTE not valid.*/
/*
** Fetch Level 2 PTE
**
** The Level 1 PTE contains the physical PFN of the base of the second level 
** (Level 2) page table.  VA<SEG2> bits of the virtual address are used to 
** index into the Level 2 page table to obtain the PTE that maps the physical 
** PFN of the base of the third level (Level 3) page table.  The Level 2 PTE 
** is obtained as follows:
**
**	L2PTE <- ({L1PTE<PFN> * PageSize} + {8 * VA<SEG2>})
*/
	sll	t7, 64-(2*VA_S_SEG+VA_S_OFF), t11 /* Clear out upper bits of va.*/
	srl	t11, 61-VA_S_SEG, t11	/* Get 8 * VA<SEG2>.*/
	srl	t8, PTE_V_PFN, t8	/* Extract Level 1 PFN.*/
	sll	t8, VA_S_OFF, t8	/* L1PTE<PFN> * PageSize.*/
	addq	t11, t8, t8		/* Form address of L2PTE.*/
	ldq_p	t8, 0(t8)		/* Read (physical) L2PTE.*/
	blbc	t8, 2f			/* Check if L2PTE not valid.*/
/*
** Fetch Level 3 PTE
**
** The Level 2 PTE contains the physical PFN of the base of the third level 
** (Level 3) page table.  VA<SEG3> bits of the virtual address are used to 
** index into the Level 3 page table to obtain the PTE that maps the physical 
** PFN of the page being referenced.  The Level 3 PTE is obtained as follows:
**
**	L3PTE <- ({L2PTE<PFN> * PageSize} + {8 * VA<SEG3>})
*/
	sll	t7, 64-(1*VA_S_SEG+VA_S_OFF), t11 /* Clear out upper bits of va.*/
	srl	t11, 61-VA_S_SEG, t11	/* Get 8 * VA<SEG3>.*/
	srl	t8, PTE_V_PFN, t8	/* Extract Level 2 PFN.*/
	sll	t8, VA_S_OFF, t8	/* L2PTE<PFN> * PageSize*/
	addq	t11, t8, t8		/* Form address of L3PTE.*/
	ldq_p	t8, 0(t8)		/* Read (physical) L3PTE.*/
	blbc	t8, 2f			/* Check if L3PTE not valid.*/
/*
** Update the TB
**
** The translation buffer is loaded with the Level 3 PTE that maps the physical
** PFN of the page being referenced.  This PFN is concatenated with the 
** VA<OFFSET> bits (byte-within-page) to obtain the physical address of the 
** location being accessed.
*/
	mtpr	t7, tbTag		/* Load tag and clear TB valid bit.*/
	mtpr	t8, dtbCtl		/* Select large or small page size.*/
	mtpr	t8, dtbPte		/* Write L3PTE into TB and set valid.*/
	mfpr	t7, pt23		/* Restore scratch registers.*/
	mfpr	t8, pt8
	mfpr	t11, pt21
	hw_rei				/* Return and try access again.	*/
/*
** Case 1: An invalid Level 1 PTE was detected.
**
** For Level 1 errors due to a HW_LD in a native-mode TB miss handler, 
** the page tables must be corrupted (which means certain death) so take a 
** machine check exception.  For kernel stack errors, just simply die with
** a kernel-stack-not-valid error.  
*/
	ALIGN_BRANCH_TARGET		/* Align on quadword boundary.*/

1:	mfpr	t11, mmcsr		/* Get MM_CSR.*/
	srl	t11, MMCSR_V_OPC, t11	/* Get opcode of faulting instruction.*/
	xor	t11, 0x1b, t11		/* Is this a HW_LD?*/
	bne	t11, pal_mchk_ksp_not_valid /* If not, must be ksp problem.*/
	mfpr	t5, pt5			/* Restore t5.*/
	br	zero, pal_mchk_pal	/* Page tables are messed up.*/
/*
** Case 2: An invalid Level 2 or Level 3 PTE was detected.
**
** For Level 2 or 3 errors due to a HW_LD in a native-mode TB handler, return
** a 'fake' PTE with all the protection bits either set or cleared depending 
** upon the KRE bit of the Level 2 or 3 PTE.  Upon returning to the native- 
** mode TB miss handler, it will appear as if we successfully fetched an 
** invalid PTE. Because the PTE is not valid, the native TB miss handler will 
** be coerced into doing "the right thing" by sorting out TNV or ACV based on 
** the protection bits of the returned 'fake' PTE.
**
** For kernel stack errors, simply die with a kernel-stack-not-valid error.  
*/
	ALIGN_BRANCH_TARGET		/* Align on quadword boundary*/

2:	mfpr	t11, mmcsr		/* Get MM_CSR.*/
	mfpr	t7, excAddr		/* Get PC of the faulting instruction.*/
	srl	t11, MMCSR_V_OPC, t11	/* Get opcode of faulting instruction.*/
	xor	t11, 0x1b, t11		/* Is this a HW_LD?*/
	bne	t11, pal_mchk_ksp_not_valid /* If not, must be ksp problem.*/
	addq	t7, 4, t7		/* Kick PC past faulting instruction.*/
	mtpr	t7, excAddr		/* Set up for return to native TB miss.*/
	lda	t5, PTE_M_PROT(zero)	/* Make 'fake' PTE with prot. bits set.*/
	srl	t8, PTE_V_KRE, t8	/* Get Level 2/3 PTE<KRE> into lsb.*/
	cmovlbc	t8, zero, t5		/* If KRE clear, force ACV upon return.*/
	mfpr	t7, pt23		/* Restore scratch registers.*/
	mfpr	t8, pt8
	mfpr	t11, pt21
	hw_rei				/* Return back to native TB miss flow.*/
/*
** Simple 1-to-1 va->pa mapping
*/
        ALIGN_BRANCH_TARGET

3:	sll	t7, PTE_V_PFN-VA_S_OFF, t11 /* Fabricate PFN.*/
	lda	t8, 0x3371(zero)	/* Fabricate protection bits.*/
	or	t8, t11, t8		/* Form PTE.*/
	mtpr	t7, tbTag		/* Set up the tag.*/
	mtpr	t8, dtbCtl		/* Select large or small page size.*/
	mtpr	t8, dtbPte		/* Write PTE into the TB and set valid.*/
	mfpr	t7, pt23		/* Restore scratch registers.*/
	mfpr	t8, pt4
	mfpr	t11, pt21
	hw_rei				/* Return and try access again.*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The unaligned data access fault handler switches mode to kernel, 
**	builds a stack frame on the kernel stack, loads the GP with the
**	KGP, loads the faulting va into a0, loads the opcode of the
**	faulting instruction into a1, loads the target register into a2,
**	and dispatches to the unaligned access handler pointed to by the 
**	entUna operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a load/store
**	instruction with unaligned data is executed.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	a0 - VA of faulting reference
**	a1 - Opcode of the faulting instruction
**	a2 - Source or destination register operand
**
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
**
*/
	HDW_VECTOR(PAL_UNALIGN_ENTRY)

pal_unalign:
	mtpr	t0, pt0                 /* save t0		*/
	mfpr	t0, pt9			/* get shadow osf ps*/
	mtpr	t1, pt1                 /* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	mtpr	t4, pt4                 /* save t4*/
	mfpr	t4, mmcsr		/* get the mm_csr*/
	mtpr	t3, pt3                 /* save t3*/
	mfpr	t3, va			/* get the va*/
	blbs	t1, pal_mchk_pal	/* unalign from PAL?*/
	addq	t1, 4, t1		/* point to instruction past unalign*/
	bic	t1, 3, t1		/* clean pc*/

	BUILD_STACK_FRAME(t0,t1)

	bis	t3, zero, a0		/* pass va in a0*/
	srl	t4, MMCSR_V_OPC, a1	/* pass opcode in a1*/
	srl	t4, MMCSR_V_RA, a2 	/* pass ra in a2*/
	mfpr	t0, ptEntUna		/* get va of service routine*/
	and	a2, 0x1f, a2		/* clean ra to five bits*/
	mfpr	t1, pt1                 /* restore t1*/
	mfpr	gp, ptKgp		/* get the kernel gp*/
	mfpr	t4, pt4	                /* restore t4*/
	mtpr	t0, excAddr		/* load va of service routine*/
	mfpr	t0, pt0                 /* restore scratch registers*/
	mfpr	t3, pt3
	hw_rei				/* out to service routine*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The reserved or privileged instruction fault handler switches mode
**	to kernel, builds a stack frame on the kernel stack, loads the GP 
**	with the KGP, loads a value of 4 (OPCDEC) into a0, and dispatches 
**	to the illegal instruction fault handler pointed to by the entIF 
**      operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a reserved or
**	privileged opcode is executed, or when a privileged CALL_PAL is 
**      attempted in user mode. Also branched to from unimplemented 
**      privileged and non-privileged CALL_PAL entry point locations.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	a0 - Fault type code (4)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
**
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from 
**	user to kernel.
**
*/
	HDW_VECTOR(PAL_OPCDEC_ENTRY)

pal_opcdec:
	mtpr	t1, pt1			/* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	mtpr	t0, pt0			/* save t0*/
	mfpr	t0, pt9			/* get shadow copy of ps*/
	blbs	t1, pal_mchk_pal	/* opcDec from PAL?*/
	addq	t1, 4, t1		/* point to instruction past opcDec*/
	bic	t1, 3, t1		/* clean pc*/

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntIF		/* get address of service routine*/
	bis	zero, 4, a0		/* flag as opcDec*/
	mtpr	t0, excAddr		/* set va of service routine*/
	mfpr	t1, pt1			/* restore scratch registers*/
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* get the kernel gp*/
	hw_rei				/* out to instruction fault handler*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The floating point fault handler switches mode to kernel,
**	builds a stack frame on the kernel stack, loads the GP with
**	the KGP, loads the value 3 (FP disabled) or 4 (OPCDEC) into
**	a0, and dispatches to the instruction fault handler pointed
**	to by the entIF operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch when a floating
**	point instruction is executed with FP instructions disabled by
**	way of ICCSR<FPE> clear, FP IEEE round to +/- infinity, or FP
**	IEEE with data type field other than S, T, or Q.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	a0 - Fault type code (3 or 4)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
** 
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/
	HDW_VECTOR (PAL_FEN_ENTRY)

pal_fen:
	mtpr	t1, pt1                 /* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	mtpr	t0, pt0			/* save t0*/
	mfpr	t0, pt2			/* get iccsr for copy of fen*/
	blbs	t1, pal_mchk_pal	/* fen from PAL?*/
	mtpr	t3, pt3			/* save t3*/
	bic	t1, 3, t1		/* clean pc*/
	srl	t0, ICCSR_V_FPE, t3	/* get fen to <0>*/
	addq	t1, 4, t0		/* increment PC*/
	cmovlbs	t3, t0, t1		/* if opcdec, use incremented PC*/
	mfpr	t0, pt9			/* get shadow osf ps*/

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntIF		/* get va of service routine*/
	and	t3, 1, a0		/* clean to just fen*/
	mtpr	t0, excAddr		/* set va of service routine*/
	addq	a0, 3, a0		/* fen if fen=0, or opcDec if fen=1*/
	mfpr	t3, pt3			/* restore scratch registers*/
	mfpr	t1, pt1
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* get the kernel gp*/
	hw_rei				/* out to service routine*/

/*======================================================================*/
/*                OSF/1 PRIVILEGED CALL_PAL ENTRY POINTS                */
/*======================================================================*/
/*
**			Offset
**	Entry Point	 (hex)	Description
**	-----------	------	-------------------------------
**	halt		 2000	Halt the processor
**	cflush		 2040	Cache flush
**	draina		 2080	Drain aborts
**	cserve		 2240	Console service
**	swppal		 2280	Swap PALcode
**	wripir		 2340	Write interprocessor interrupt request
**	rdmces		 2400	Read machine check error summary
**	wrmces		 2440	Write machine check error summary
**	wrfen		 2AC0	Write floating point enable
**	wrvptptr	 2B40	Write virtual page table pointer
**	swpctx		 2C00	Swap process context
**	wrval		 2C40	Write system value
**	rdval		 2C80	Read system value
**	tbi		 2CC0	Translation buffer invalidate
**	wrent		 2D00	Write system entry address
**	swpipl		 2D40	Swap IPL
**	rdps		 2D80	Read processor status
**	wrkgp		 2DC0	Write kernel global pointer
**	wrusp		 2E00	Write user stack pointer
**	rdusp		 2E80	Read user stack pointer
**	whami		 2F00	Who am I
**	retsys		 2F40	Return from system call
**	rti		 2FC0	Return from trap, fault, or interrupt
*/

	.text	1
	. = 0x2000

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The halt processor (halt) function stops normal instruction
**	processing, updates the PCB of the current process, and enters 
**	console mode.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	t0 = Halt action
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_HALT_ENTRY)

pal_halt:
	STALL
	br	zero, pal_halt_cont

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The cache flush (cflush) function flushes an entire physical
**	page, specified by a PFN passed in a0, from any data caches 
**	associated with the current processor.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
**
**	a0 - Page Frame Number of the page to be flushed
** 
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**	
**      Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return
**
*/
	CALL_PAL_PRIV(PAL_CFLUSH_ENTRY)

pal_cflush:
	STALL
	br	zero, sys_cflush	/* All this is platform-specific*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The drain aborts (draina) function stalls instruction 
**	issuing until all prior instructions are guaranteed to 
**	complete without incurring aborts.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the 
**	CALL_PAL instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_DRAINA_ENTRY)

pal_draina:
	STALL				/* Required stall.*/
        hw_rei				/* Back to caller.*/

	CALL_PAL_PRIV(0x0003)

pal_opcdec03:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0004)

pal_opcdec04:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0005)

pal_opcdec05:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0006)

pal_opcdec06:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0007)

pal_opcdec07:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0008)

pal_opcdec08:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The console service (cserve) function implements console and
**	platform specific operations.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
**
**	a0 - function parameter
**	a1 - function parameter
**	a2 - function type
**
** OUTPUT PARAMETERS:
**
**	v0 = function result
**
** SIDE EFFECTS:
**	
*/
	CALL_PAL_PRIV(PAL_CSERVE_ENTRY)

pal_cserve:
	STALL
	br	zero, sys_cserve	/* All this is platform-specific*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The swap PALcode image (swppal) function causes the current
**	(active) PALcode to be replaced by the specified new PALcode
**	image.  This instruction is intended for use by operating
**	systems only during system bootstraps and system restarts
**	and by consoles during transitions to console I/O mode.
**
**	The PAL descriptor contained in a0 is interpreted as either
**	a PALcode variant or the base physical address of the new
**	PALcode image.  If a variant, the PALcode image must have
**	been previously loaded.  No PALcode loading occurs as a
**	result of this instruction.
**
**	NOTE:
**	This implementation of SWPPAL does not support PALcode variants.
**	If a variant is specified in a0, a check is performed to 
**	determine whether the variant is OSF/1 or not and the returned 
**	status is either unknown variant (if not OSF/1) or variant not 
**	loaded.
**
** CALLING SEQUENCE:
**
**	Vectored into by hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - New PAL variant or base physical address
**	a1 - New PC
**	a2 - New PCB
**	a3 - New VptPtr
**
** OUTPUT PARAMETERS:
**
**	v0 - Return status:
**		0 success (PAL was switched)
**		1 unknown PAL variant
**		2 known PAL variant, but PAL not loaded
**
** SIDE EFFECTS:
**
** The following processor state is updated in the RESET entry of the
** new PALcode, resulting from a SWPPAL operation.
**
**	Processor State				At exit from SWPPAL
**	-----------------------------------	------------------------------
**	ASN	Address space number		ASN in PCB passed in a2
**	FEN	Floating point enable		FEN in PCB passed in a2
**	IPL	Interrupt priority level	7
**	MCES	Machine check error summary	8
**	PCBB	Privileged context block	Address of PCB passed in a2
**	PC	Program counter			PC passed in a1
**	PS	Processor status		IPL=7, CM=K
**	PTBR	Page table base register	PFN in PCB passed in a2
**	unique	Processor unique value		unique in PCB passed in a2
**	WHAMI	Who-Am-I			unchanged
**	sysval	System value			unchanged
**	sp	Kernel stack pointer		KSP in PCB passed in a2
**	Other IPRs				UNPREDICTABLE
**	R0					zero
**	GPRs					UNPREDICTABLE, except SP & R0
**	FPRs					UNPREDICTABLE
*/
	CALL_PAL_PRIV(PAL_SWPPAL_ENTRY)

pal_swppal:
	cmpule	a0, 255, v0		/* Check for a PAL variant.*/
	cmoveq	a0, a0, v0		/* If a0=0, then a valid new PAL address.*/
	blbc	v0, pal_swppal_cont	/* Nope, must be new PAL address.*/
	cmpeq	a0, 2, v0		/* Is this variant OSF/1?*/
	addq	v0, 1, v0		/* If not return as unknown, otherwise,*/
	hw_rei				/* return as not loaded.*/

        CALL_PAL_PRIV(0x000B)

pal_opcdec0B:
	STALL
	br	zero, pal_CallPalRsvd

        CALL_PAL_PRIV(0x000C)

pal_opcdec0C:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write interprocessor interrupt request (wripir)
**	function generates an interprocessor interrupt on 
**	the processor number passed in register a0.  The 
**	interrupt request is recorded on the target processor 
**	and is initiated when the proper enabling conditions 
**	are present.
**
**	NOTE: This function is currently unimplemented.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - Target processor number
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_WRIPIR_ENTRY)

pal_wripir:
	STALL				/* Required stall*/
	hw_rei				/* Back to caller*/

	CALL_PAL_PRIV(0x000E)

pal_opcdec0E:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x000F)

pal_opcdec0F:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The read machine check error summary (rdmces) function returns
**	the value of the MCES register in v0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - MCES
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11 are UNPREDICTABLE upon return.
*/

	CALL_PAL_PRIV(PAL_RDMCES_ENTRY)

pal_rdmces:
	mfpr	v0 , ptMces			/* get MCES*/
	srl	v0, MCES_V_MIP, v0		/* get mces to <0>*/
	and	v0, 0x1F, v0			/* clean to just mces bits*/
	hw_rei					/* out to service routine*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write machine check error summary (wrmces) function selectively 
**	clears the machine check in progress bit, the system correctable 
**	error in progress bit, or the processor correctable error in progress 
**	bit.  Also sets or clears the processor or system correctable error 
**	reporting enabled bit in the MCES register
**
** CALLING SEQUENCE:
**
**	Vectored into by hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - MCES input
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRMCES_ENTRY)

pal_wrmces:
	mfpr	t9, ptMces		/* get mces*/
	and	a0, 7, t8		/* isolate <2:0>*/
	bis	t8, 3<<3, t8		/* will clear dsc and dpc*/
	sll	t8, MCES_V_MIP, t8	/* move to position*/
	bic	t9, t8, t9		/* selectively clear <2:0>, clear <4:3>*/
	and	a0, 3<<3, t8		/* isolate dsc and dpc bits*/
	sll	t8, MCES_V_MIP, t8	/* move to position*/
	bis	t9, t8, t9		/* affect dsc and dpc bits*/
	mtpr	t9, ptMces		/* load MCES*/

/*
** Affect CRD interrupt enable state based on MCES<DPC>
*/
	srl	t9, (MCES_V_DPC-ABOX_V_CRD_EN), t8 /* Get DPC into position*/
	mfpr	a0, ptImpure		           /* Get ptr to impure area*/
	and	t8, ABOX_M_CRD_EN, t8              /* Isolate DPC*/
/*
** Get the saved ABOX_CTL value from the impure scratch area
*/
	lda	a0, CNS_Q_BASE(a0)
	ldq_p	t9, CNS_Q_ABOX_CTL(a0) 

	/* cannot add any more instructions or code will overflow */

	br	zero, pal_wrmces_cont

	CALL_PAL_PRIV(0x0012)

pal_opcdec12:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0013)

pal_opcdec13:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0014)

pal_opcdec14:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0015)

pal_opcdec15:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0016)

pal_opcdec16:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0017)

pal_opcdec17:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0018)

pal_opcdec18:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0019)

pal_opcdec19:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001A)

pal_opcdec1A:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001B)

pal_opcdec1B:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001C)

pal_opcdec1C:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001D)

pal_opcdec1D:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001E)

pal_opcdec1E:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x001F)

pal_opcdec1F:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0020)

pal_opcdec20:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0021)

pal_opcdec21:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0022)

pal_opcdec22:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0023)

pal_opcdec23:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0024)

pal_opcdec24:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0025)

pal_opcdec25:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0026)

pal_opcdec26:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0027)

pal_opcdec27:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0028)

pal_opcdec28:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x0029)

pal_opcdec29:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x002A)

pal_opcdec2A:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write floating point enable (wrfen) function writes bit zero
**	of the value passed in a0 to the floating point enable register.
**	The wrfen function also writes the value for FEN to the PCB at
**	offset (PCBB+40).
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - value to be written
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRFEN_ENTRY)

pal_wrfen:
	mfpr	t8, ptPrcb		/* Get PCB pointer*/
	and	a0, 1, a0		/* Clean new FEN value*/
	mfpr	t10, pt2		/* Get ICCSR copy*/
	sll	a0, ICCSR_V_FPE, t9	/* Get FEN into correct position*/
	or	zero, 1, t11		/* Get a 1*/
	stl_p	a0, PCB_Q_FEN(t8)	/* Set new fen bit*/
	sll	t11, ICCSR_V_FPE, t11	/* Place 1 in ICCSR<FPE> position*/
	bic	t10, t11, t10		/* Clear ICCSR<FPE>*/
	bis	t10, t9, t10		/* Or in new FEN value*/
	mtpr	t10, pt2_iccsr		/* Establish new ICCSR and shadow*/
	STALL				/* Stall 4 cycles after fen write*/
	STALL
	STALL
	STALL
	STALL
	br	zero, pal_wrfen_cont

	CALL_PAL_PRIV(0x002C)

pal_opcdec2C:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write virtual page table pointer (wrvptptr) function writes 
**	the pointer value passed in a0 to the virtual page table pointer 
**	register (VPTPTR).
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - new virtual page table pointer value.
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRVPTPTR_ENTRY)

pal_wrvptptr:
	mtpr	a0, ptVptPtr
	hw_rei

	CALL_PAL_PRIV(0x002E)

pal_opcdec2E:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_PRIV(0x002F)

pal_opcdec2F:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The swap process context (swpctx) function saves the current 
**	process data in the current PCB, then switches to the PCB 
**	passed in a0 and loads the new process context.  The old PCB 
**	is returned in v0.
**
** CALLING SEQUENCE:
**
**	Vectored into by hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - New PCB
**
** OUTPUT PARAMETERS:
**
**	v0 - Old PCB
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
**	If in the PCB, PTBR<63> = 1, then put us in 'physical mode'. Mark the mode
**	by setting low bit of ptPTBR and ptVptPtr.
** 
*/
	CALL_PAL_PRIV(PAL_SWPCTX_ENTRY)

pal_swpctx:
	mfpr	v0, ptPrcb		/* Get the old PCB base.*/
	mfpr	t8, ptUsp		/* Get the user stack pointer.*/
	rpcc	t0			/* Read the cycle counter.*/
	stq_p	sp, PCB_Q_KSP(v0)	/* Save the old kernel stack pointer.*/
	stq_p	t8, PCB_Q_USP(v0)	/* Save the old user stack pointer.*/
	srl	t0, 32, t9		/* Get CC<Offset>.*/
	addq	t0, t9, t9		/* Merge with CC<Counter>.*/
	stl_p	t9, PCB_L_PCC(v0)	/* Save new time.*/
	mtpr	a0, ptPrcb		/* Set the new PCB.*/
	ldq_p	sp, PCB_Q_KSP(a0)	/* Get the new kernel stack pointer.*/
	ldq_p	t8, PCB_Q_USP(a0)	/* Get the new user stack pointer.*/
	ldq_p	t9, PCB_Q_PTBR(a0)	/* Get the new page table base.*/
	ldl_p	t10, PCB_L_PCC(a0)	/* Get the new cycle counter.*/
	ldq_p	t11, PCB_Q_FEN(a0)	/* Get the new floating point enable bit*/
	mtpr	t8, ptUsp		/* Save the new user stack pointer.*/
	br	zero, pal_swpctx_cont

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write system value (wrval) function write the value passed in 
**	a0 to a 64-bit system value register.  The combination of wrval
**	with the rdval instruction allows access by the operating system 
**	to a 64-bit per-processor value.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - sysval
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRVAL_ENTRY)

pal_wrval:
	mtpr	a0, ptSysVal		/* Set value*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The read system value (rdval) function returns the sysval in v0,
**	allowing access to a 64-bit per-processor value for use by the
**	operating system.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - sysval
**
** SIDE EFFECTS:
**
**	Registers t0 and t8 .. t11 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_RDVAL_ENTRY)

pal_rdval:
	mfpr	v0, ptSysVal		/* Get per-processor system value*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The translation buffer invalidate (tbi) function invalidates 
**	specified entries in the ITB/DTB when the mapping changes.  The 
**	TB invalidate function removes specific entry types based on a 
**	parameter passed in a0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - TB invalidate type
**	a1 - va for TBISx types
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, a0 and a1 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_TBI_ENTRY)

pal_tbi:
	addq	a0, 2, a0		/* Change tbi type range to 0 - 6.*/
	br	t9, 1f			/* Get base address of code.*/
1:	cmpult	a0, 6, t8		/* Check if tbi type is in range.*/
	blbc	t8, 2f			/* Return if out of range.*/
	sll	a0, 4, a0		/* Form index into jump table.*/
	lda	t9, pal_tbi_table-1b(t9) /* Set base to start of table.*/
	addq	t9, a0, t9		/* Form jump table target address.*/
pvc$osf0$3000:
	jmp	zero, (t9)		/* Go do it!*/

2:	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write system entry (wrent) function determines the specific 
**	system entry point based on a CASE selection of the value passed 
**	in a1.  The virtual address of the specified system entry point 
**	is then set to the value passed in a0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - System entry address
**	a1 - System entry type to select
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	The registers t0, t8 .. t11, a0, and a1 are UNPREDICTABLE
**	upon return.
*/
	CALL_PAL_PRIV(PAL_WRENT_ENTRY)

pal_wrent:

#if defined(KDEBUG) 
	cmpult	a1, 7, t8		/* Check if entry type is in range.*/
#else
	cmpult	a1, 6, t8		/* Check if entry type is in range.*/
#endif /* KDEBUG */

	br	t9, 1f			/* Get base address of code.*/
1:	bic	a0, 3, a0		/* Clean the PC.*/
	blbc	t8, 2f			/* Return if out of range.*/
	lda	t9, pal_wrent_table-1b(t9) /* Set base to start of table.*/
	s8addq	a1, t9, t9		/* Form jump table target address.*/

pvc$osf20$3100:
	jmp	zero, (t9)		/* Go do it!*/

2:	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The swap IPL (swpipl) function returns the current value of the
**	PS<IPL> bits in v0 and sets the IPL to the value passed in a0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - New IPL
**
** OUTPUT PARAMETERS:
**
**	v0 - Old IPL
**
** SIDE EFFECTS:
**
**	The registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
*/
	CALL_PAL_PRIV(PAL_SWPIPL_ENTRY)

pal_swpipl:
	mfpr	v0, ptIntMask		/* get interrupt mask*/
	and	a0, 7, a0		/* get ipl*/
	extbl	v0, a0, v0		/* get mask for this ipl*/
	insbl	v0, 1, v0		/* move into position for hier*/

	cmple   a0, 6, t8		/* are crd's blocked? 0=> blocked*/
	s4addq	t8, 0, t8		/* move into position*/
	bis	v0, t8, v0		/* enable crd's if not high ipl*/

	mtpr	v0, hier		/* set new mask*/
	br	zero, pal_swpipl_cont

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The read processor status (rdps) function returns the PS
**	in v0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - PS
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11 are UNPREDICTABLE upon return.
*/
	CALL_PAL_PRIV(PAL_RDPS_ENTRY)

pal_rdps:
	mfpr	v0, pt9			/* Get ps*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write kernel global pointer (wrkgp) function writes the value
**	passed in a0 to the KGP.  The KGP is used to load the GP on
**	exceptions.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - New KGP
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRKGP_ENTRY)

pal_wrkgp:
	mtpr	a0, ptKgp		/* Set KGP*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write user stack pointer (wrusp) function writes the value 
**	passed in a0 to the user stack pointer.
**
** CALLING SEQUENCE:
**
**	Vectored into by hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - New USP
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11, and a0 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_WRUSP_ENTRY)

pal_wrusp:
	mtpr	a0, ptUsp		/* Set user stack pointer*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write performance monitor (wrperfmon) function implements 
**	a platform and chip dependent function, controlling on-chip 
**	performance counters.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
**
**	a0 - function type
**	a1 - function parameter
**
**	    a0			    a1
**	----------		----------
**	0 disable		don't care
**	1 enable		don't care
**	2 affect mux		exact mux fields in ICCSR write format
**	3 process options	<0> when set, log all processes
**				<1> when set, log selected processes
**
** OUTPUT PARAMETERS:
**
**	v0 - 0 failure
**	     1 success
**
** SIDE EFFECTS:
**	
**	Registers t8 .. t11 are UNPREDICTABLE upon return
** 
*/
	CALL_PAL_PRIV(PAL_WRPERFMON_ENTRY)

pal_wrperfmon:
	STALL
	beq	a0, pal_perf1_handler	/* Request to disable?*/
	cmpeq	a0, 1, v0		/* Check for enable*/
	bne	v0, pal_perf2_handler	/* Request to enable?*/
	cmpeq	a0, 3, v0		/* Check for options*/
	bne	v0, pal_perf3_handler	/* Request to set options?*/
	cmpeq	a0, 2, v0		/* Check for mux ctl*/
	beq	v0, 1f			/* If not mux ctl, unknown request*/
	br	zero, pal_wrperfmon_cont
1:	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The read user stack pointer (rdusp) function returns the user 
**	stack pointer in v0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - User stack pointer
**
** SIDE EFFECTS:
**
**	Registers t0, t8 .. t11 are UNPREDICTABLE upon return.
** 
*/
	CALL_PAL_PRIV(PAL_RDUSP_ENTRY)

pal_rdusp:
	mfpr	v0, ptUsp		/* Get the user stack pointer*/
	hw_rei				/* Back to caller*/

	CALL_PAL_PRIV(0x003B)

pal_opcdec3B:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The Who-Am-I (whami) function returns the processor number for 
**	the current processor in v0.  The processor number is in the
**	range 0 .. maxCPU-1 that can be configured in the system.  
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - current processor number
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_WHAMI_ENTRY)

pal_whami:
	mfpr	v0, ptWhami		/* Get Who-Am-I identifier.*/
	STALL
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The return from system call (retsys) function pops the return 
**	address and the user-mode global pointer from the kernel stack.  
**	It then saves the kernel stack pointer, sets the current mode 
**	to user, sets IPL to zero, and enters the user-mode code at the 
**	return address popped off the stack.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_RETSYS_ENTRY)

pal_retsys:
	mfpr    t8, ptIntMask		/* get int mask*/
	lda	t11, FRM_K_SIZE(sp)	/* pop stack*/
	ldq     t9, FRM_Q_PC(sp)	/* get pc*/
	ldq	gp, FRM_Q_GP(sp)	/* get gp*/
	mtpr	t11, ptKsp		/* save kernel stack*/
	insbl	t8, 1, t8		/* move hier value into position*/
        stl_c   t11, -4(sp)             /* clear lock_flag*/
	bis	zero, 8, t10		/* make value for mode=user*/
	mtpr	t10, pt9_ps             /* set new ps*/
	bis	t8, HIERW_M_CRE, t8	/* enable crd's*/
	mtpr	t8, hier		/* set the hardware*/
	br	zero, pal_retsys_cont

	CALL_PAL_PRIV(0x003E)

pal_opcdec3E:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The return from fault, trap, or interrupt (rti) function pops 
**	the registers (a0..a2, and gp) the PC, and the PS, from the 
**	kernel stack.  If the new mode is user, the kernel stack is 
**	saved and the user stack is restored.
**
**	Set the hier, based on priority, and OR in the CRE constant 
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_PRIV(PAL_RTI_ENTRY)

pal_rti:
	stl_c	a2, -4(sp)		/* clear lock_flag*/
	lda	gp, FRM_K_SIZE(sp)	/* get updated sp*/
	ldq	sp, -6*8(gp)		/* get ps*/
	rc	a1			/* clear intr_flag*/
	mfpr	a0, ptIntMask		/* get interrupt mask*/
	mtpr	gp, ptKsp		/* save in case rti to user*/

	and	sp, PS_M_CM, a1		/* get mode*/
	ldq	a2, -1*8(gp)		/* restore a2*/
	beq	a1, pal_rtiToKern	/* br if rti to kernel*/
/* 
** Is a rti to user, a0=interrupt mask 
*/
	insbl	a0, 1, a0		/* new hier for user mode into position*/

	ldq	sp, -5*8(gp)		/* pc*/
	ldq	a1, -2*8(gp)		/* restore a1*/

	bis	a0, HIERW_M_CRE, a0	/* or in CRE*/
	mtpr	a0, hier		/* set the hier*/
	bic	sp, 3, sp		/* clean return pc*/

	br	zero, pal_rti_cont

/*======================================================================*/
/*               OSF/1 UNPRIVILEGED CALL_PAL ENTRY POINTS               */
/*======================================================================*/
/*
**			Offset
**	Entry Point	 (hex)	Description
**	-----------	------	----------------------------
**	bpt		 3000	Breakpoint trap
**	bugchk		 3040	Bugcheck trap
**	callsys		 30C0	System call
**	imb		 3180	I-stream memory barrier
**	rdunique	 3780	Read unique
**	wrunique	 37C0	Write unique
**	gentrap		 3A80	Generate trap
**
*/
	.text	2
	. = 0x3000

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The breakpoint trap (bpt) function switches to kernel mode, 
**	builds a stack frame on the kernel stack, loads the GP with 
**	the KGP, loads a value of 0 into a0, and dispatches to the 
**	breakpoint code pointed to by the entIF operating system 
**	entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Entry code for bpt (0)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
** 
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/ 
	CALL_PAL_UNPRIV(PAL_BPT_ENTRY)

pal_bpt:
	mtpr	t0, pt0                 /* save t0*/
	mfpr	t0, pt9			/* get shadow osf ps*/
	mtpr	t1, pt1                 /* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	bic	t1, 3, t1		/* clean pc*/
	br	zero, pal_bpt_cont

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The bugcheck trap (bugchk) function switches mode to kernel, 
**	builds a stack frame on the kernel stack, loads the GP with 
**	the KGP, loads a value of 1 into a0, and dispatches to the 
**	bugcheck code pointed to by the entIF operating system entry 
**	point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	a0 - Entry code for bugchk (1)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
**
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/
	CALL_PAL_UNPRIV(PAL_BUGCHK_ENTRY)

pal_bugchk:
	mtpr	t0, pt0                 /* save t0*/
	mfpr	t0, pt9			/* get shadow osf ps*/
	mtpr	t1, pt1                 /* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	bic	t1, 3, t1		/* clean pc*/
	br	zero, pal_bugchk_cont

	CALL_PAL_UNPRIV(0x0082)

pal_opcdec82:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The system call (callsys) function switches to kernel mode,
**	builds a stack frame on the kernel stack, loads GP with KGP, 
**	and dispatches to the system call code pointed to by the 
**	entSys operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	Registers t8 .. t11 are UNPREDICTABLE upon entry.
** 
**	The current mode of the processor may be switched from
**	user to kernel.
**
*/
	CALL_PAL_UNPRIV(PAL_CALLSYS_ENTRY)

pal_callsys:
	mfpr	t8, pt9			/* get ps*/
	and	t8, PS_M_CM, t10	/* get mode*/
	beq	t10, pal_mchk_callsys	/* in kernel mode?*/
	mfpr	t9, excAddr		/* get pc*/
	bic	t9, 3, t9		/* clean pc*/
	br	zero, pal_callsys_cont

	CALL_PAL_UNPRIV(0x0084)

pal_opcdec84:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0085)

pal_opcdec85:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The I-stream memory barrier (imb) function flushes the write
**	buffer and I-cache, making I-stream coherent with D-stream.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_UNPRIV(PAL_IMB_ENTRY)

pal_imb:
	mtpr	zero, flushIc		/* Flush the Icache*/
        mb				/* Flush the write buffer*/
	STALL				/* Required stall*/
	STALL				/* Required stall*/
	STALL				/* Required stall*/
	STALL
	STALL
	STALL
	STALL
	mb
        hw_rei				/* Back to caller.*/

	CALL_PAL_UNPRIV(0x0087)

pal_opcdec87:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0088)

pal_opcdec88:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0089)

pal_opcdec89:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008A)

pal_opcdec8A:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008B)

pal_opcdec8B:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008C)

pal_opcdec8C:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008D)

pal_opcdec8D:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008E)

pal_opcdec8E:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x008F)

pal_opcdec8F:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0090)

pal_opcdec90:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0091)

pal_opcdec91:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0092)

pal_opcdec92:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0093)

pal_opcdec93:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0094)

pal_opcdec94:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0095)

pal_opcdec95:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0096)

pal_opcdec96:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0097)

pal_opcdec97:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0098)

pal_opcdec98:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x0099)

pal_opcdec99:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x009A)

pal_opcdec9A:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x009B)

pal_opcdec9B:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x009C)

pal_opcdec9C:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x009D)

pal_opcdec9D:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The read unique value (rdunique) function returns the process 
**	unique value in v0.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	v0 - unique value
**
** SIDE EFFECTS:
**
**	The returned value in v0 is UNPREDICTABLE if a write unique
**	(wrunique) CALL_PAL function did not previously set the value.
** 
*/
	CALL_PAL_UNPRIV(PAL_RDUNIQUE_ENTRY)

pal_rdunique:
	mfpr	v0, ptPrcb		/* Get the PCB pointer*/
	ldq_p	v0, PCB_Q_UNIQUE(v0)	/* Get the unique value*/
	hw_rei				/* Back to caller*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The write unique value (wrunique) function sets the process unique 
**	register to the value passed in a0.
**
** CALLING SEQUENCE:
**
**	Vectored into by hardware-activated dispatch via the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	a0 - unique value
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	CALL_PAL_UNPRIV(PAL_WRUNIQUE_ENTRY)

pal_wrunique:
	mtpr	t0, pt0			/* Save t0*/
	mfpr	t0, ptPrcb		/* Get the PCB pointer*/
	stq_p	a0, PCB_Q_UNIQUE(t0)	/* Store new value*/
	mfpr	t0, pt0			/* Restore t0*/
	hw_rei				/* Back to caller*/

	CALL_PAL_UNPRIV(0x00A0)

pal_opcdecA0:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A1)

pal_opcdecA1:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A2)

pal_opcdecA2:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A3)

pal_opcdecA3:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A4)

pal_opcdecA4:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A5)

pal_opcdecA5:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A6)

pal_opcdecA6:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A7)

pal_opcdecA7:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A8)

pal_opcdecA8:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00A9)

pal_opcdecA9:
	STALL
	br	zero, pal_CallPalRsvd

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The generate trap (gentrap) function switches to kernel mode, 
**	builds a stack frame on the kernel stack, loads the GP with 
**	KGP, loads a value of 2 into a0, and dispatches to the generate 
**	trap code pointed to by the entIF operating system entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Entry code for gentrap (2)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
** 
** SIDE EFFECTS:
** 
**	The current mode of the processor may be switched from
**	user to kernel.
**
*/ 
	CALL_PAL_UNPRIV(PAL_GENTRAP_ENTRY)

pal_gentrap:
	mtpr	t0, pt0                 /* save t0*/
	mfpr	t0, pt9			/* get shadow osf ps*/
	mtpr	t1, pt1                 /* save t1*/
	mfpr	t1, excAddr		/* get pc*/
	bic	t1, 3, t1		/* clean pc*/
	br	zero, pal_gentrap_cont

	CALL_PAL_UNPRIV(0x00AB)

pal_opcdecAB:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00AC)

pal_opcdecAC:
	STALL
	br	zero, pal_CallPalRsvd

#if defined(KDEBUG)
/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Special breakpoint trap for the kernel debugger.  This breakpoint
**	trap switches to kernel mode, builds a stack frame on the kernel 
**	stack, loads the GP with the KGP, loads a value of 0 into a0, and 
**	dispatches to the breakpoint code pointed to by the entDbg debugger
**	entry point.
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
** 
** OUTPUT PARAMETERS:
**
**	a0 - Entry code for dbgstop (0)
**	a1 - UNPREDICTABLE
**	a2 - UNPREDICTABLE
** 
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel.
** 
*/ 
	CALL_PAL_UNPRIV(PAL_DBGSTOP_ENTRY)

pal_dbgstop:
	mtpr	t0, pt0			/* Save t0*/
	mfpr	t0, pt9			/* Get PS*/
	mtpr	t1, pt1			/* Save t1*/
	mfpr	t1, excAddr		/* Get PC*/
	bic	t1, 3, t1		/* Clean PC*/
	br	zero, pal_dbgstop_cont

#else
	CALL_PAL_UNPRIV(0x00AD)

pal_opcdecAD:
	STALL
	br	zero, pal_CallPalRsvd

#endif /* KDEBUG */

	CALL_PAL_UNPRIV(0x00AE)

pal_opcdecAE:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00AF)

pal_opcdecAF:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B0)

pal_opcdecB0:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B1)

pal_opcdecB1:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B2)

pal_opcdecB2:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B3)

pal_opcdecB3:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B4)

pal_opcdecB4:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B5)

pal_opcdecB5:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B6)

pal_opcdecB6:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B7)

pal_opcdecB7:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B8)

pal_opcdecB8:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00B9)

pal_opcdecB9:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00BA)

pal_opcdecBA:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00BB)

pal_opcdecBB:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00BC)

pal_opcdecBC:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00BD)

pal_opcdecBD:
	STALL
	br	zero, pal_CallPalRsvd

	CALL_PAL_UNPRIV(0x00BE)

pal_opcdecBE:
	STALL
	br	zero, pal_CallPalRsvd

#if defined(NPHALT)
/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The non-privileged halt processor (nphalt) function 
**	stops normal instruction processing, updates the PCB, 
**	and enters console I/O mode. 
**
** CALLING SEQUENCE:
**
**	Vectored into via hardware-activated dispatch by the CALL_PAL
**	instruction.
**
** INPUT PARAMETERS:
** 
**	None
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**	The current mode of the processor may be switched from
**	user to kernel mode.
**
*/
	CALL_PAL_UNPRIV(PAL_NPHALT_ENTRY)

pal_nphalt:
	mtpr	t0, pt0                 /* Save t0.*/
	mfpr	t0, pt9                 /* Get shadow copy of PS.*/
	sll	t0, 63-PS_V_CM, t0      /* Get current mode into lsb.*/
    	bge	t0, 1f                  /* Check for kernel vs. user mode.*/
    	mtpr    sp, ptUsp               /* Save user stack pointer.*/
    	mfpr    sp, ptKsp               /* Load kernel stack pointer.*/
    	mtpr    zero, pt9_ps            /* Change mode to kernel.*/
1:  	mfpr	t0, pt0                 /* Restore t0.*/
	br	zero, pal_halt          /* Dispatch to kernel halt routine.*/
#else
	CALL_PAL_UNPRIV(0x00BF)

pal_opcdecBF:
	STALL
	br	zero, pal_CallPalRsvd

#endif /* NPHALT */


/*======================================================================*/
/*                  HARDWARE VECTOR CONTINUATION AREA                   */
/*======================================================================*/

	.text	3
	. = 0x4000

pal_reset_cont:

        mtpr    t1, pt1                 /* Save t1.*/
	ldl_p	t1, 0x10(t0)		/* Get offset to impure scratch area.*/
	bic	t0, 0x1F, t0		/* Point to start of PALcode.*/
	addq	t1, t0, t1		/* Form base address of impure area.*/
	mtpr	t1, ptImpure		/* Save pointer to impure scratch area.*/
        lda	t1, CNS_Q_BASE(t1)	/* Point to start of save state area.*/
        SAVE_GPRS			/* Save all general purpose registers.*/
	mfpr    r0, excAddr		/* Save internal processor registers.*/
	stq_p   r0, CNS_Q_EXC_ADDR(t1)
	mfpr    r0, ps
	stq_p   r0, CNS_Q_PS(t1)
	mfpr    r0, palBase
	stq_p   r0, CNS_Q_PAL_BASE(t1)
	mfpr    r0, hier
	stq_p   r0, CNS_Q_HIER(t1)
	mfpr    r0, sirr
	stq_p   r0, CNS_Q_SIRR(t1)
	mfpr    r0, iccsr
	stq_p   r0, CNS_Q_ICCSR(t1)

	mfpr	t1, palBase		/* Get the previous PAL base.*/
	mtpr	t1, ptPrevPal		/* Save it so we can return, if needed.*/
	mtpr	zero, altMode		/* Set the ALT_MODE to kernel.*/
	mb				/* Isolate to insure no exceptions*/
	mb				/* occur while writing the PAL base.*/
	mtpr	t0, palBase		/* Establish the new PAL base.*/
	mb
	mb

	mfpr	t0, pt0			/* Restore t0.*/
	mfpr	t1, pt1			/* Restore t1.*/
/*
** Initialize the processor status.
*/
	lda	t7, IPL_K_HIGH(zero)	/* Set to high IPL, mode=kernel.*/
	mtpr	t7, pt9_ps		/* Establish PS and shadow copy.*/
/*
** Disable all interrupts ...
*/
	mtpr	zero, astrr		/* Disable ASTs.*/
	mtpr	zero, aster
	mtpr	zero, sirr		/* Disable software interrupts.*/
	mtpr	zero, sier
	mtpr	zero, hirr		/* Disable hardware interrupts.*/
	mtpr	zero, hier
	mtpr	zero, slClr		/* Clear SLC, PCx, and CRD interrupts.*/
/*
** Disable the reporting of CRD errors ...
*/
	bis	zero, 1, t7		/* Get a "1"*/
	sll	t7, MCES_V_DPC, t7	/* Shift into correct position.*/
	mtpr	t7, ptMces		/* Establish machine check error summary*/

/*
** Initialize the I-Cache Control and Status Register ...
*/
	mfpr	t11, ptImpure		/* Get pointer to the impure scratch area.*/
	lda	t11, CNS_Q_BASE(t11)	/* Point to the save state area.*/
	br	t8, 1f			/* Branch around static data.*/

	.align	3
	.quad	ICCSR_K_INIT
	.quad	ABOX_K_INIT

1:	addq	t8, 4, t8
	bic	t8, 7, t8
	ldq_p	t7, 0(t8)		/* Read the default ICCSR value.*/
	sll	t7, 32, t7		/* Shift it into correct position.*/
	mtpr	t7, pt2_iccsr		/* Establish ICCSR and shadow copy.*/
	ldq_p	t7, 8(t8)		/* Read the default ABOX_CTL value.*/
	mfpr	t8, dcStat		/* Load DC_STAT to find out what cpu.*/
	lda	t0, ABOX_M_DC_16K+ABOX_M_DOUBLE_INVAL(t7) /* Enable 16K cache */
	zap	t0, 0xf0, t0				  /* for 21064A */
	and	t8, 5, t8		/* Mask out all but CHIP_ID[2,0] */
	xor	t8, 1, t8		/* If it's 21064A, it should be 1 */
	cmovne	t8, t7, t0		/* Back out change if 21064 */
	mtpr	t0, aboxCtl		/* Establish ABOX_CTL.*/
	stq_p	t0, CNS_Q_ABOX_CTL(t11) /* Save a shadow copy in the impure area.*/
/*
** Register a3 (r19) contains a signature which identifies that the
** transfer from the POST (Power-on Self Test) code is following a
** standard protocol and that the other input parameter values passed
** in to PALcode may be trusted.  This register also contains a 16-bit
** system revision ID that the POST code may use to communicate system
** revisions that would be significant to system software.
**
** The bit summary for register a3 is as follows:
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	<63:32>	 32	RSVD	Reserved.
**	<31:16>  16	SIG	Protocol signature value.
**	<15:00>	 16	REV	System revision ID.
**
** Read the signature value and determine if PALcode is to follow a 
** standard transfer protocol from the POST code. 
*/
	srl	a3, 16, t7		/* Shift signature into lower word.*/

	LDLI(t6,0xDECB)			/* Load the expected valid signature.*/

	cmpeq	t7, t6, t7		/* Is signature passed in a3 valid?*/
	beq	t7, 2f			/* If invalid, don't trust input params.*/
/*
** Save the processor specific parameters ...
*/
	mtpr	t0, aboxCtl		/* Establish ABOX_CTL.*/
	stq_p	t0, CNS_Q_ABOX_CTL(t11)	/* Save ABOX_CTL.*/

/*
** Logically OR in BIU_CTL<OE> bit so that the output enable pins
** always mean cache read and NEVER chip enable which could cause
** catastrophic electrical problems on a write.
*/
	bis	t1, BIU_M_OE, t1	/* OR in BIU_CTL<OE> bit.*/
	mtpr	t1, biuCtl		/* Establish BIU_CTL.*/
	stq_p	t1, CNS_Q_BIU_CTL(t11)	/* Save BIU_CTL.*/
/*
** Save the firmware specific parameters ...
*/
	stq_p	s6, CNS_Q_SROM_REV(t11)	 /* Save srom revision.*/
	stq_p	a0, CNS_Q_PROC_ID(t11)	 /* Save processor id.*/
	stq_p	a1, CNS_Q_MEM_SIZE(t11)	 /* Save memory size.*/
	stq_p	a2, CNS_Q_CYCLE_CNT(t11) /* Save cycle count.*/
	stq_p	a3, CNS_Q_SIGNATURE(t11) /* Save signature and system revision.*/
	stq_p	a4, CNS_Q_PROC_MASK(t11) /* Save processor mask.*/
	stq_p	a5, CNS_Q_SYSCTX(t11)	 /* Save system context.*/

	bis	r31, r31, a3		 /* Clear the signature register.*/
	mtpr	zero, ptWhami		 /* Clear WHAMI and swap flag.*/
/*
** Initialize the jsr prediction stack ...
*/
pvc$osf60$5100:
2:	bsr	t0, 3f			/* Push PC.*/
	br	zero, 7f		/* Join common.*/
pvc$osf61$5101:
3:	bsr	t1, 4f			/* Push PC.*/
pvc$osf67$5100.1:
	ret	zero, (t0)		/* Pop PC.*/
pvc$osf62$5102:
4:	bsr	t2, 5f			/* Push PC.*/
pvc$osf66$5101.1:
	ret	zero, (t1)		/* Pop PC.*/
pvc$osf63$5103:
5:	bsr	t3, 6f			/* Push PC.*/
pvc$osf65$5102.1:
	ret	zero, (t2)		/* Pop PC.*/
6:	STALL				/* Wait 4 cycles.*/
	STALL
	STALL
	STALL
pvc$osf64$5103.1:
	ret	zero, (t3)		/* Pop PC.*/
/*
** Clear any existing exceptions
*/
7:	mfpr	t0, va			/* Unlock VA.*/
	mfpr	t1, dcAddr		/* Read pseudo-IPR to unlock DC_STAT.*/

	mfpr	t0, biuAddr		/* Clear errors in BIU_STAT IPR by*/
	mfpr	t1, fillAddr		/* reading BIU_ADDR and FILL_ADDR IPRs.*/
	mtpr	zero, bcTag		/* Unlock BC_TAG.*/
/*
** Clear the exception summary register 
*/
	mtpr	zero, excSum
	or	zero, 64, t0		/* Loop 64 times to clear all registers.*/
	STALL				/* Wait before reading.*/
	STALL
	STALL
8:	mfpr	zero, excSum		/* Read and clear register mask.*/
	mfpr	zero, excSum		/* Read and clear register mask.*/
	mfpr	zero, excSum		/* Read and clear register mask.*/
	mfpr	zero, excSum		/* Read and clear register mask.*/
	subq	t0, 4, t0		/* Decrement loop counter.*/
	bne	t0, 8b			/* Loop till zero.*/
/*
**
** Detect entry via CALL_PAL SWPPAL and update the processor state.
** Arguments passed in from SWPPAL:
**
**	a1 = New PC
**	a2 = New PCBB
**	a3 = New VptPtr
*/
	mfpr	t0, ptWhami		/* Get swap flag.*/
	srl	t0, WHAMI_V_SWAP, t1	/* Shift the SWAP flag into position.*/
	zapnot	t0, 0xF, t0		/* Clear swap flag longword.*/
	mtpr	t0, ptWhami		/* Update the chip.*/

	LDLI(t8,WHAMI_K_SWAP)		/* Get swap flag constant.*/

	cmpeq	t1, t8, t1		/* Compare flag against constant.*/
	beq	t1, 9f			/* Branch if not SWPPAL, to come up as normal.*/

/*
** We entered on a SWPPAL, so act accordingly.
*/
	rpcc	t1			/* Read the process cycle counter.*/
	mtpr	a2, ptPrcb		/* Set the new PCBB.*/
	ldq_p	sp, PCB_Q_KSP(a2)	/* Get the new kernel stack pointer.*/
	ldq_p	t8, PCB_Q_USP(a2)	/* Get the new user stack pointer.*/
	ldq_p	t9, PCB_Q_PTBR(a2)	/* Get the new page table base.*/
	ldq_p	t10, PCB_L_PCC(a2)	/* Get the new cycle counter.*/
	ldq_p	t11, PCB_Q_FEN(a2)	/* Get the new floating point enable.*/
	mtpr	t8, ptUsp		/* Set the new USP.	*/

/*
** If in the PCB, PTBR<63> = 1, put us in physical mode by setting the
** low bit of ptPtbr and ptVptPtr.
*/
	srl	t9, 63, t0		/* Check new PTBR<63> for physical mode.*/
	bis	a3, t0, a3		/* If physical mode, mark it so.*/

	sll	t9, VA_S_OFF, t9	/* Convert PFN to PTBR physical address.*/
	bis	t9, t0, t9		/* If physical mode, mark it so.*/
	mtpr	t9, ptPtbr		/* Set the new PTBR.*/

	subl	t10, t1, t1		/* Generate new CC<Offset>.*/
	sll	t1, 32, t1		/* Shift into position.*/
	mtpr	t1, cc			/* Set the new PCC.*/
/*
** Setup ASN, PME, and FEN in ICCSR.
** If either of <45:44> is set, performance monitoring is enabled.
** The two bits are being used as flags also, with <45> as the per
** per process bit, and <44> as the global monitor bit. So we just zap <45>.
*/
	STALL
	srl	t11, 63-4, t0		/* Move PME to position relative to FEN.*/
	mfpr	t2, pt2			/* Get current ICCSR shadow value.*/
	lda	t3, 0x7F9(zero)		/* ICCSR<53:42> bit mask.*/
	sll	t3, ICCSR_V_FPE, t3	/* Shift mask into correct position.*/
	and	t11, 1, t11		/* Isolate FEN.*/
	bic	t2, t3, t2		/* Clear ASN, PME, and FPE bits.*/
	or	t0, t11, t11		/* Merge FEN and PME.*/
	bic	t11, 0x16, t11		/* Clear stray bits.*/
	srl	t10, 32-5, t10		/* Move ASN to low order position.*/
	bic	t10, 0x1F, t10		/* Isolate ASN.*/
	or	t10, t11, t10		/* Merge FEN/PME and ASN bits.*/
	sll	t10, ICCSR_V_FPE, t10	/* Shift into correct position.*/
	or	t2, t10, t2		/* Merge into ICCSR.*/
	mtpr	t2, pt2_iccsr		/* Update ICCSR and shadow.*/

	mtpr	zero, flushIc		/* Flush the I-cache.*/
	STALL				/* Need 9 stalls after flush.*/
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	mtpr	zero, xtbZap		/* Zap the ITB/DTB.*/
	bic	a1, 3, a1		/* Clean the PC.*/
	mtpr	a1, excAddr		/* Set the new PC.*/
	mtpr	a3, ptVptPtr		/* Set the new VPTPTR.*/

pvc$osf70$5020:
	bsr	v0, sys_reset_swppal	/* Do platform specific stuff.*/

	bis	zero, zero, v0		/* Set return status to success.*/
	STALL
	hw_rei				/* Begin execution at new PC.*/
/*
** We entered from the POST (Power-On Self Test) code, so initialize 
** accordingly.  This code section initializes the following reserved 
** PAL temporary registers:
**
**	Process Control Block (PCB) Pointer
**	Kernel Stack Pointer (KSP)
**	System Value (SYSVAL)
**	Page Table Base Register (PTBR)
**	Virtual Page Table Pointer (VPTPTR)	
*/
9:      mtpr    zero, ptPtbr            /* Clear page table base register.*/
	mtpr	zero, ptKsp		/* Clear kernel stack pointer.*/
	mtpr	zero, ptSysVal		/* Clear system value register.*/
/*
** The PCB must be initialized to a location such that a swpctx
** operation will not overwrite anything important.  When changing
** context, the old PCB is written with the current cycle count, ptKsp,
** and ptUsp before being replaced by the new PCB.
*/
	STALL				/* Avoid MT/MF conflict*/
	STALL
	mfpr	t0, ptImpure		/* Get base of impure area.*/
	lda	t0, CNS_Q_SCRATCH(t0)	/* Point to a scratch location.*/
	mtpr	t0, ptPrcb		/* Initialize PCB to scratch location.*/

        lda     t0, 2(zero)             /* Get a '2'*/
        sll     t0, 32, t0              /* Generate upper bits.*/
	mtpr	t0, ptVptPtr		/* Establish virtual page table pointer.*/
/*
** Write all the scalar GPRs at least once to insure that the 'Z' 
** bit gets properly set. 
*/
	or	r0, zero, r0
	or	r1, zero, r1
	or	r2, zero, r2
	or	r3, zero, r3
	or	r4, zero, r4
	or	r5, zero, r5
	or	r6, zero, r6
	or	r7, zero, r7
	or	r8, zero, r8
	or	r9, zero, r9
	or	r10, zero, r10
	or	r11, zero, r11
	or	r12, zero, r12
	or	r13, zero, r13
	or	r14, zero, r14
	or	r15, zero, r15
	or	r16, zero, r16
	or	r17, zero, r17
	or	r18, zero, r18
	or	r19, zero, r19
	or	r20, zero, r20
	or	r21, zero, r21
	or	r22, zero, r22
	or	r23, zero, r23
	or	r24, zero, r24
	or	r25, zero, r25
	or	r26, zero, r26
	or	r27, zero, r27
	or	r28, zero, r28
	or	r29, zero, r29
	or	r30, zero, r30
/*
** Clear and restart the cycle counter
*/
	mtpr	zero, ccCtl		/* Clear CC<31:0>*/
	mtpr	zero, cc		/* Clear CC<63:32>*/
	lda	t2, 0x1(zero)		/* Get a '1'*/
	sll	t2, 32, t2		/* Shift it into position.*/
	mtpr	t2, ccCtl		/* Enable the cycle counter.*/

	mtpr	zero, xtbZap		/* Clear the translation buffers.*/

pvc$osf68$5104:
	bsr	ra, sys_reset		/* Do platform specific initialization.*/
	rc	zero			/* Clear the intr_flag.*/

	mfpr	t1, ptImpure		/* Get the base of impure scratch area.*/
	lda	t1, CNS_Q_BASE(t1)	/* Point to the save state area.*/

	RESTORE_GPRS			/* Restore previous saved GPR state.*/

	mtpr	t0, pt0			/* Save original t0 in pt0.*/
	lda	t0, HLT_K_RESET(zero)	/* Mark reason for entry*/
        br      zero, sys_enter_console /* Transfer to console I/O mode ...*/

/*======================================================================*/
/*                     SYSTEM MACHINE CHECK HANDLERS                    */
/*======================================================================*/

	ALIGN_CACHE_BLOCK

pal_mchk_cont:
	mtpr	t0, pt0			/* Save t0.*/
	mtpr	t1, pt1                 /* Save t1.*/
	mtpr	t3, pt3                 /* Save t3.*/

	mfpr	t0, excAddr		/* Get PC*/
	subq	t0, 2, t0		/* If CALL_PAL need PC-4, otherwise,*/
	bic	t0, 2, t0		/* just clean bottom bits of PC.*/
	mtpr	t0, excAddr		/* Update the chip.*/

	blbc	t0, 1f			/* Check for MCHK from PAL mode.*/
	lda	t0, HLT_K_MCHK_FROM_PAL(zero)	/* If yes, load halt code and*/
	br	zero, sys_enter_console		/* enter console ...*/

	ALIGN_BRANCH_TARGET
1:

/*
** Check if the processor generates/expects ECC or parity on the
** check_h pins.  If parity, a D-stream parity error will not
** cause the D-cache to be invalidated, so flush the D-cache.
*/
	mfpr	t1, ptImpure		/* Get pointer to impure scratch area.*/
	lda	t1, CNS_Q_BASE(t1)	/* Point to the save state area.*/
	ldq_p	t3, CNS_Q_BIU_CTL(t1) 	/* Read BIU_CTL*/
	and	t3, BIU_M_ECC, t0	/* Check if ECC or parity mode?*/
	bne	t0, 2f			/* If ECC, no need to flush the D-cache.*/
/*
** Processor is running in parity mode.  Clear the D-cache enable 
** bit in the ABOX_CTL register to turn off and flush the D-cache,
** leaving machine checks enabled.  Restore the original ABOX_CTL
** value when done.
*/
	lda	t3, ABOX_M_MCHK_EN(zero)
	mtpr	t3, aboxCtl		/* Flush the D-cache.*/
	STALL				/* Required stall ...*/
	STALL
	ldq_p	t3, CNS_Q_ABOX_CTL(t1)
	mtpr	t3, aboxCtl		/* Restore saved ABOX_CTL value.*/
	STALL				/* Required stall ...*/
	STALL
/*
** Sort out the reason for the machine check and load the appropriate
** error code into t3 ... 
*/
2:	lda	t3, MCHK_K_UNKNOWN(zero) /* Assume error is unknown for now. */
	mfpr	t0, biuStat		 /* Read BIU status.*/
/*
** Check for bad tag parity in the tag address during an external 
** cache tag probe.
*/
	and	t0, BIU_STAT_M_BC_TPERR, t1
	cmovne	t1, MCHK_K_TPERR, t3
/*
** Check for bad parity in the tag control RAM during an external
** cache tag probe. 
*/
	and	t0, BIU_STAT_M_BC_TCPERR, t1
	cmovne	t1, MCHK_K_TCPERR, t3
/*
** Check for a hard error during an external cycle.
*/
	and	t0, BIU_STAT_M_BIU_HERR, t1
	cmovne	t1, MCHK_K_HERR, t3

	cmpeq	t3, MCHK_K_UNKNOWN, t1	 /* Is error still unknown?*/
	beq	t1, pal_mchk_logout	 /* If not, go create the logout frame*/
/*
** At this point the only thing left to check for is ECC errors,
** both correctable and uncorrectable, if the processor is running
** in ECC mode.  A check to see if the processor properly supports
** ECC correction (pass 3 or better) is also required.
**
** A check for primary cache parity errors is also done here.
*/
	mfpr	t1, dcStat		/* Check who we are.*/
				        /* (this will reset EV45 error bits)*/
	lda	t3, MCHK_K_ECC_NC(zero)	/* Assume uncorrectable ECC error*/
	blbc	t1, pal_mchk_logout	/* Doesn't support ECC or cache parity*/
/*
** Check for and handle any primary cache parity errors ...
*/
	and	t1, (C_STAT_M_DC_ERR | C_STAT_M_IC_ERR), t3
	beq	t3, 4f			/* Not cache parity, go try ECC ..*/

	lda	t3, BIU_STAT_M_FILL_ECC(zero)
	and	t3, t0, t3 		/* Check for an ECC error.*/
	blbc	t3, 3f    		/* Not ECC, so continue ..*/
	lda	t3, MCHK_K_UNKNOWN(zero)/* If ECC error, give up!*/
	br	zero, pal_mchk_logout	/* Go build the logout frame ...*/
/* 
** Determine whether it's an I-cache or D-cache parity error.
*/
	ALIGN_BRANCH_TARGET

3:	lda	t3, MCHK_K_ICPERR(zero)	/* Assume it's an I-cache parity error.*/
	and	t1, C_STAT_M_IC_ERR, t0 /* Is it really an I-cache error?*/
	cmoveq	t0, MCHK_K_DCPERR, t3	/* If not, must be D-cache parity error.*/
	sll	t1, 32, t1		/* Shift C_STAT into upper longword.*/
	or	t1, t3, t3		/* Put error code into lower longword.*/
	beq	t0, pal_mchk_logout	/* Go log error if not I-cache error*/
/* 
** We have an I-cache parity error. If a MCHK is already in progress, 
** dismiss the error, otherwise, go log the error ...
*/
	mfpr	t0, ptMces
	srl	t0, MCES_V_MIP, t0	/* check MIP bit*/
	blbc	t0, pal_mchk_logout	/* if not busy log it, else dismiss it.*/
	mfpr	t0, pt0			/* Restore scratch registers*/
	mfpr	t1, pt1
	mfpr	t3, pt3
	hw_rei				/* And back we go ...*/

	ALIGN_BRANCH_TARGET

4:	lda	t3, MCHK_K_ECC_NC(zero)	/* Assume uncorrectable once again.*/
/*
** Check if primary cache fill data received from outside the
** CPU contained an ECC error.
*/
	lda	t1, BIU_STAT_M_FILL_ECC(zero)
	and	t1, t0, t1
	beq	t1, sys_mchk 		/* Not ECC, something else going on ...*/
/*
** Check if a primary cache fill operation resulted in either
** a multi-bit ECC error, or in a parity error while FILL_ECC
** or FILL_DPERR was already set.
*/
	lda	t1, BIU_STAT_M_FILL_SEO(zero)
	and	t1, t0, t1
	bne	t1, pal_mchk_logout	/* Unable to correct this error.*/
/*
** Check if corrected read data on a primary cache fill operation.
*/
	lda	t1, BIU_STAT_M_FILL_CRD(zero)
	and	t1, t0, t1
	beq	t1, pal_mchk_logout	/* If not CRD, unable to correct error.*/
/*
** Fall through to here for multiple single-bit errors that corrupted
** more than one quadword of an I-cache fill block. Since we have flushed
** the I-cache and I-stream buffer already, all that remains is to
** scrub the error and log it. However, we have to first check for and 
** deal with any simultaneous arithmetic errors, because arithmetic
** traps will not trigger a second time after returning from exception
** service from the machine check.
*/
	mtpr	t4, pt4			/* Save t4.*/
pvc$osf11$5000:
	bsr	t4, pal_mchk_and_arith	/* Check for and deal with arith error.*/
	mfpr	t4, pt4			/* Restore t4.*/
/*
** Turn the ECC error into an interrupt. Change mode to kernel, build
** a stack frame on the kernel stack, and dispatch to the CRD interrupt
** handler ...
*/
        mfpr    t0, pt9			/* Get shadow copy of PS*/
        mfpr    t1, excAddr             /* Get the PC.*/
        bic     t1, 3, t1               /* Clean lower bits of PC.*/

	BUILD_STACK_FRAME(t0,t1)

        br      zero, pal_crd_handler 	/* Dispatch to CRD handler ...*/


/*======================================================================*/
/*                   PROCESSOR MACHINE CHECK HANDLERS                   */
/*======================================================================*/
/*
** This code provides entry points for various processor machine checks
** detected in other PALcode routines.
**
**	t0 - reason for halt
**	t3 - reason for machine check
*/ 
	ALIGN_BRANCH_TARGET

pal_mchk_pal:				  /* Exception illegal in PAL mode*/
	lda	t3, MCHK_K_BUGCHECK(zero)
	br	zero, pal_mchk_logout
	
	ALIGN_BRANCH_TARGET

pal_mchk_callsys:			  /* Callsys from kernel mode*/
	lda	t3, MCHK_K_OS_BUGCHECK(zero)
	br	zero, pal_mchk_logout

	ALIGN_BRANCH_TARGET

pal_mchk_ksp_not_valid:			  /* kernel stack not valid halt*/
	mfpr	t8, ptPrcb		  /* get process control block base*/
        mfpr	t9, pt9			  /* get shadow copy of ps*/
        and	t9, PS_M_CM, t9		  /* get current mode*/
        beq	t9, 1f			  /* check if already in kernel mode*/
        mtpr	sp, ptUsp		  /* save user stack*/
        stq_p   sp, PCB_Q_USP(t8)	  /* store usp*/
        br      zero, 2f		  /* join common*/
1:	stq_p   sp, PCB_Q_KSP(t8)	  /* store ksp*/
2:	rpcc	t9			  /* get cycle counter*/
        srl	t9, 32, t10		  /* move offset*/
        addl    t9, t10, t10		  /* merge for new time*/
        stl_p   t10, PCB_L_PCC(t8)	  /* save time*/
	lda	t0, HLT_K_KSP_INVAL(zero) /* set the halt code*/
	br	zero, sys_enter_console   /* go enter console ...*/

/*======================================================================*/
/*                      MACHINE CHECK LOGOUT FRAME                      */
/*======================================================================*/

        ALIGN_BRANCH_TARGET

pal_mchk_logout:
/*
** Input:
**
**	t3 - Machine check error code
*/
	mtpr	zero, altMode		/* make sure altmode is KERNEL mode*/
	mfpr	t0, ptMces		/* get MCES bits*/
	srl	t0, MCES_V_MIP, t1	/* shift MCES<MIP> into lsb*/
	blbc	t1, 1f          	/* if no mchk's in progress, continue*/
	lda	t0, HLT_K_DBL_MCHK(zero)/* else load halt code*/
	br	zero, sys_enter_console	/* and enter console ...*/

1:	lda	t1, MCES_M_MIP(zero)    /* get machine-check-in-progress flag*/
	bis	t0, t1, t0		/* set the MCES<MIP> bit*/
	mtpr	t0, ptMces		/* save it in the chip*/
/*
** Get the address of the full logout area and build the logout frame ...
*/
	mfpr	t1, ptImpure		/* get base address of impure area*/
	lda	t1, LAF_Q_BASE(t1)	/* point to the full logout area*/
	lda	t0, LAF_Q_EXC_ADDR(zero)/* cpu offset*/
	stl_p	t0, LAF_L_CPU(t1) 	/* set offset*/
	lda	t0, LAF_Q_SYS_BASE(r31)	/* sys offset*/
	stl_p	t0, LAF_L_SYS(t1) 	/* set offset*/

	lda	t0, LAF_K_SIZE(zero) 	/* frame size*/
	stl_p	t0, LAF_L_FRAME(t1) 	/* set frame size*/

/*
** If we had an I-cache parity error, the error is retryable. 
** Check the error code stored in the lower longword of t3.
*/
	zapnot	t3, 0xF, t0		/* extract error code*/
	cmpeq	t0, MCHK_K_ICPERR, t0	/* was it an I-cache parity error?*/
	sll	t0, 31, t0		/* answer becomes retry flag (yes = 1)*/
	stl_p	t0, LAF_L_FLAG(t1)	/* store retry flag*/
	srl	t3, 32, t0		/* get C_STAT value or zero if not */
				        /* I-cache parity error*/
	stq_p	t0, LAF_Q_DC_STAT(t1)	/* store C_STAT value*/
	zap	t3, 0xF0, t3		/* clear C_STAT info (if any)*/

	lda	t0, MCHK_K_REV(zero)	/* get mchk rev level*/
	sll	t0, 32, t0		/* move into position*/
	bis	t3, t0, t3		/* merge rev level and error code*/
	stq_p	t3, LAF_Q_PT0(t1)	/* set error code (why in PT0?)*/
/* 
** Save PAL temporary registers ...
*/
	mfpr	t0, pt1
	stq_p	t0, LAF_Q_PT1(t1)
	mfpr	t0, pt2
	stq_p	t0, LAF_Q_PT2(t1)
	mfpr	t0, pt3
	stq_p	t0, LAF_Q_PT3(t1)
	mfpr	t0, pt4
	stq_p	t0, LAF_Q_PT4(t1)
	mfpr	t0, pt5
	stq_p	t0, LAF_Q_PT5(t1)
	mfpr	t0, pt6
	stq_p	t0, LAF_Q_PT6(t1)
	mfpr	t0, pt7
	stq_p	t0, LAF_Q_PT7(t1)
	mfpr	t0, pt8
	stq_p	t0, LAF_Q_PT8(t1)
	mfpr	t0, pt9
	stq_p	t0, LAF_Q_PT9(t1)
	mfpr	t0, pt10
	stq_p	t0, LAF_Q_PT10(t1)
	mfpr	t0, pt11
	stq_p	t0, LAF_Q_PT11(t1)
	mfpr	t0, pt12
	stq_p	t0, LAF_Q_PT12(t1)
	mfpr	t0, pt13
	stq_p	t0, LAF_Q_PT13(t1)
	mfpr	t0, pt14
	stq_p	t0, LAF_Q_PT14(t1)
	mfpr	t0, pt15
	stq_p	t0, LAF_Q_PT15(t1)
	mfpr	t0, pt16
	stq_p	t0, LAF_Q_PT16(t1)
	mfpr	t0, pt17
	stq_p	t0, LAF_Q_PT17(t1)
	mfpr	t0, pt18
	stq_p	t0, LAF_Q_PT18(t1)
	mfpr	t0, pt19
	stq_p	t0, LAF_Q_PT19(t1)
	mfpr	t0, pt20
	stq_p	t0, LAF_Q_PT20(t1)
	mfpr	t0, pt21
	stq_p	t0, LAF_Q_PT21(t1)
	mfpr	t0, pt22
	stq_p	t0, LAF_Q_PT22(t1)
	mfpr	t0, pt23
	stq_p	t0, LAF_Q_PT23(t1)
	mfpr	t0, pt24
	stq_p	t0, LAF_Q_PT24(t1)
	mfpr	t0, pt25
	stq_p	t0, LAF_Q_PT25(t1)
	mfpr	t0, pt26
	stq_p	t0, LAF_Q_PT26(t1)
	mfpr	t0, pt27
	stq_p	t0, LAF_Q_PT27(t1)
	mfpr	t0, pt28
	stq_p	t0, LAF_Q_PT28(t1)
	mfpr	t0, pt29
	stq_p	t0, LAF_Q_PT29(t1)
	mfpr	t0, pt30
	stq_p	t0, LAF_Q_PT30(t1)
	mfpr	t0, pt31
	stq_p	t0, LAF_Q_PT31(t1)
/* 
** Check for any pending arithmetic exceptions 
*/
	mtpr	t4, pt4                 /* save t4*/
	mtpr	t1, pt5			/* save frame pointer*/

pvc$osf13$5000:
	bsr	t4, pal_mchk_and_arith	/* go check for and deal with arith*/
	mfpr	t1, pt5			/* restore frame pointer*/
/* 
** Save the rest of the world ...
*/
	mfpr	t0, pt9			/* propagate from arith*/
	stq_p	t0, LAF_Q_PT9(t1)
	mfpr	t0, excAddr
	stq_p	t0, LAF_Q_EXC_ADDR(t1)
	mfpr	t0, palBase
	stq_p	t0, LAF_Q_PAL_BASE(t1)
	mfpr	t0, hier
	stq_p	t0, LAF_Q_HIER(t1)
	mfpr	t0, hirr
	stq_p	t0, LAF_Q_HIRR(t1)
	mfpr	t0, mmcsr
	stq_p	t0, LAF_Q_MM_CSR(t1)

/*
** If C_STAT not previously saved, save it now ...
*/
	ldq_p	t0, LAF_Q_DC_STAT(t1)	/* get DC_STAT value*/
	bne	t0, 1f			/* if not zero, already got C_STAT*/
	mfpr	t0, dcStat		/* get DC_STAT for non-EV45*/
	stq_p	t0, LAF_Q_DC_STAT(t1)   /* store DC_STAT*/
1:	mfpr	t0, dcAddr
	stq_p	t0, LAF_Q_DC_ADDR(t1)
/* 
** Get ABOX_CTL value out of the impure area.
*/
	mfpr	t3, ptImpure
	lda	t3, CNS_Q_BASE(t3)
	ldq_p	t0, CNS_Q_ABOX_CTL(t3)
	stq_p	t0, LAF_Q_ABOX_CTL(t1)

/* 
** Get BIU_CTL value out of the impure area.
*/
	ldq_p	t0, CNS_Q_BIU_CTL(t3)
	stq_p	t0, LAF_Q_BIU_CTL(t1)

        mfpr	t0, biuStat
        stq_p	t0, LAF_Q_BIU_STAT(t1)
        mfpr	t0, biuAddr
        stq_p	t0, LAF_Q_BIU_ADDR(t1)
        mfpr	t0, fillSyndrome
        stq_p	t0, LAF_Q_FILL_SYNDROME(t1)
        mfpr	t0, fillAddr
        stq_p	t0, LAF_Q_FILL_ADDR(t1)
        mfpr	t0, va
        stq_p	t0, LAF_Q_VA(t1)
        ornot   zero, 1<<SL_CLR_V_CRD, t0 /* get acknowledge bit for crd*/
        mtpr	t0, slClr                 /* acknowledge any pending crds*/
/*
** Read and save bcTag ...
*/
        or      zero, zero, t3          /* init counter*/
        or      zero, zero, t4          /* start with no bits set*/
2:	mfpr	t0, bcTag               /* get next bit of bc_tag*/
        sll     t0, t3, t0              /* move current mask over by one*/
        or      t4, t0, t4              /* set this bit in the mask*/
        addq    t3, 1, t3               /* increment counter*/
        cmplt   t3, 64, t0              /* are we done?*/
        blbs    t0, 2b		        /* loop till done*/

        mtpr	zero, bcTag		/* unlock bc_tag*/
        stq_p   t4, LAF_Q_BC_TAG(t1)	/* and store it*/

        br      zero, sys_mchk_logout   /* go log platform specific info ...*/

/*
** Set up the kernel mode trap and dispatch to the handler.
**
**	t1  - physical address of logout frame
**	pt0 - saved t0
**	pt1 - saved t1
**	pt3 - saved t3
**	pt4 - saved t4
*/ 
	ALIGN_BRANCH_TARGET

pal_mchk_kernel_mode_trap:
        mfpr	t0, pt9			/* Get shadow copy of PS.*/
        mfpr	t3, excAddr             /* Get the PC.*/
	bic	t3, 3, t3		/* Clean off lower bits of the PC.*/

	BUILD_STACK_FRAME(t0,t3)

        or      zero, IPL_K_MCHK, t0	/* Get new PS (mode=kernel, ipl=7).*/
        mtpr	t0, pt9_ps              /* Set new PS and shadow.*/
 	mfpr	t0, ptEntInt		/* Get pointer to interrupt handler.*/
	subq	zero, 1, a2		/* Get a '-1'*/
	srl	a2, 42, a2		/* Shift off low bits for kseg address.*/
	sll	a2, 42, a2		/* Shift kseg bits back into position.*/
	lda	a0, INT_K_MCHK(zero)	/* Pass MCHK entry code in a0.*/

	ldl_p	t3, LAF_Q_PT0(t1)	/* Get back MCHK reason.*/
	lda	t4, MCHK_K_SYSTEM(zero)	/* Base value for system machine checks.*/
	subq	t3, t4, t4		/* Code - base value.*/

	lda	a1, SCB_Q_PROCMCHK(zero)/* Pass procmchk SCB vector in a1.*/
	blt	t4, 1f			/* Branch when processor machine check.*/
	lda	a1, SCB_Q_SYSMCHK(zero)	/* Change to system machine check.*/
1:
	mfpr	t4, ptIntMask		/* Get interrupt mask.*/
	bis	a2, t1, a2		/* Pass ptr to logout area as kseg addr.*/
	mtpr	t0, excAddr		/* Load kernel interrupt handler entry.*/
	extbl	t4, IPL_K_MCHK, t4	/* Get the right mask.*/
	insbl	t4, 1, t4		/* Shift it into position for HIER.*/
	mtpr	t4, hier		/* Set the new interrupt enable mask.*/

	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t1, pt1
	mfpr	t0, pt0
	mfpr	t4, pt4
	mfpr	gp, ptKgp		/* Load the kernel global pointer.*/
	hw_rei				/* Dispatch to kernel interrupt handler*/

/*
** Check for an arithmetic exception under machine check.
** Arithmetic traps will not trigger exceptions a second 
** time after returning from exception service for machine 
** check. Set up for arithmetic exception if necessary.
**
**	t0 - available
**	t1 - available
**	t3 - available
**	t4 - return address
*/
	ALIGN_BRANCH_TARGET

pal_mchk_and_arith:
	mfpr	t0, excSum		/* get exc_sum*/
	mtpr	zero, excSum		/* clear error bits*/
	srl	t0, EXC_V_SWC, t0	/* shift data to correct position*/
	bne	t0, 1f			/* skip arith trap unless needed*/
pvc$osf28$5000.1:
	ret	zero, (t4)		/* back we go if no arith under mchk*/

1:	mfpr	t1, pt9			/* get ps*/
	mfpr	t3, excAddr		/* get pc*/
	bic	t3, 3, t3		/* clean pc*/

	BUILD_STACK_FRAME(t1,t3)

	bis	t0, zero, a0		/* shift exception summary into a0*/
	bis	zero, 63, t1		/* get a counter*/
	bis	zero, zero, a1		/* clear register acculmulator*/
	bis	zero, zero, t0		/* fr31 never traps*/
/*
** Loop to read Exception Write Mask through the one-bit window
** in exc_sum.
**
** When we enter, we already set t0 to the MSK bit for F31 (always 0).
** Build the register mask in a1.
*/

2:	mfpr	a2, excSum		/* get next bit*/
	sll	a1, 1, a1		/* move current mask over by one*/
	cmplt	zero, t0, t0		/* set bit<0> if MSK was set*/
	bis	a1, t0, a1		/* 'or' result into a1<0>*/
	sll	a1, 1, a1		/* move current mask over by one*/
	mfpr	t0, excSum		/* get next bit*/
	cmplt	zero, a2, a2		/* set bit<0> if MSK was set*/
	subq	t1, 2, t1		/* dec counter*/
	bis	a1, a2, a1		/* set this bit in mask*/
	bge	t1, 2b			/* loop till done*/

	mfpr	t0, ptEntArith		/* get va for arithmetic exception*/
	mfpr	gp, ptKgp		/* get the kern gp*/
	mtpr	t0, excAddr		/* set up va*/
pvc$osf29$5000.2:
	ret	zero, (t4)		/* now that the arith is recorded*/
					/* go handle the mchk.*/

	ALIGN_CACHE_BLOCK

pal_arith_cont:
/*
** Loop to read Exception Register Write Mask through the one-bit window
** in exc_sum. 
**
** When we enter, we have already set t0 to the MSK bit for F31 (always 0).
** As an optimization, we alternate using t0 and a2 as destination for the
** MSK bit, and doing the mfprpr long before we need the bit.
**
** The exception summary is already in a0, and we will be building the
** register mask in a1
*/
	mfpr	a2, excSum		/* get next bit*/
	sll	a1, 1, a1		/* move current mask over by one*/
	cmplt	zero, t0, t0		/* set bit<0> if MSK was set*/
	bis	a1, t0, a1		/* 'or' result into a1<0>*/
	sll	a1, 1, a1		/* move current mask over by one*/
	mfpr	t0, excSum		/* get next bit*/
	cmplt	zero, a2, a2		/* set bit<0> if MSK was set*/
	subq	t1, 2, t1		/* decrement by 2 the counter*/
	bis	a1, a2, a1		/* 'or' result into a1<0>*/
	bge	t1, pal_arith_cont	/* loop till done*/

	mfpr	t0, ptEntArith		/* get the va of the arith routine*/
	mfpr	t1, pt1			/* restore scratch*/
	mfpr	t3, pt3			/* restore scratch*/
	mfpr	gp, ptKgp		/* set the kernel gp*/
	mtpr	t0, excAddr		/* set the va of the arith routine*/
	mfpr	t0, pt0			/* restore scratch*/
	hw_rei				/* dispatch to kernel arith handler*/

/*======================================================================*/
/*                      HARDWARE INTERRUPT HANDLERS                     */
/*======================================================================*/

/*
** Correctable Read Data Interrupt Handler
*/
	ALIGN_BRANCH_TARGET

pal_crd_handler:

/*
** Sort out reason for CRD interrupt.  The interrupt
** could have been caused by primary cache fill data,
** received from outside the processor, contained an
** ECC error or an external transaction was acknowledged
** with a soft error.
*/
	mfpr	a0, biuStat		    /* Get BIU status.*/
	srl	a0, BIU_STAT_V_FILL_ECC, a0 /* Get ECC bit into lsb.*/
	and	a0, 1, a0		    /* Isolate to single bit.*/

/*
** Check the MCES<PCE> and MCES<SCE> bits.  If either bit is set,
** do not build the logout frame since one must already be in use.
*/
	mfpr	t1, ptMces		/* Get MCES bits.*/
	srl	t1, MCES_V_SCE, t1	/* Shift PCE/SCE into lsb.*/
	and	t1, 3, t1		/* Check if either bit is set.*/
	beq	t1, pal_crd_logout	/* If clear, go build the logout frame.*/

/*
** Fall through here if we do not build the logout frame.
** If the error is a processor correctable ECC error, (not a
** system correctable error), then scrub memory using the
** cache fill address passed in t3.  Register a0 contains
** the ECC error flag.
*/
	STALL
	mfpr	t3, fillAddr		/* Get the cache fill address.*/
/*
** Merge from writing logout frame. Register t3 has been
** restored to the fill address. Register a0 still contains
** the ECC error flag. The logout frame address has been
** saved in pt4.
*/
pal_crd_merge:
	blbc	a0, 1f			/* Skip scrub if SCE.*/
pvc$osf19$5004:
	bsr	t0, sys_scrub_mem	/* Go scrub the error ...*/
	bis	zero, 1, a0		/* Restore ECC error flag.*/

1:	subq	zero, 1, t0		/* Get a '-1'.*/
	lda	t3, (1<<SL_CLR_V_CRD)(zero) /* Get acknowledge bit.*/
	bic	t0, t3, t0		/* Invert for write-zero-to-clear.*/
	mtpr	t0, slClr		/* Acknowledge the CRD interrupt.*/
/*
** If the reporting of correctable errors is disabled, or a 
** correctable error is already in progress, simply dismiss 
** the interrupt as if it were a late passive release.
*/
	mfpr	t0, ptMces		/* Get MCES bits.*/
	lda	t3, MCES_M_DSC(zero)	/* Isolate the MCES<DSC> bit.*/
	srl	t3, a0, t3		/* Shift DSC bit into MCES<DPC> position.*/
	and	t0, t3, t3		/* Check if reporting is enabled.*/
	bne	t3, 2f			/* Reporting disabled, dismiss interrupt.*/
	lda	t3, 3<<MCES_M_SCE(zero)	/* Get mask for PCE/SCE bits.*/
	and	t0, t3, t3		/* Check if error already in progress.*/
	bne	t3, 2f			/* If so, dismiss the interrupt.*/
/*
** Set the MCES<PCE> or MCES<SCE> bit according to whether this
** error is a cache fill ECC error, or external soft error.  The
** ECC error bit is currently still in register a0.
*/
	lda	t3, MCES_M_SCE(zero)	/* Get MCES<SCE> bit mask.*/
	sll	t3, a0, t3		/* If ECC, shift to MCES<PCE> mask.*/
	bis	t0, t3, t0		/* Set the proper bit in the MCES.*/
	mtpr	t0, ptMces		/* Update the chip.*/
/*
** Set up to post the interrupt and dispatch to the kernel. 
*/
	lda	a1, SCB_Q_SYSERR(zero)	/* Get SCB vector for SCE.*/
	sll	a0, 4, a0		/* Get increment to change SCE to PCE.*/
	addq	a1, a0, a1		/* Set the correct SCB vector.*/
	mfpr	t1, pt4			/* Restore base address of logout area.*/
	subq	zero, 1, a2		/* Get a '-1'*/
	srl	a2, 42, a2		/* Shift off low bits for kseg address.*/
	sll	a2, 42, a2		/* Shift kseg bits back into position.*/
	bis	a2, t1, a2		/* Pass ptr to logout area as kseg addr.*/
	lda	a0, INT_K_MCHK(zero)	/* Pass MCHK entry code in a0.*/
	lda	t1, IPL_K_MCHK(zero)	/* Set IPL = 7.*/
	br	zero, pal_interrupt_post

	ALIGN_BRANCH_TARGET

2:      ldq     gp, FRM_Q_GP(sp)        /* Restore gp*/
        ldq     t0, FRM_Q_PS(sp)        /* Restore ps*/
        ldq     a0, FRM_Q_A0(sp)        /* Restore a0*/
        ldq     a1, FRM_Q_A1(sp)        /* Restore a1*/
        ldq     a2, FRM_Q_A2(sp)        /* Restore a2*/
        lda     sp, FRM_K_SIZE(sp)      /* Deallocate the frame*/
        and     t0, PS_M_CM, t1         /* Get the current mode*/
        beq     t1, 3f                  /* Kernel or user mode?*/
        mfpr    sp, ptUsp               /* If user mode, restore sp*/
3:	mtpr	t0, pt9_ps		/* Restore original ps and shadow*/
	STALL
	STALL
        mfpr    t3, pt3                 /* Restore scratch registers*/
        mfpr    t1, pt1
        mfpr    t0, pt0
        hw_rei                          /* Return from interrupt ...*/

/*
** Performance Counter Interrupt Handler.
*/
        ALIGN_BRANCH_TARGET

pal_pc_handler:
        lda     a1, 1(zero)             /* Get a '1'*/
        subq    zero, 1, a0             /* Get a '-1' (for SlClr)*/
        or      zero, SL_CLR_V_PC0, a2  /* Get SL_CLR<PC0> bit*/
        srl     t3, HIRR_V_PC0, t1      /* Get HIRR<PC0> into lsb*/
        and     t1, 1, t1               /* Isolate to single bit*/
        cmovlbc t1, SL_CLR_V_PC1, a2    /* If PC1, get SL_CLR<PC1> bit*/
        sll     a1, a2, a1              /* Shift clearing bit into position*/
        bic     a0, a1, a0              /* Invert for write-zero-to-clear*/
        mtpr    a0, slClr               /* Clear the correct bit*/
        bis     zero, INT_K_PERF, a0    /* Flag as performance counter*/
        lda     a1, 0x650(zero)         /* Load the SCB vector*/
        cmpeq   t1, zero, a2            /* a2 = 0 for PC0, a2 = 1 for PC1*/
        bis     zero, IPL_K_PERF, t1    /* Load new IPL*/
	br	zero, pal_interrupt_post

/*
** Use the IPL passed in from the interrupt handler to set the
** Hardware Interrupt Enable bits accordingly, post the interrupt
** and dispatch to the kernel interrupt handler ...
**
** Register Usage Conventions:
**
**	a0 - Interrupt type
**	a1 - Platform-specific interrupt vector
**	a2 - UNPREDICTABLE
**      t0 - Scratch, saved in pt0
**	t1 - New IPL
**	t3 - Scratch, saved in pt3
**	gp - Scratch, saved on kernel stack
*/
	ALIGN_BRANCH_TARGET

pal_interrupt_post:

        mfpr    t3, ptIntMask           /* Fetch the interrupt mask*/
        extbl   t3, t1, t3              /* Extract the mask bits*/
        insbl   t3, 1, t3               /* Move it to right location for HIER*/

	cmple   t1, 6, t0		/* are crd's blocked? 0=> blocked*/
	s4addq	t0, 0, t0		/* move into position*/
	bis	t3, t0, t3		/* enable crd's if not high ipl*/

	mfpr	t0, ptEntInt		/* Get ptr to the kernel interrupt handler*/
	mtpr	t1, pt9_ps		/* Set new ipl*/
	mtpr	t3, hier		/* Set the new enable mask*/
	mtpr	t0, excAddr		/* Load the va of the kernel handler*/
	mfpr    gp, ptKgp		/* Set new gp*/
	mfpr	t3, pt3			/* Restore scratch registers*/
	mfpr	t1, pt1
	mfpr	t0, pt0			
	hw_rei				/* Dispatch to kernel interrupt handler*/


/*======================================================================*/
/*                    CORRECTABLE ERROR LOGOUT FRAME                    */
/*======================================================================*/

	ALIGN_BRANCH_TARGET

pal_crd_logout:
	mfpr	t1, ptImpure		/* Get base address of impure area.*/
	lda	t1, LAS_Q_BASE(t1)	/* Point to the short logout area.*/

	lda	t0, LAS_Q_BIU_STAT(zero) /* Offset to CPU specific part of frame.*/
	stl_p	t0, LAS_L_CPU(t1)	/* Store CPU offset.*/
	lda	t0, LAS_K_SIZE(zero)	/* Get the frame size.*/
	stl_p	t0, LAS_L_FRAME(t1)	/* Store frame size.*/
	bis	zero, 1, t0		/* Get a '1'.*/
	sll	t0, 31, t0		/* Move into retry flag position.*/
	stl_p	t0, LAS_L_FLAG(t1)	/* Store retry flag.*/
	lda	t3, MCHK_K_REV(zero)	/* Get MCHK revision level.*/
	sll	t3, 32, t3		/* Shift into upper longword.*/
	lda	t0, MCHK_K_ECC_C(t3)	/* Get ECC error code into low longword.*/
	lda	t3, MCHK_K_CACKSOFT(t3)	/* Get soft error code into low longword.*/
	cmovlbc a0, t3, t0		/* Determine ECC or soft error.*/
	stq_p	t0, LAS_Q_MCHK_CODE(t1)	/* Store error code.*/
/*
** Build the CPU specific part of the logout frame ...
*/
	mfpr	t0, biuStat
	stq_p	t0, LAS_Q_BIU_STAT(t1)
	mfpr	t0, biuAddr
	stq_p	t0, LAS_Q_BIU_ADDR(t1)
	mfpr	t0, dcStat
	stq_p	t0, LAS_Q_DC_STAT(t1)
	mfpr	t0, fillSyndrome
	stq_p	t0, LAS_Q_FILL_SYNDROME(t1)
	mfpr	t0, fillAddr
	stq_p	t0, LAS_Q_FILL_ADDR(t1)

	bis	zero, zero, t0		/* init counter*/
	bis	zero, zero, t3		/* start with no bits set*/
1:	mfpr	a1, bcTag		/* get next bit of bc_tag*/
	sll	a1, t0, a1		/* shift bit over*/
	bis	a1, t3, t3		/* set this bit in mask*/
	addq	t0, 1, t0		/* inc counter*/
	cmplt	t0, 64, a1		/* are we done?*/
	blbs	a1, 1b			/* loop till done*/

	mtpr	zero, bcTag		/* unlock bc_tag*/
	stq_p	t3, LAS_Q_BC_TAG(t1)	/* and store it*/

/*
** Build the platform specific part of the logout frame ...
*/
	mtpr	t4, pt4			/* Save t4.*/
pvc$osf17$5105:				/* Ignore PVC computed goto violation.*/
	bsr	t4, sys_crd_logout
	mfpr	t4, pt4			/* Restore t4.*/

/*
** In case we need to scrub, restore t3 to fill address.
*/
	ldq_p	t3, LAS_Q_FILL_ADDR(t1) /* Get the saved cache fill address*/
	mtpr	t1, pt4			/* Save base address of logout area*/
	br	zero, pal_crd_merge
	

/*======================================================================*/
/*                   MEMORY MANAGEMENT FAULT HANDLERS                   */
/*======================================================================*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**   The invalid PTE fault handler switches to kernel mode, builds a stack 
**   frame on the kernel stack, loads the GP with the KGP, loads the VA of 
**   the faulting target reference in a0, sorts out ACV or TNV, loads the 
**   appropriate memory management fault code in a1, loads the reference 
**   type in a2 (D-read, D-write, or I-fetch), and dispatches to the memory
**   fault handler code pointed to by the entMM operating system entry point.
**
** CALLING SEQUENCE:
**
**	Called from the native mode DTB/ITB miss handlers when an invalid 
**	Level 3 PTE is fetched.
**
** INPUT PARAMETERS:
** 
**	t3   VA of faulting reference
**	t4   MM_CSR if D-stream, zero if I-stream
**	t5   PTE
**	t6   PC of faulting instruction
** 
** OUTPUT PARAMETERS:
**
**	a0   VA of faulting reference
**	a1   MM fault code (ACV, TNV, or FOE)
**	a2   Reference type (D-read, D-write, or I-fetch)
** 
*/ 
	ALIGN_BRANCH_TARGET

pal_pte_invalid:
	mtpr	t7, pt7			/* Save t7.*/
	srl	t4, MMCSR_V_OPC, t7	/* Get opcode of faulting instruction.*/
	bic	t6, 3, t6		/* Clean PC of faulting instruction.*/
	cmpeq	t7, 0x18, t7		/* Is this a FETCH[_M] instruction?*/
	blbs	t7, 4f			/* If so, ignore.*/
	mfpr	t7, pt9			/* Get the processor status.*/

	BUILD_STACK_FRAME(t7,t6)

	sll	t7, 63-PS_V_CM, t7	/* Get the current mode into msb.*/
	bge	t7, 1f			/* Check for kernel vs user mode access.*/
	srl	t5, PTE_V_URE-PTE_V_KRE, t5 /* Move the user protection fields.*/
1:	lda	t7, MM_K_ACV(zero)	/* Assume access violation.*/
	srl	t5, PTE_V_KRE, t5	/* Get read enable bit into lsb.*/
	blbc	t4, 2f			/* Check for read vs write reference.*/
	srl	t5, PTE_V_KWE-PTE_V_KRE, t5 /* Get write enable bit into lsb.*/
2:	cmovlbs	t5, MM_K_TNV, t7	/* Resolve TNV or ACV.*/
	and	t4, 1, a2		/* Indicate read or write reference.*/
	bne	t4, 3f			/* Check for I-stream vs D-stream.*/
	subq	zero, 1, a2		/* Indicate I-stream reference.*/
3:	bis	zero, t7, a1		/* Move MM fault code to a1.*/
	bis	zero, t3, a0		/* Move VA of faulting reference to a0.*/
	mfpr	t7, ptEntMM		/* Get kernel MM fault handler entry.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	mfpr	t6, pt6
	mtpr	t7, excAddr		/* Establish kernel entry point address.*/
	mfpr	gp, ptKgp		/* Load the kernel global pointer.*/
	mfpr	t7, pt7			/* Restore t7.*/
	hw_rei				/* Transfer to kernel MM fault handler.*/
/*
** Ignore faults on FETCH[_M]. Make as if the instruction was 
** never executed.
*/
	ALIGN_BRANCH_TARGET

4:	addq	t6, 4, t6		/* Increment past FETCH[_M] instruction.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	mfpr	t7, pt7
	mtpr	t6, excAddr		/* Load new PC address.*/
	mfpr	t6, pt6
	hw_rei				/* Resume execution.*/

/*
**
** FUNCTIONAL DESCRIPTION:
**
**   The I-stream fault-on-execute (FOE) handler switches mode to kernel, 
**   builds a stack frame on the kernel stack, loads the GP with the KGP, 
**   loads the VA of the faulting I-stream reference into a0, sorts out 
**   the reason for the fault (ACV or FOE), loads the appropriate memory
**   management fault code into a1, loads a value of -1 into a2 (I-fetch)
**   and dispatches to the memory management fault handler code pointed
**   to by the entMM operating system entry point.
**
** CALLING SEQUENCE:
**
**	Called from the ITB miss handler when an I-stream fault-on-execute 
**	is detected.
**
** INPUT PARAMETERS:
** 
**	t3   VA of faulting reference
**	t4   Scratch
**	t5   PTE
**	t6   PC of faulting instruction
** 
** OUTPUT PARAMETERS:
**
**	a0   VA of faulting reference
**	a1   MM fault code (ACV or FOE)
**	a2   Reference type (I-fetch)
** 
*/ 
	ALIGN_BRANCH_TARGET

pal_itb_foe:
	mfpr	t4, pt9			/* Get the processor status.*/
	bic	t6, 3, t6		/* Clean PC of faulting instruction.*/

	BUILD_STACK_FRAME(t4,t6)

	sll	t4, 63-PS_V_CM, t4	/* Get the current mode into msb.*/
	bge	t4, 1f			/* Check for kernel vs user mode access.*/
	srl	t5, PTE_V_URE-PTE_V_KRE, t5 /* Move user protection fields.*/
1:	lda	t4, MM_K_ACV(zero)	/* Assume access violation.*/
	srl	t5, PTE_V_KRE, t5	/* Get the read enable bit into lsb.*/
	cmovlbs	t5, MM_K_FOE, t4	/* Resolve MM fault.*/
	subq	zero, 1, a2		/* Indicate I-fetch reference.*/
	bis	zero, t4, a1		/* Move MM fault code to a1.*/
	bis	zero, t3, a0		/* Move VA of faulting instruction to a0*/
	mfpr	t4, ptEntMM		/* Get kernel MM fault handler entry.*/
	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t5, pt5
	mfpr	t6, pt6
	mtpr	t4, excAddr		/* Establish kernel entry point address.*/
	mfpr	gp, ptKgp		/* Load the kernel global pointer.*/
	mfpr	t4, pt4			/* Restore t4.*/
	hw_rei				/* Transfer to kernel MM fault handler.*/

/*======================================================================*/
/*                       CALL_PAL CONTINUATION AREA                     */
/*======================================================================*/

	ALIGN_CACHE_BLOCK

pal_halt_cont:

	mtpr	t0, pt0			/* Save t0.*/
	mfpr	t0, excAddr		/* Get the current PC address.*/
	mtpr	t3, pt3			/* Save t3.*/
	mfpr	t3, ptPrcb		/* Get the process control block.*/
	mtpr	t4, pt4			/* Save t4.*/
	mtpr	t5, pt5			/* Save t5.*/

	stq_p	sp, PCB_Q_KSP(t3)	/* Kernel mode, store KSP.*/
	rpcc	t4			/* Read the cycle counter.*/
	srl	t4, 32, t5		/* Get CC<Offset>.*/
	addl	t4, t5, t5		/* Merge with CC<Counter>.*/
	stl_p	t5, PCB_L_PCC(t3)	/* Store new time.*/

	subq	t0, 4, t0		/* Compute address of previous instr.*/
	mtpr	t0, excAddr		/* Set new PC address.*/
	lda	t0, HLT_K_SW_HALT(zero)	/* Get halt action code.*/

	mfpr	t3, pt3			/* Restore scratch registers.*/
	mfpr	t4, pt4
	mfpr	t5, pt5
	br	zero, sys_enter_console	/* Transition to console.*/

	ALIGN_CACHE_BLOCK

pal_swppal_cont:
	mfpr	t0, ptWhami		/* Get ptWhami.*/
	zapnot	t0, 0xF, t0		/* Clean <63:32>.*/

	LDLI(v0,WHAMI_K_SWAP)		/* Load the swap flag constant.*/

	sll	v0, WHAMI_V_SWAP, v0	/* Shift into position.*/
	bis	t0, v0, t0		/* Set the "I've been switched" flag.*/
	mtpr	t0, ptWhami		/* Update the chip.*/

pvc$osf47$1007:				/* Ignore PVC violation on computed goto*/
	jmp	zero, (a0)		/* Jump to new PAL!*/

	ALIGN_CACHE_BLOCK

pal_wrmces_cont:

	xor	t8, ABOX_M_CRD_EN, t8	/* Flip DPC to reflect CRD setting*/
	bic	t9, ABOX_M_CRD_EN, t9 	/* Clear CRD*/
	bis	t9, t8, t9		/* merge in new CRD enable value*/
	mtpr	t9, aboxCtl		/* update in chip and shadow copy*/
	stq_p	t9, CNS_Q_ABOX_CTL(a0)
	hw_rei				/* back to user*/

	ALIGN_CACHE_BLOCK

pal_wrfen_cont:
	STALL
	STALL
	hw_rei				/* Return*/


	ALIGN_CACHE_BLOCK

pal_swpctx_cont:
	subl	t10, t0, t0		/* Generate new CC<Offset>.*/
	sll	t0, 32, t0		/* Shift into position.*/
	mtpr	t0, cc			/* Set the new PCC.*/

/*
** If in the PCB, PTBR<63> = 1, put us in physical mode by setting the
** low bit of ptPtbr and ptVptPtr.
*/
	mfpr	t10, ptVptPtr		/* Get in case we want physical mode.*/
	mfpr	t8, ptPtbr		/* Get the old page table base.*/
	srl	t9, 63, t0		/* Check new PTBR<63> for physical mode.*/
	bis	t10, t0, t10		/* If physical mode, mark it so.*/
	mtpr	t10, ptVptPtr		/* Write it back.*/
	sll	t9, VA_S_OFF, t9	/* Convert PFN to physical address.*/
	bis	t9, t0, t9		/* If physical mode, mark it so.*/

/*
** Put in new FPE and performance monitoring bits
**
**	ICCSR<42> is the FPE bit
**	ICCSR<45:44> are the performance monitor bits.
**
** If either of <45:44> is set, performance monitoring is enabled.
** The two bits are being used as flags also, with <45> as the per
** process bit, and <44> as the global monitor bit. In the code below,
** we zap and replace the per process bit, but leave the global bit
** alone. The original code used 7f9 as the mask for cleaning
** ICCSR<52:42>, but I believe we should use 7fb, since <43> is reserved.
*/ 
	srl	t11, 63-4, t0		/* Move PME to position relative to FEN.*/
	mfpr	t10, pt2		/* Get the current ICCSR.*/
	lda	a0, 0x7f9(zero)		/* ICCSR cleaning mask, no asn/fpe/pme.*/
	sll	a0, ICCSR_V_FPE, a0	/* Move cleaning mask into position.*/
	and	t11, 1, t11		/* Isolate FEN.*/
	bic	t10, a0, t10		/* Clean out <45> and <42> in ICCSR.*/
	or	t0, t11, t11		/* Merge FEN and PME.*/
	bic	t11, 0x16, t11		/* Clear stray bits.*/
	sll	t11, ICCSR_V_FPE, t11	/* Move into position.*/
	or	t10, t11, t11		/* Merge into ICCSR.*/
	mtpr	t11, pt2_iccsr		/* Update new ICCSR and shadow.*/

	xor	t8, t9, t8		/* old PTBR = new PTBR?*/
	beq	t8, 1f			/* Branch if page table bases equal.*/
	mtpr	zero, xtbAsm		/* Invalidate all TB entries with ASM=0.*/
	mtpr	t9, ptPtbr		/* Save the new page table base.*/
1:	STALL				/* Wait for IPR updates ...*/
	STALL
	STALL
	STALL
	STALL
	STALL
	hw_rei

	ALIGN_CACHE_BLOCK

pal_tbi_table:
	
/*
** tbia - Invalidate all ITB/DTB entries
*/
pvc$osf1$3000.1:
	mtpr	zero, flushIc		/* Flush all I-cache blocks.*/
	NOP
	mtpr	zero, xtbZap		/* Invalidate all ITB/DTB entries.*/
	br	zero, 1f	
/*
** tbiap - Invalidate all ITB/DTB entries with ASM = 0
*/
pvc$osf2$3000.2:
	mtpr	zero, flushIcAsm	/* Flush all I-cache blocks with ASM=0.*/
	NOP
	mtpr	zero, xtbAsm		/* Invalidate ITB/DTB entries with ASM=0*/
	br	zero, 1f
/*
** 0 - Unused table entry
*/
pvc$osf3$3000.3:
	hw_rei				/* Just return.*/
	.long	0			/* Pad out to 16 bytes.*/
	.long	0
	.long	0
/*
** tbisi - Invalidate ITB entry for VA = a1
*/
pvc$osf4$3000.4:
	mtpr	zero, flushIc		/* Flush all I-cache blocks.*/
	NOP
	mtpr	a1, itbIs		/* Invalidate single ITB entry.*/
	br	zero, 1f
/*
** tbisd - Invalidate DTB entry for VA = a1
*/
pvc$osf5$3000.5:
	mtpr	a1, dtbIs		/* Invalidate single DTB entry.*/
	STALL
	hw_rei
	.long	0
/*
** tbis - Invalidate ITB/DTB entries for VA = a1
*/
pvc$osf6$3000.6:
	mtpr	zero, flushIc		/* Flush all I-cache blocks.*/
	mtpr	a1, dtbIs		/* Invalidate single DTB entry.*/
	mtpr	a1, itbIs		/* Invalidate single ITB entry.*/
	br	zero, 1f

	ALIGN_BRANCH_TARGET
1:	STALL				/* Wait for IPR updates ...*/
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	hw_rei

	ALIGN_CACHE_BLOCK

pal_wrent_table:

pvc$osf21$3100.1:
	mtpr	a0, ptEntInt		/* Interrupt entry address */
	br	zero, 1f

pvc$osf22$3100.2:
	mtpr	a0, ptEntArith		/* Arithmetic trap entry address */
	br	zero, 1f

pvc$osf23$3100.3:
	mtpr	a0, ptEntMM		/* Memory-management fault entry address*/
	br	zero, 1f

pvc$osf24$3100.4:
	mtpr	a0, ptEntIF		/* Instruction fault entry address*/
	br	zero, 1f

pvc$osf25$3100.5:
	mtpr	a0, ptEntUna		/* Unaligned fault entry address*/
	br	zero, 1f

pvc$osf26$3100.6:
	mtpr	a0, ptEntSys		/* Call system entry address*/

#if defined(KDEBUG)

	br	zero, 1f

pvc$osf27$3100.7:
	mtpr	a0, ptEntDbg		/* Kernel debugger entry address*/

#endif /* KDEBUG */

1:	hw_rei				/* Back to caller*/


	ALIGN_CACHE_BLOCK

pal_swpipl_cont:

	mfpr	v0, pt9			/* get old ipl (cm = kernel)*/
	mtpr	a0, pt9_ps		/* set new ps*/

	STALL
	STALL
	STALL
	STALL
	STALL

        hw_rei                          /* now go back*/

	ALIGN_CACHE_BLOCK

#define ICCSR_M_LOW   (ICCSR_M_MUX0 | ICCSR_M_PC0 | ICCSR_M_PC1)

pal_wrperfmon_cont:
/*
** MUX Control
*/
	mfpr	t8, pt2			/* Get current iccsr*/
	lda	a0, ICCSR_M_LOW(zero)	/* Start to build iccsr mask*/
	lda	v0, ICCSR_M_MUX1(zero)	/* Get the rest*/
	sll	v0, 32, v0		/* Move to correct spot*/
	or	v0, a0, a0		/* Build cleaning mask*/
	and	a1, a0, a1		/* Clean request mask*/
	bic	t8, a0, t8		/* Remove old data from iccsr*/
	or	t8, a1, t8		/* Insert new stuff into iccsr*/
	mtpr	t8, pt2_iccsr		/* Update iccsr*/
	or	zero, 1, v0		/* Set success*/
pvc$osf51$77:				/* Disable rule checker on the hw_rei*/
	hw_rei				/* Back to user*/
/*
** Request to disable performance monitoring. Note that it doesn't take
** effect until the next interrupt.
*/
	ALIGN_BRANCH_TARGET

#define INT_K_ZAP_PC	0x8181

pal_perf1_handler:
	mfpr	v0, ptIntMask		/* Get current interrupt masks*/
					/* Build mask to zap pc0 and pc1*/
	lda	a0, (INT_K_ZAP_PC & 0xFFFF)(zero)
	ldah	a0, ((INT_K_ZAP_PC + 32768) >> 16)(a0)
	sll	a0, 16, a1		/* Shift for 2 & 3*/
	or	a0, a1, a0		/* Now is mask for 0/1/2/3*/
	sll	a0, 32, a1		/* Shift for 4/5/6/7*/
	zapnot	a1, 0x3F, a1		/* Clear off mask for 7/6*/
	bis	a0, a1, a0		/* Now is mask for 0/1/2/3/4/5, not 6/7*/

	bic	v0, a0, v0		/* Clear out old enables*/
	mtpr	v0, ptIntMask		/* Update the mask*/
	or	zero, 1, v0		/* Set success*/
	hw_rei				/* Back to user*/
/*
** Request to enable performance monitoring. Note that it doesn't take
** effect until the next interrupt.
*/
	ALIGN_BRANCH_TARGET

pal_perf2_handler:
	mfpr	v0, ptIntMask		/* Get current interrupt masks*/
	lda	a0, (INT_K_ZAP_PC & 0xFFFF)(zero)
	ldah	a0, ((INT_K_ZAP_PC + 32768) >> 16)(a0)
	sll	a0, 16, a1		/* Shift for 2 & 3*/
	or	a0, a1, a0		/* Now is mask for 0/1/2/3*/
	sll	a0, 32, a1		/* Shift for 4/5/6/7*/
	zapnot	a1, 0x3F, a1		/* Clear off mask for 7/6*/
	bis	a0, a1, a0		/* Now is mask for 0/1/2/3/4/5, not 6/7*/

	bis	v0, a0, v0		/* Set new enables*/
	mtpr	v0, ptIntMask		/* Update the mask*/
	or	zero, 1, v0		/* Set success*/
	hw_rei				/* Back to user*/
/*
** Process options
*/
	ALIGN_BRANCH_TARGET

pal_perf3_handler:
	mfpr	v0, pt2			/* Get options from iccsr*/
	and	a1, 3, a1		/* Isolate perfmon enable bits*/
	lda	t0, 3(zero)		/* Get bits*/
	sll	t0, ICCSR_V_PME0, t0	/* Shift to position*/
	bic	v0, t0, v0		/* Clear out old pme bits*/
	sll	a1, ICCSR_V_PME0, a1	/* Shift options to position*/
	or	v0, a1, v0		/* Merge in new pme bits*/
	mtpr	v0, pt2_iccsr		/* Set the new options*/
	or	zero, 1, v0		/* Set return status*/
pvc$osf52$77:				/* Disable rule checker on the hw_rei*/
	hw_rei				/* back to user*/

	ALIGN_CACHE_BLOCK

pal_retsys_cont:

	bic	t9, 3, t9		/* clean return pc*/
	mtpr	t9, excAddr		/* set return address*/
	mfpr	sp, ptUsp		/* get users stack*/
        rc      t11                     /* clear inter_flag*/
	hw_rei

	ALIGN_CACHE_BLOCK

pal_rti_cont:

	ldq	a0, -3*8(gp)		/* restore a0*/
	mtpr	sp, excAddr		/* set return address*/
	bis	zero, 8, sp		/* make value for mode=user */
/*
** The following load will not take a tb miss because the load
** of -6*8(sp) and -1*8(sp) both worked, so the entry for -4*8(sp)
** must be in the tb. If it took a pal dtb miss, the return address 
** we already placed in excAddr would get stepped on.
*/
	ldq	gp, -4*8(gp)		/* restore gp*/
	mtpr	sp, pt9_ps		/* set new ps*/
	mfpr	sp, ptUsp
	STALL
	STALL
	STALL
	STALL				/* 5 bubbles after mtpr ps*/
	hw_rei

	ALIGN_BRANCH_TARGET

pal_rtiToKern:
/* 
** Is a rti to kernel, a0=interrupt mask 
*/
	and	sp, 7, sp		/* get only priority*/
	extbl	a0, sp, a0		/* get mask for this ipl*/
	mtpr	sp, pt9_ps		/* set the new ipl*/

	cmple	sp, 6, a1		/* flag for crd's blocked, 0=>blocked*/
pvc$osf45$0:				/* ps mode unchanged, so we are okay*/
pvc$osf31$84:				/* ps mode unchanged, so we are okay*/
pvc$osf50$16:				/* ps mode unchanged, so we are okay*/
	ldq	sp, -5*8(gp)		/* get pc*/
	insbl	a0, 1, a0		/* move new hier mask into position*/
	s4addq  a1, 0, a1		/* move a1 into position*/
	bis	a0, a1, a0		/* enable crd's if not high ipl*/
	mtpr	a0, hier		/* set the enables*/
					/* pvc_violate 84 -- ps mode unchanged*/
	ldq	a1, -2*8(gp)		/* restore a1*/
	ldq	a0, -3*8(gp)		/* restore a0*/
	bic	sp, 3, sp		/* clean pc*/

	mtpr	sp, excAddr		/* set return address*/
	bis	gp, 0, sp		/* restore sp*/
/*
** The following load will not take a tb miss because the load
** of -6*8(sp) and -1*8(sp) both worked so the entry for -4*8(sp)
** must be in the tb. If it took a pal dtb miss, the return address 
** we already placed in excAddr would get stepped on.
*/
	ldq	gp, -4*8(gp)		/* restore gp*/
	hw_rei

	ALIGN_CACHE_BLOCK

pal_bpt_cont:

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntIF		/* get entry point*/
	bis	zero, zero, a0		/* flag as bpt*/
	mtpr	t0, excAddr		/* load va of entry point*/
	mfpr	t1, pt1			/* restore scratch*/
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* get the kernel gp*/
	hw_rei				/* out to service routine*/

	ALIGN_BRANCH_TARGET

pal_bugchk_cont:

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntIF		/* get entry point*/
	bis	zero, 1, a0		/* flag as bugchk*/
	mtpr	t0, excAddr		/* load va of entry point*/
	mfpr	t1, pt1			/* restore scratch*/
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* get the kernel gp*/
	hw_rei				/* out to service routine*/

	ALIGN_CACHE_BLOCK

pal_callsys_cont:

	BUILD_STACK_FRAME(t8,t9)

	mfpr	t10, ptEntSys		/* get entry point*/
	mfpr	gp, ptKgp		/* get the kernel gp*/
	mtpr	t10, excAddr		/* load va of entry point*/
	STALL
	hw_rei				/* out to service routine*/

	ALIGN_CACHE_BLOCK

pal_gentrap_cont:

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntIF		/* get entry point*/
	bis	zero, 2, a0		/* flag as gentrap*/
	mtpr	t0, excAddr		/* load va of entry point*/
	mfpr	t1, pt1			/* restore scratch*/
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* get the kernel gp*/
	hw_rei				/* out to service routine*/

#if defined(KDEBUG)

	ALIGN_CACHE_BLOCK

pal_dbgstop_cont:

	BUILD_STACK_FRAME(t0,t1)

	mfpr	t0, ptEntDbg		/* Get debugger entry point*/
	bis	zero, zero, a0		/* Flag as CALL_PAL dbgstop*/
	mtpr	t0, excAddr		/* Load VA of entry point*/
	mfpr	t1, pt1			/* Restore scratch registers*/
	mfpr	t0, pt0
	mfpr	gp, ptKgp		/* Get the kernel GP*/
	hw_rei				/* Transfer to debugger*/

#endif /* KDEBUG */

	ALIGN_BRANCH_TARGET

pal_CallPalRsvd:
	mtpr	t1, pt1			/* Save t1.*/
	mfpr	t1, excAddr		/* Get address of faulting instruction.*/
	subq	t1, 4, t1		/* Backup PC.*/
	mtpr	t1, excAddr		/* Load PC of previous instruction.*/
	mfpr	t1, pt1			/* Restore t1.*/
	br	zero, pal_opcdec	/* Go handle reserved opcode fault ...*/

	ALIGN_PAGE

pal_osfpal_end:
