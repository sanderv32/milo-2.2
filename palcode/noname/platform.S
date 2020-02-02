/*
 *      VID: [T1.2] PT: [Fri Apr 21 14:23:30 1995] SF: [platform.s]
 *       TI: [/sae_users/fdh/bin/vice -iplatform.s -l// -p// -DDC21066 -DEB66 -DPASS1 -h -m -aeb66_p1 ]
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
** 	platform.s
** 
**  MODULE DESCRIPTION:
** 
**	Platform specific OSF/1 PALcode for the DECchip 21064/21066
** 
**  AUTHOR: ER 
** 
**  CREATION-DATE: 21-Feb-1994
**
**  MODIFICATION HISTORY: 
**
**  $Log: platform.s,v $
** Revision 1.1  1995/07/26  17:24:54  paradis
** Now either builds PAL from source or unpacks a pre-built PAL based on
** configuration option
**
**  Revision 2.15  1995/02/27  18:14:06  samberg
**  In comments, don't make a0-a2 available to sys_crd_logout
**
**  Revision 2.14  1995/02/01  16:09:35  samberg
**  In sys_enter_console, save t0 (reason) into pt3, with original t0 in pt0
**
**  Revision 2.13  1995/01/25  18:03:56  samberg
**  Make DC21071-DA interrupts become SYSMCHK machine check.
**
**  Revision 2.12  1994/10/21  14:55:14  ericr
**  Add saved SROM Revision ID and Processor ID to values passed
**  by the jtopal CSERVE function
**
**  Revision 2.11  1994/08/29  17:03:18  samberg
**  Eliminate SIO CFIG init, someone else already worries about it
**  The coniditonal reverse was incorrect anyway
**
**  Revision 2.10  1994/08/10  17:07:22  samberg
**  Handle SIO NMI with MCHK_K_SIO... codes
**
**  Revision 2.9  1994/08/08  14:32:07  samberg
**  Typo.
**
**  Revision 2.8  1994/08/08  14:22:28  samberg
**  Add timeout on com line buffer flush.
**
**  Revision 2.7  1994/06/30  15:15:08  samberg
**  Write ptIntMask on swppal.
**
**  Revision 2.6  1994/06/16  15:29:12  samberg
**  Include cserve.h
**
**  Revision 2.5  1994/06/16  14:51:05  samberg
**  For ANSI, changed $ to _, except for pvc labels
**
**  Revision 2.4  1994/06/10  17:44:25  samberg
**  For pass2 eb64p and eb66, the irq0 is an 'or' of
**  SIO, PCI, SCSI, and TULIP. So let the system software sort it out
**
**  Revision 2.3  1994/06/02  17:51:13  samberg
**  On PIC IR7 interrupts, differentiate between passive and real
**
**  Revision 2.2  1994/05/12  17:35:06  ericr
**  Added code to remap RTC registers on EB64.
**
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.17  1994/03/30  16:36:40  ericr
**  Use LDLI macro in place of GET_ADDR macro calls
**
**  Revision 1.16  1994/03/24  21:23:38  ericr
**  Added support for pass 2 EB64+ and EB66 interrupt handling
**
**  Revision 1.15  1994/03/18  19:41:54  samberg
**  Combined the cflush flows
**
**  Revision 1.14  1994/03/18  15:15:55  samberg
**  In SIO_IADCON initialization, init with SIO_IADCON_M_INIT
**  instead of SIO_IADCON
**
**  Revision 1.13  1994/03/18  13:24:08  jeffw
**  replave last usages of IN() and OUT() with *PortByte*()
**
**  Revision 1.12  1994/03/17  21:17:03  jeffw
**  Second pass of scrub_mem cleanup
**  second round of sys cleanup
**
**  Revision 1.11  1994/03/16  21:51:08  samberg
**  fix jtopal w/ srom signature
**  start macro cleanup (OutPortByteReg...)
**
**  Revision 1.10  1994/03/14  20:52:18  ericr
**  Removed indexing to impure saved state area in enter_console.
**
**  Revision 1.9  1994/03/14  16:39:36  samberg
**  Use LOAD_REGION_BASE macro instead of individual load_x_csr macros
**
**  Revision 1.8  1994/03/14  15:17:01  ericr
**  Cleaned up CSERVE routine.
**  Fixed impure area offset bug in CSERVE function jtopal
**
**  Revision 1.7  1994/03/12  01:01:33  ericr
**  Changed line comment character to //
**  Added RCS Id string
**
**  Revision 1.6  1994/03/09  22:57:55  ericr
**  Fixed numerous bugs related to ABOX_CTL and offsets into
**  the save state region of the impure scratch area
**
**  Revision 1.5  1994/03/08  00:16:09  ericr
**  Reworked enter_console to support debug monitor entry.
**
**  Revision 1.4  1994/03/07  16:05:58  jeffw
**  first pass of scrub_mem cleanup.
**
**  Revision 1.3  1994/03/02  21:55:37  ericr
**  Reworked hardware interrupt handling code
**
**  Revision 1.2  1994/02/28  22:13:48  jeffw
**  Fix IOC/12-bit offset error in sys_reset
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
**
*/
#if 0
        .data
        .asciz "$Id: platform.s,v 1.1 1995/07/26 17:24:54 paradis Exp $"
#endif

#include	"dc21064.h"	/* DECchip 21064 specific definitions 	*/
#include	"osf.h"		/* OSF/1 specific definitions 		*/
#include	"macros.h"	/* Common macro definitions		*/
#include	"platform.h"	/* Platform specific definitions	*/
#include	"cserve.h"	/* Platform specific cserve definitions	*/
#include	"impure.h"	/* Impure area data structure defs      */

	.set noat

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
/*                    PLATFORM SPECIFIC ENTRY POINTS                    */
/*======================================================================*/

	.text	3

	ALIGN_CACHE_BLOCK

sys_mchk:
/*
** Register Usage Conventions:
**
**      t0 - Available scratch, saved in pt0
**      t1 - Available scratch, saved in pt1
**      t3 - Machine Check error code
*/
	lda	t3, MCHK_K_UNKNOWN(zero) // Assume error is unknown. 
/*
** Code to sort out platform specific errors goes here ...
*/
        br      zero, pal_mchk_logout    // Go build the logout frame ...

        ALIGN_CACHE_BLOCK

sys_mchk_logout:
/*
** Register Usage Conventions:
**
**      t0 - Available scratch, saved in pt0.
**      t1 - Base address of logout frame
**      t3 - Available scratch, saved in pt3.
**      t4 - Available scratch, saved in pt4.
*/

/*
** Code to write platform specific information into the logout
** frame goes here ... 
*/
	br	zero, pal_mchk_kernel_mode_trap


	ALIGN_CACHE_BLOCK

sys_crd_logout:
/*
** Register Usage Conventions:
**
**      t0 - Available scratch, saved in pt0.
**      t1 - Base address of logout frame
**      t3 - Available scratch, saved in pt3.
**	t4 - Return address
*/

/*
** Code to write platform specific information into the logout
** frame goes here ... 
*/
	subq	zero, 1, t0		// -1 = no platform-specific information
	stl_p	t0, LAS_L_SYS(t1) 	// Store system specific offset
pvc$osf18$5105.1:
	ret	zero, (t4)		// Back to caller ...


	ALIGN_CACHE_BLOCK

sys_interrupt:

/*
** EB66 - DECchip 21066 Evaluation Board Hardware Interrupt Handler
**
**	The order for parsing interrupts is IRQ2, IRQ0, and IRQ1. 
**	IRQ3 and IRQ4 are used to signal interrupts from the
**	I/O and Memory Controllers.
**
**	Register a0 (r16) gets the interrupt type, and a1 (r17) gets
**	a platform specific interrupt vector.  Interrupts are dispatched
**	through a single operating system entry point.  This is true 
**	in the handling of all interrupts.  The list below describes 
**	exactly what is done on the specified interrupt request.
**
**	At this point in the interrupt handling, the stack frame is
**	built with a0 (r16), a1 (r17), a2 (r18), gp (r29), ps, and
**	pc saved on the kernel stack. Registers t0 (r1), t1 (r2), 
**	and t3 (r4) contain information for the parsing chain.
**
** INPUT PARAMETERS:
**
**	t0 - Scratch, saved in pt0.
**	t1 - Scratch, saved in pt1.
**	t3 - HIRR masked with HIER.
**
**	a0 (r16), a1 (r17), a2 (r18) and gp (r29) are saved on the 
**	kernel stack and are also available for use.
**
** OUTPUT PARAMETERS:
**
**	a0 - Interrupt type
**	a1 - Platform specific interrupt vector
**	a2 - UNPREDICTABLE
**	t1 - New IPL
**
** FUNCTION DESCRIPTION:
**
**	Parse the HIRR value passed in t3 to handle the highest
**	priority pending interrupt.
**
**	IRQ0 - 82378IB SIO Non-Maskable Interrupts (IPL 7)
**
**                1. Build a machine check log.
**                2. Set a1 to ^x660 and a0 to ? 
**
**	IRQ2 - DS1287A Real Time Clock Interrupt (IPL 5)
**
**		 1. Dismiss interrupt.
**		 2. Set a1 to ^x600 and a0 to 1 (clock type)
**
**	IRQ1 - 82378IB SIO Device Interrupts (IPL 3)
**
**		 1. iAck the 82378IB PIC to detect the source.
**		 2. Dismiss specific interrupt.
**		 3. Set a1 to x^800 and a0 to 3 (device type)
**
**	       This interrupt supports all devices on ISA.
**
**		IR0	Interval timer
**		IR1	Keyboard
**		IR2	(chains interrupt from slave PIC)
**		IR3	8-bit ISA - National 87312 COM2
**		IR4	8-bit ISA - National 87312 COM1
**		IR5	8-bit ISA - National 87312 Parallel port (or IR7)
**		IR6	8-bit ISA - National 87312 floppy disk controller
**		IR7	8-bit ISA - National 87312 Parallel port (or IR5)
**		IR8	Unused (RTC internal to the SIO)
**		IR9	8-bit  ISA /PCI Ethernet/PCI SCSI
**		IR10	16-bit ISA /PCI Ethernet/PCI SCSI
**		IR11	16-bit ISA /PCI Ethernet/PCI SCSI
**		IR12	16-bit ISA Mouse 
**		IR13	Unused 
**		IR14	16-bit ISA
**		IR15	16-bit ISA /PCI Ethernet/PCI SCSI
*/
	srl	t3, HIRR_V_IRQ2, t1
	blbs	t1, sys_irq2_handler

	srl	t3, HIRR_V_IRQ0, t1
	blbs	t1, sys_irq0_handler

	srl	t3, HIRR_V_IRQ1, t1
	blbs	t1, sys_irq1_handler

	br	zero, sys_dismiss_interrupt

/*          
** 82378IB SIO Non-Maskable Interrupt Handler
**
** IRQ0 handler code - IPL 7
**
*/
	ALIGN_BRANCH_TARGET

sys_irq0_handler:

	InPortByte(SIO_NMISC, gp)	 // Get the NMI Status/Control bits

        srl     gp, SIO_NMISC_V_SERR, a1 // Get SERR status bit into lsb.
        blbc    a1, 1f			 // Not SERR, go check for IOCHK.

        srl     gp, SIO_NMISC_V_SERR_EN, a1 // Get SERR enable bit into lsb.
        blbs    a1, 1f  		    // SERR disabled, go try IOCHK.

        bis     gp, SIO_NMISC_M_SERR_EN, a1 // Set the SERR enable bit.
        and     a1, 0x0F, a1           	    // Clear status bits for write.

	OutPortByteReg(SIO_NMISC, a1, a2, a1) // Clear source of interrupt.

        bis     zero, gp, a1           	 // Copy back original NMISC
        and     a1, 0x0F, a1         	 // Clear status bits for write.
        sll     a1, 8, a1            	 // Shift into proper byte lane.
        stl_p   a1, 0(a2)              	 // Write NMISC contents.
	lda	a2, MCHK_K_SIO_SERR(zero)
					 // Load reason for MCHK.
        br      zero, 2f

	ALIGN_BRANCH_TARGET

1:      srl     gp, SIO_NMISC_V_IOCHK, a1    // Get IOCHK status bit into lsb
        blbc    a1, sys_dismiss_interrupt    // Not IOCHK, dismiss interrupt.

        srl     gp, SIO_NMISC_V_IOCHK_EN, a1 // Get IOCHK enable bit into lsb
        blbs    a1, sys_dismiss_interrupt    // IOCHK disabled, 

        bis     gp, SIO_NMISC_M_IOCHK_EN, a1 // Set the IOCHK enable bit.
        and     a1, 0x0F, a1         	     // Clear status bits for write.

	OutPortByteReg(SIO_NMISC, a1, a2, a1) // Clear source of interrupt.

        bis     zero, gp, a1           	 // Copy back original NMISC.
        and     a1, 0x0F, a1         	 // Clear status bits for write.
        stl_p   a1, 0(a2)              	 // Write NMISC contents.
	lda	a2, MCHK_K_SIO_IOCHK(zero)
					 // Load reason for MCHK.
/*
** Unwind the stack and dispatch to common MCHK logout frame builder ...
*/
2:	bis	zero, a2, t3		// Put error in r3 for frame builder

	ldq	gp, FRM_Q_GP(sp) 	// Restore gp
	ldq	t0, FRM_Q_PS(sp) 	// Restore ps
	ldq	a0, FRM_Q_A0(sp) 	// Restore a0
	ldq	a1, FRM_Q_A1(sp)	// Restore a1
	ldq	a2, FRM_Q_A2(sp) 	// Restore a2
	lda	sp, FRM_K_SIZE(sp)	// Deallocate the frame

	and	t0, PS_M_CM, t1		// get current mode
	beq	t1, 3f			// kernel or user mode?
	mfpr	sp, ptUsp		// Restore user stack pointer.
3:	mtpr	t0, pt9_ps		// Set ps back to original
	br	zero, pal_mchk_logout

/*
** 82378IB SIO Device Interrupt Handler
**
** IRQ1 handler code - IPL 3
**
** The SIO interrupt controller logic contains two 82C59A megacells,
** with the second megacell cascaded into the first.  Two interrupt 
** acknowledge cycles are required with a delay in between. On the first
** cycle, the cascading priority is resolved to determine which of the 
** two megacells will output the interrupt vector onto the data bus.
** On the second cycle, the appropriate megacell drives the data bus with
** the correct interrupt vector for the highest priority interrupt (the
** controller's IRs (not the 21064's) are implicitly ordered: master IR0, 
** master IR1, the slave's IR0 - IR7 (since it comes in on the master's IRQ2),
** master IR3 - IR7.
**
** The vector returned has bits <7:3> loaded with an offset set a system 
** reset, and bits <2:0> contain the IR. IR7 means passive release, i.e., 
** something gliched and is no longer requesting the interrupt, or it
** could mean a real interrupt. Read IS register to determine which.
*/
	ALIGN_BRANCH_TARGET

sys_irq1_handler:

	IACK(a2,t0)

	and	a2, 0xff, a1		// Clean vector to 8 bits.
	cmpeq	a1, 7, a0		// Check for IR7.
	beq	a0, 0f 			// If not, continue on...
/*
** Determine whether this is really an interrupt on IR7 that is
** requesting service, or simply a passive release. Read the
** IS register to get the status of each interrupt request line.
*/
					// Set up OCW3 to read IS.
	OutPortByte(PIC1_OCW3, 0x0B, t0, t1)

	InPortByte(PIC1_OCW3, t0)	// Read the IS.

	srl	t0, 7, t0		// Shift IS<7> into low bit.
	blbc	t0, 5f			// Passive release, dismiss.
/*
** Determine which of the two controllers the interrupt came in on 
** and dismiss it accordingly.  If the interrupt is coming in on
** the slave controller (INT1) then we need to dismiss both the 
** interrupt on the slave, and also IR2 on the master (INT0).
** IR2 is used to cascade the two controllers together and is not
** available as an external pin.
*/
0:
#if 0
	cmple	a1, 7, a0		// Check if interrupt is on INT0 or INT1
	bis	a1, zero, gp		// Save a copy of the vector.
	bne	a0, 4f 			// If set, interrupt is on INT0.
/*
** Service interrupt on INT1 ...
*/
	and	a1, 7, a1		// Mask IR0-7 for specific EOI.
	lda	a0, 0x60(zero)		// Load specific EOI command.
	bis	a0, a1, a0		// Add in the IR level to be acted upon

	OutPortByteReg(PIC2_OCW2,a0,t0,t1) // Send EOI command.

	bis	zero, 2, a1		// Now dismiss IR2 on INT0.
/*
** Service interrupt on INT0 ...
*/

4:	lda	a0, 0x60(zero)		// Load specific EOI command.
	bis	a0, a1, a0		// Add in the IR level to be acted upon

	OutPortByteReg(PIC1_OCW2,a0,t0,t1) // Send EOI command.
	bis	gp, zero, a1		// Restore original interrupt vector
#endif /* XXX */

/*
** So we got a device interrupt. IPL=3 for all device interrupts.
** Convert interrupt byte to SCB address. Set up a0, a1, t1.
*/
	sll	a1, 4, a1		// Multiply by 16 for SCB offset
	lda	a1, 0x800(a1)		// Load SCB base + offset.
	or	zero, INT_K_DEV, a0	// Indicate device interrupt
	or	zero, IPL_K_DEV0, t1	// Set IPL = 3

	br	zero, pal_interrupt_post
/*
** Passive release from the controller, i.e., IR7 
**
** Send non-specific EOI command to INT0 and INT1
*/
	ALIGN_BRANCH_TARGET

5:	OutPortByte(PIC2_OCW2, 0x20, a0, a2)
	OutPortByte(PIC1_OCW2, 0x20, a0, a2)

	br	zero, sys_dismiss_interrupt

/*
** DS1287A Real Time Clock Interrupt Handler
**
** IRQ2 handler code - IPL 5
**
*/
sys_irq2_handler:

	OutPortByte(RTCADD,0x0C,t0,t1)	// Set up RTCADD to index register C.
	InPortByte(RTCDAT,t0)		// Dismiss interrupt by reading csr.

	or	zero, INT_K_CLK, a0	// Indicate clock interrupt.
	or	zero, IPL_K_CLK, t1	// Set IPL = 5.

	br	zero, pal_interrupt_post
/*
** If we got here it means that the interrupt either went
** away (late passive release), or we are simply choosing
** to explicitly ignore the interrupt.  Unwind the kernel 
** stack, restore scratch registers, and return.
*/
	ALIGN_BRANCH_TARGET

sys_dismiss_interrupt:

        ldq     gp, FRM_Q_GP(sp)        // Restore gp
        ldq     t0, FRM_Q_PS(sp)        // Restore ps
        ldq     a0, FRM_Q_A0(sp)        // Restore a0
        ldq     a1, FRM_Q_A1(sp)        // Restore a1
        ldq     a2, FRM_Q_A2(sp)        // Restore a2
        lda     sp, FRM_K_SIZE(sp)      // Deallocate the frame
        and     t0, PS_M_CM, t1         // Get the current mode
        beq     t1, 1f                  // Kernel or user mode?
        mfpr    sp, ptUsp               // If user mode, restore sp
1:	mtpr	t0, pt9_ps		// Restore original ps and shadow
	STALL
	STALL
        mfpr    t3, pt3                 // Restore scratch registers
        mfpr    t1, pt1
        mfpr    t0, pt0
        hw_rei                          // Return from interrupt ...

/*
**
** FUNCTIONAL DESCRIPTION:
**
**      The cache flush (cflush) function flushes an entire physical
**      page, specified by a PFN passed in a0, from any data caches 
**	associated with the current processor.
**
** CALLING SEQUENCE:
**
**	Branched into from the cflush entry point code.
**
** INPUT PARAMETERS:
**
**      a0 - Page Frame Number of the page to be flushed 
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
**      Registers t0, t8 .. t11 are UNPREDICTABLE upon return
** 
*/
	ALIGN_CACHE_BLOCK
/*
** Convert pfn to a page aligned address.  Then invert one bit
** in the tag.  The result is a physical address that conflicts in 
** the cache with the page to be flushed (since the tag has been made 
** different).  Doing a load at that address will displace whatever 
** is in the cache (unless it happens to be that exact address, which
** it is ok to leave in the cache).
**
*/
sys_cflush:
	mtpr	t1, pt1			// Save t1

/*
** Convert pfn to an address and clean it down to the first tag address bit,
** which is BC_V_SIZE (19 for a 512KB bcache). Then xor the first tag address
** bit to ensure we victimize the page.
*/
	sll	a0, (VA_S_OFF+(63-BC_V_SIZE)), t1
	ldah	t0, ((1<<BC_V_SIZE)>>16)(zero)

	srl	t1, 63-BC_V_SIZE, t1	// Shift back to normal position
	xor	t0, t1, t1		// Xor bit to ensure different tag

/*
** Now loop through one page worth of reads, doing reads of 8 cache
** blocks per loop.  Note that a load of one quadword in the block 
** will allocate four quadwords.  t0 will be used as a counter.
**
** Also, check for interrupts in the loop and retry the cflush if we get
** one.  If that isn't done then cflush will cause interrupt latency be huge.
*/
	bis	zero, VA_S_PAGE_SIZE/(32*8), t0

1:	subq	t0, 1, t0		// Decrement counter

	ldq_p	t8,  32*0(t1)		// Do a load
	ldq_p	t9,  32*1(t1)		// Do next load
	ldq_p	t10, 32*2(t1)		// Do next load
	ldq_p	t11, 32*3(t1)		// Do next load
	ldq_p	t8,  32*4(t1)		// Do next load
	ldq_p	t9,  32*5(t1)		// Do next load
	ldq_p	t10, 32*6(t1)		// Do next load
	ldq_p	t11, 32*7(t1)		// Do next load

	mfpr	t8, hirr		// Get hardware interrupt mask's
	and	t8, HIRR_M_HWR, t8	// Check interrupts
	lda	t1, 32*8(t1)		// Skip to next cache block address
	bne	t8, 2f			// If any pending, retry cflush
	bne	t0, 1b			// Loop till done

	mfpr	t1, pt1			// Restore t1
	hw_rei				// Back to user
/*
** Come here to retry the cflush
*/

2:	mfpr	t8, excAddr		// Fetch the pc
	subq	t8, 4, t8		// Back up pc to point at the cflush
	mtpr	t8, excAddr		// Re-try
	mfpr	t1, pt1			// Restore t1
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	The console service (cserve) function implements console and
**	platform specific operations.
**
** CALLING SEQUENCE:
**
**	Branched into from the CALL_PAL cserve entry point.
**
** INPUT PARAMETERS:
**
**	a0 - function parameter
**	a1 - function parameter
**	a2 - function type
**
** OUTPUT PARAMETERS:
**
**	v0 - function result
**
** SIDE EFFECTS:
**	
*/
	ALIGN_CACHE_BLOCK

sys_cserve:
	cmpeq	a2, CSERVE_K_LDQP, v0
	bne	v0, sys_cserve_ldqp

	cmpeq	a2, CSERVE_K_STQP, v0
	bne	v0, sys_cserve_stqp

	cmpeq	a2, CSERVE_K_RD_ABOX, v0
	bne	v0, sys_cserve_rd_abox

        cmpeq   a2, CSERVE_K_RD_ESR, v0
	bne	v0, sys_cserve_rd_esr
	cmpeq	a2, CSERVE_K_RD_ICCSR, v0
	bne	v0, sys_cserve_rd_iccsr

	cmpeq	a2, CSERVE_K_WR_ABOX, v0
	bne	v0, sys_cserve_wr_abox

        cmpeq   a2, CSERVE_K_WR_ESR, v0
	bne	v0, sys_cserve_wr_esr
	cmpeq	a2, CSERVE_K_WR_ICCSR, v0
	bne	v0, sys_cserve_wr_iccsr

	cmpeq	a2, CSERVE_K_JTOPAL, v0
	bne	v0, sys_cserve_jtopal

	cmpeq	a2, CSERVE_K_WR_INT, v0
	bne	v0, sys_cserve_wr_int

	cmpeq	a2, CSERVE_K_RD_IMPURE, v0
	bne	v0, sys_cserve_rd_impure

	cmpeq	a2, CSERVE_K_PUTC, v0
	bne	v0, sys_cserve_putc

	hw_rei                          // If none of the above, just return ...

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Load (physical) quadword from memory to register v0.
**
** INPUT PARAMETERS:
**
**      a0 (r16) - Physical address
**
** OUTPUT PARAMETERS:
**
**	v0 (r0)  - Returned data
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_ldqp:
	ldq_p	v0, 0(a0)
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Store (physical) quadword from register a1 to memory.
**
** INPUT PARAMETERS:
**
**      a0 (r16) - Physical address
**      a1 (r17) - Data to be written
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_stqp:
	stq_p	a1, 0(a0)
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Return the shadow copy of the ABOX_CTL IPR in 
**      register v0 (r0).
**
** INPUT PARAMETERS:
**
**      None
**
** OUTPUT PARAMETERS:
**
**	v0 (r0) - Returned ABOX_CTL value
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_rd_abox:
	mfpr	t0, ptImpure            // Get base of impure scratch area
	lda	t0, CNS_Q_BASE(t0)      // Point start of save state area.
        ldq_p   v0, CNS_Q_ABOX_CTL(t0)  // Read the ABOX_CTL shadow copy.
	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Return the Memory Controller Error Status IPR in 
**      register v0 (r0).
**
** INPUT PARAMETERS:
**
**      None
**
** OUTPUT PARAMETERS:
**
**	v0 (r0) - Returned ESR value
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_rd_esr:
	LOAD_REGION_BASE(t0,MEM_CSR_BASE)  // Get Memory Controller CSR base.
	ldq_p	v0, esr(t0)                // Read the Error Status Register.
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Return the I-Cache Control and Status IPR in 
**      register v0 (r0).
**
** INPUT PARAMETERS:
**
**      None
**
** OUTPUT PARAMETERS:
**
**	v0 (r0) - Returned ICCSR value
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_rd_iccsr:
        mfpr	v0, pt2			// Read ICCSR shadow copy
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Write the Abox Control IPR 
**
** INPUT PARAMETERS:
**
**      a0 (r16) - New ABOX_CTL value
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**	
*/
	ALIGN_BRANCH_TARGET

sys_cserve_wr_abox:
	mfpr	t0, ptImpure		// Get base of impure scratch area
	lda	t0, CNS_Q_BASE(t0)      // Point to start of save state area
        stq_p   a0, CNS_Q_ABOX_CTL(t0)  // Update the ABOX_CTL shadow copy
	mtpr	a0, aboxCtl		// Write the ABOX_CTL IPR
        STALL                           // Required stall to update chip ...
	STALL
	STALL
	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Write the Memory Controller Error Status IPR
**
** INPUT PARAMETERS:
**
**      a0 (r16) - New ESR value
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_wr_esr:
	LOAD_REGION_BASE(t0,MEM_CSR_BASE)  // Get Memory Controller CSR base.
	stq_p	a0, esr(t0)                // Write the new ESR value
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Write the I-Cache Control and Status IPR
**
** INPUT PARAMETERS:
**
**      a0 (r16) - New ICCSR value
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_wr_iccsr:
	mtpr	a0, pt2_iccsr		// Write ICCSR and shadow copy
	STALL                           // Required stall to update chip ...
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

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Transfer control to the specified address, passed in 
**      register a0, in PAL mode.
**
** INPUT PARAMETERS:
**
**      a0 (r16) - Transfer address
**
** OUTPUT PARAMETERS:
**
**   DECchip 21064 specific parameters:
**   
**      t0 (r1)  - aboxCtl
**      t1 (r2)  - biuCtl
**
**   DECchip 21066 specific parameters:
**
**      t0 (r1)  - aboxCtl
**      t1 (r2)  - Bank Configuration Register 0/1
**      t2 (r3)  - Bank Configuration Register 2/3
**      t3 (r4)  - Bank Mask Register 0/1
**      t4 (r5)  - Bank Mask Register 2/3
**
**   Firmware specific parameters:
**
**      a1 (r17) - Size of good memory in bytes
**      a2 (r18) - Cycle count in picoseconds
**      a3 (r19) - Protocol signature and system revision
**      a4 (r20) - Active processor mask
**      a5 (r21) - System Context value
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_jtopal:
	bic	a0, 3, t8		// Clear out low 2 bits of address
	bis	t8, 1, t8		// Or in PAL mode bit

	mfpr	t11, ptImpure		// Get base of impure scratch area.
        lda     t11, CNS_Q_BASE(t11)    // Point to start of save state area.
        ldq_p   a3, CNS_Q_SIGNATURE(t11)// Read the saved protocol signature.
        srl     a3, 16, t0              // Shift signature into lower word.

	LDLI(t1,0xDECA)			// Load the expected valid signature.

        cmpeq   t0, t1, t0              // Check if saved signature was valid.
        blbc    t0, 1f                  // If invalid, pass nothing.
/*
** Load the processor specific parameters ...
*/
	ldq_p	t0, CNS_Q_ABOX_CTL(t11)

	ldq_p	t1, CNS_L_BCR0(t11)
	ldq_p	t2, CNS_L_BCR2(t11)
	ldq_p	t3, CNS_L_BMR0(t11)
	ldq_p	t4, CNS_L_BMR2(t11)
/*
** Load the firmware specific parameters ...
*/
	ldq_p	s6, CNS_Q_SROM_REV(t11)
	ldq_p	a0, CNS_Q_PROC_ID(t11)
	ldq_p	a1, CNS_Q_MEM_SIZE(t11)
	ldq_p	a2, CNS_Q_CYCLE_CNT(t11)
	ldq_p	a4, CNS_Q_PROC_MASK(t11)
	ldq_p	a5, CNS_Q_SYSCTX(t11)

1:	mtpr	zero, ptWhami		// Clear WHAMI and swap bit.
	mtpr	t8, excAddr		// Load the dispatch address.
	mtpr	zero, flushIc		// Flush the icache.
	STALL                           // Required stalls ...
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	hw_rei				// Dispatch in PAL mode ...

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Write the Interrupt Mask IPR
**
** INPUT PARAMETERS:
**
**      a0 (r16) - New mask value
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_wr_int:
	mtpr	a0, ptIntMask		// Write new interrupt mask
        STALL                           // Required stall to update chip ...
	STALL
	STALL
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Return the base address of the PAL impure scratch
**      area in register v0 (r0).
**
** INPUT PARAMETERS:
**
**      None
**
** OUTPUT PARAMETERS:
**
**	v0 (r0) - Returned base address of impure scratch area.
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_rd_impure:
	mfpr	v0, ptImpure		// Get base of impure scratch area.
	hw_rei

/*
**
** FUNCTIONAL DESCRIPTION:
**
**      Output a character to the serial port.
**
** INPUT PARAMETERS:
**
**      None
**
** OUTPUT PARAMETERS:
**
**	v0 (r0) - Return status
**
** SIDE EFFECTS:
**
*/
	ALIGN_BRANCH_TARGET

sys_cserve_putc:
	mtpr	t0, pt0

1:	InPortByte(COM1_LSR,t0)

	srl     t0, LSR_V_THRE,t0
	blbc    t0, 1b

	OutPortByteReg(COM1_THR,a0,t0,v0)

	lda     v0, 1(zero)
	mfpr	t0, pt0
	hw_rei


/*
**
** FUNCTIONAL DESCRIPTION:
**
**	Save state in the impure area, zap a few things, and transfer
**	to console I/O mode.
**
**      NOTE: This routine is tailored for use with the debug monitor
**      on the DECchip Evaluation Boards.
**
** CALLING SEQUENCE:
**
**      Entered via branch to enter console. 
**
** INPUT PARAMETERS:
**
**	pt0 - saved t0
**      t0  - reason for entering console
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	ALIGN_CACHE_BLOCK

sys_enter_console:
	mtpr	t0, pt3			// Save reason for entering console.
					// Note that pt0 has original t0.
	mtpr	t1, pt1			// Save t1.
	STALL				// Avoid mtpr/mfpr conflict
	STALL
	mfpr	t1, ptImpure		// Get pointer to impure scratch area.

	SAVE_STATE			// Save the current state.
					// Assumes pt0 is original saved t0.

	mfpr	t0, ptVptPtr		// Get the virtual page table pointer.
	mfpr	t1, ptPtbr		// Get the page table base.
	bis	t0, 1, t0		// Enable physical mode translation.
	bis	t1, 1, t1
	mtpr	t0, ptVptPtr		// Update the chip ...
	mtpr	t1, ptPtbr

	LDLI(sp,0x001FE000)		// Initialize the kernel stack pointer.

	mtpr	sp, ptKsp               // Update the saved KSP value.
	mtpr	zero, ps                // Set mode to kernel, IPL = 0.
	mtpr	zero, dtbZap            // Clear all D-stream TB entries.
	mtpr	zero, itbZap	        // Clear all I-stream TB entries.
	mtpr	zero, hier              // Clear all interrupts ...
	mtpr	zero, sier
	mtpr	zero, aster
	mtpr	zero, sirr
	mtpr	zero, astrr

	STALL				// Required stalls to update chip ...
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL

	br	t0, 1f                  // Branch over static data
	.long	CONSOLE_ENTRY
1:	ldl_p	t1, 0(t0)               // Get the transfer address
	mtpr	t1, excAddr             // Load console entry point
	mfpr	t0, pt3                 // Restore t0 = reason for entry.
	mfpr	t1, pt1			// Restore t1.
	hw_rei                          // Transfter to console I/O mode ...

/*
** FUNCTIONAL DESCRIPTION:
**
**	Scrub cache block using physical load locked and physical store
**	conditional instructions.
**
** CALLING SEQUENCE:
**
**      Entered via branch subroutine to scrub memory on
**	crd errors.
**
** INPUT PARAMETERS:
**
**      t0	return address
**	t1	available (calling routine preserves if necessary)
**	t3	fill address within the block
**	a0..a2	available
**	gp	available
**
** OUTPUT PARAMETERS:
**
**	None
**
** SIDE EFFECTS:
**
*/
	ALIGN_CACHE_BLOCK
/*
** Note: alignment is critical -- as mchk's will be turned off during
** the scrub, we must make sure that the istream is safely in the Icache
** prior to turning off mchk's.  Else, we could suck garbage into the
** istream and not be able to detect it.  
**
** The istream is loaded into Icache by branching to the first
** intruction of each cache block, which is a branch to the next block.
** When the branch is fetched the cache will fill the block with 7
** additional instructions.  After all instructions are in Icache the
** code is executed, with the 8th instruction in each block being a
** branch to the 2nd instruction of the next block - to skip over the
** branch instructions used to fill the icache (there are 6 'real'
** instructions per block). 
**
** The address with the bad ecc could be in either the BCache or DRAM.
** In addition it could be in either cacheable or non-cacheable space.
** If in cacheable space it would have been cached at the time the ecc
** error was detected (either already in Bcache or allocted in during
** the load), however it could have been evicted between then and now.
**
** The following table enumerates the cases:
**
** Cacheable? In Cache?
** ---------- ---------
**     Y         N     Will be allocated on ldq_pa; 
**		          stq_pa will write corrected data to BCache
**     Y         Y     stq_pa will write corrected data to BCache
**     N         N     stq_pa will write corrected data to DRAM
**     N         Y     Not possible
**
** In the cases where the corrected data is written to BCache it will be
** put back into the DRAM when the block is eventually evicted.
**
** Since the EB64 and EB64+ both operate in parity mode, this flow will
** never actually be executed on either system and is included complete
** as an example.
*/
	ALIGN_BRANCH_TARGET
sys_scrub_mem:
	mfpr	t1, ptImpure
	lda	t1, CNS_Q_BASE(t1)
	ldq_p	a1, CNS_Q_ABOX_CTL(t1)	     // read shadow of ABOX_CTL
	lda	a0, ABOX_M_MCHK_EN(zero)     // get mchk enable bit
	bic	a1, a0, a0		     // a0 <- abox_ctl with mchk turned off
	bic     t3, 31, t3                   // truncate addr to start of block

/*
** Make sure that if the error address is in I/O space, it doesn't get
** scrubbed.  I/O space on the evaluation boards can be identified as
** follows:
**
** On the EB64 and EB64+, if either bit 33 or bit 32 of the physical
** address is set, the address is in I/O space.
**
** On the EB66, I/O space is handled by the I/O controller and doesn't
** need to be checked for here since the only way to get here is a
** correctable error reported by the memory controller.
*/
/*
** End of I/O space check
*/

	br	zero, sys_scrub_mem_fetch_00 // fetch first cache block

/*
** The following blocks should be modified very carefully adn only if
** necessary.  These blocks are brought into the icache prior before 
** disabling machine checks and must be no larger than a cache block 
** in length (8 instructions, including the fetching branch and the
** execution branch to the next cache block.  Each of these blocks
** must also be cache block aligned.
*/

	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_00:
	br	zero, sys_scrub_mem_fetch_10 // fetch next cache block
sys_scrub_mem_exec_00:
	mtpr	a0, aboxCtl                  // disable mchk...
	STALL				     // ...and stall to allow...
	STALL				     // ...time for mchk disable...
	STALL				     // ...to propagate
	STALL
	STALL
	br	zero, sys_scrub_mem_exec_10  // continue in next cache block

	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_10:
	br	zero, sys_scrub_mem_fetch_20 // fetch next cache block
sys_scrub_mem_exec_10:
	STALL
sys_scrub_mem_load:
	ldq_pa	a0, 0(t3)                    // read physical locked
	or	a0, zero, a0                 // let the load complete
	LOAD_REGION_BASE(gp, MEM_CSR_BASE)   // 2 instructions
	ldq_p	a2, esr(gp)                  // read esr
	br	zero, sys_scrub_mem_exec_20  // continue in next cache block

	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_20:
	br	r31, sys_scrub_mem_fetch_30  // fetch next cache block
sys_scrub_mem_exec_20:
	stq_p	a2, esr(gp)                  // clear the ESR errors
	lda	gp, ESR_M_CTE | ESR_M_UEE | ESR_M_NXM(zero)
	and	a2, gp, gp                   // any weird errors on the load?
	bne	gp, sys_scrub_mem_leave
	br	zero, sys_scrub_mem_exec_30  // continue in next cache block

/*
** If there were no uncorrectable errors on the load, attempt the store.
** If the conditional store fails, the go back to sys_scrub_mem_load and
** try again until the store succeeds.  Probably should put a counter
** and limit the number of retries before going on to the next quad 
** in the block, but that would require another register and another
** cache block. 
*/
	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_30:
	br	zero, sys_scrub_mem_fetch_40 // fetch next cache block
sys_scrub_mem_exec_30:
sys_scrub_mem_store:
	stq_pa	a0, 0(t3)                    // do physical store conditional
	STALL
	blbc	a0, sys_scrub_mem_load       // ...repeat until store succeeds
	br	zero, sys_scrub_mem_exec_40  // continue in next cache block

/*
** The store succeeded so go on to the next quad.  t3 is being used as
** both an address and a counter here.  Incrementing t3 by 9
** accomplishes incrementing of the address by 8 (to the next quad) and
** the counter by 1.  We only loop 4 times (4 quads in an internal cache
** block), so the address won't accidentally by incremented to skip a
** quad.
*/
	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_40:
	br	zero, sys_scrub_mem_fetch_50 // fetch next cache block
sys_scrub_mem_exec_40:
	addq	t3, 9, t3                    // bump address by 8 & counter by 1
	and	t3, 4, a0                    // have we done 4 store's?
	beq	a0, sys_scrub_mem_load       // no, go back and do next
sys_scrub_mem_leave:
	mtpr	a1, aboxCtl                  // re-enable mchk's
	LOAD_REGION_BASE(gp, MEM_CSR_BASE)   // 2 instructions
	br	zero, sys_scrub_mem_exec_50  // continue in next cache block

/*
** Now that we have touched everything, start the real flow
**
** When exiting, make sure everything is unlocked. 
** DC21064: Question: do we want to read from biuAddr? Note that
**   biuStat<7:0> were unlocked on the read, and we might want to catch
**   errors on the outstanding write.  So we don't want to confuse things
**   by reading from biuAddr.
** DC21066: All errors in ESR are cleared here, so if there really was
**   an error we want to keep we will have to stumble over it again.
*/
	ALIGN_CACHE_BLOCK
sys_scrub_mem_fetch_50:
	br	zero, sys_scrub_mem_exec_00  // now start the real flow
sys_scrub_mem_exec_50:
	ldq_p	a2, esr(gp)                  // read esr
	stq_p	a2, esr(gp)                  // clear the ESR errors
	br	zero, sys_scrub_mem_exec_60  // continue, in next+1 cache block
/*
** This is a dead cache block.. It is never executed, but
** is here in case the stream buffer brings it in while we
** are pre-fetching data into the Icache. If this were not
** here, we could possibly execute garbage code if we managed to
** have multple ecc errors on THIS cache block. The odds of that
** happening are pretty slim, but with this here, we are safe
** from that failure.
*/
	ALIGN_CACHE_BLOCK
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
	STALL
/* 
** Make sure lock flag gets cleared (from punt error flows)
*/      
	ALIGN_CACHE_BLOCK

sys_scrub_mem_exec_60:
	br	a1, 1f
	.long	pal_lockCell
1:      ldl_p	a1, 0(a1)               // get lockCell address
	mfpr	a0, palBase             // get pal base
	addq	a0, a1, a0              // add the two to get address
	stl_pa	a0, 0(a0)               // do unlock
pvc$osf30$5004.1:
	ret	zero, (t0)


	ALIGN_CACHE_BLOCK
/*
** When doing a swppal to this PALcode, we need to load ptIntMask
*/

sys_reset_swppal:
	STALL
	br	t0, sys_patch1		// Branch over static data.
	.quad	INT_K_MASK
sys_patch1:
	ldq_p	t1, 0(t0)		// Load the interrupt mask.
	mtpr	t1, ptIntMask		// Establish the interrupt mask.
pvc$osf71$5020.1:
	ret	zero, (v0)		// Return to caller.


	ALIGN_CACHE_BLOCK

sys_reset:
	STALL
	br	t0, sys_patch0		// Branch over static data.
	.quad	INT_K_MASK
sys_patch0:
	ldq_p	t1, 0(t0)		// Load the interrupt mask.
	mtpr	t1, ptIntMask		// Establish the interrupt mask.

/*
** Initialize the serial ports
**
**	Baud rate:	9600 baud
**	Word length:	8 bit characters
**	Stop bits:	1 stop bit
**	Parity:		No parity
**	Modem control:	DTR, RTS active, OUT1 lov, UART interrupts enabled
*/

/*
** Initialize COM1
*/
	OutPortByte(COM1_LCR,LCR_M_DLAB,t0,t1)	// Access clock divisor latch.
	OutPortByte(COM1_DLL,DLA_K_BRG,t0,t1)	// Set the baud rate.
	OutPortByte(COM1_DLH,0,t0,t1)
	OutPortByte(COM1_LCR,LCR_K_INIT,t0,t1)	// Set line control register.
	OutPortByte(COM1_MCR,MCR_K_INIT,t0,t1)	// Set modem control register.
	OutPortByte(COM1_IER,0x0F,t0,t1)	// Turn on interrupts.
/*
** Flush COM1's receive buffer. Allow for timeout if com lines not on
** board and not in ISA slot.
*/
	lda	t1, 16(zero)			// Load time out counter.
sys_reset_flush_com1:
	InPortByte(COM1_LSR,t0)			// Read the line status.
	blbc	t0, sys_reset_com1_done		// Are we done yet?
	InPortByte(COM1_RBR,t0)			// Read receive buffer register.
	subq	t1, 1, t1			// Decrement counter.
	bgt	t1, sys_reset_flush_com1	// Loop till done.

sys_reset_com1_done:
	mb
/*
** Initialize COM2
*/
	OutPortByte(COM2_LCR,LCR_M_DLAB,t0,t1)	// Access clock divisor latch.
	OutPortByte(COM2_DLL,DLA_K_BRG,t0,t1)	// Set the baud rate.
	OutPortByte(COM2_DLH,0,t0,t1)
	OutPortByte(COM2_LCR,LCR_K_INIT,t0,t1)	// Set line control register.
	OutPortByte(COM2_MCR,MCR_K_INIT,t0,t1)	// Set modem control register.
	OutPortByte(COM2_IER,0x0F,t0,t1)	// Turn on interrupts.
/*
** Flush COM2's receive buffer. Allow for timeout if com lines not on
** board and not in ISA slot.
*/
	lda	t1, 16(zero)			// Load time out counter.
sys_reset_flush_com2:
	InPortByte(COM2_LSR,t0)			// Read the line status.
	blbc	t0, sys_reset_com2_done		// Are we done yet?
	InPortByte(COM2_RBR,t0)			// Read receive buffer register.
	subq	t1, 1, t1			// Decrement counter.
	bgt	t1, sys_reset_flush_com2	// Loop till done.

sys_reset_com2_done:
	mb

/*
**
** Initialize the Intel 82C59A Priority Interrupt Controller (PIC)
**
** The interrupt controller logic consists of two 82C59A megacells with
** eight interrupt request lines each for a total of 16 interrupts.  The
** two megacells are cascaded internally (via IRQ2) and two of the interrupt
** request inputs are connected to the internal circuitry of the VL82C486
** (SC486) Controller (IRQ0 and IRQ13). This allows a total of 13 external
** interrupt requests.
**
** A typical interrupt sequence is as follows.  Any unmasked interrupt
** generates an interrupt request signal to the CPU.  The interrupt
** controller megacells then respond to the interrupt acknowledge pulses
** from the CPU.  On the first interrupt acknowledge cycle, the cascading
** priority is resolved to determine which of the two 82C59A megacells
** outputs the interrupt vector onto the data bus.  On the second interrupt
** acknowledge cycle, the appropriate 82C59A megacell drives the data bus
** with an 8-bit pointer to the correct interrupt vector for the highest
** priority interrupt.
**
**		CONTENT OF INTERRUPT VECTOR BYTE:
**
**			D7 D6 D5 D4 D3 D2 D1 D0
** 
**		IR7	0  0  0  0  1  1  1  1
**		IR6	0  0  0  0  1  1  1  0
**		IR5	0  0  0  0  1  1  0  1
**		IR4	0  0  0  0  1  1  0  0
**		IR3	0  0  0  0  1  0  1  1
**		IR2	0  0  0  0  1  0  1  0
**		IR1	0  0  0  0  1  0  0  1
**		IR0	0  0  0  0  1  0  0  0
**
** If no interrupt is present during either interrupt acknowledge sequence
** (i.e. the request was too short in duration) the 82C59A will issue an
** interrupt level 7 (passive release).
**
** INITIALIZATION COMMAND WORDS (ICW)
**
** Whenever a command is issued with A0=0 and D4=1, this is interpreted as
** Initialization Command Word 1 (ICW1).  ICW1 starts the initialization
** sequence during which the following automatically occur.
**
**	a. The edge sense circuit is reset, which means that
**	   following initialization, an interrupt request input
**	   must make a low-to-high transition to generate an
**	   interrupt.
**
**	b. The interrupt mask register is cleared.
**
**	c. IRQ7 input is assigned priority 7.
**
**	d. The slave mode address is set to 7.
**
**	e. Special mask mode is cleared and status read is
**	   set to IRR.
**
**	f. If IC4=0, then all functions selected in ICW4 
**	   are set to zero.
** 
** INITIALIZATION COMMAND WORD 1 FORMAT (ICW1):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 0 |  | 0 | 0 | 0 | 1 | x | 0 | 0 | 1 |
** +---+  +---+---+---+---+---+---+---+---+
**                          ^   ^   ^   ^
**                          |   |   |   |
**                          |   |   |   +---- ICW4 needed 
**                          |   |   |
**                          |   |   +-------- Cascade mode
**                          |   |
**                          |   +------------ Call address interval of 8
**                          |
**                          +---------------- 0 = Edge triggered mode (INT2)
**                                            1 = Level triggered mode (INT1)
** 
** INITIALIZATION COMMAND WORD 2 FORMAT (ICW2):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 1 |  | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |
** +---+  +---+---+---+---+---+---+---+---+
**          ^   ^   ^   ^   ^
**          |   |   |   |   |
**          +---+---+---+---+---------------- T7 - T3 of interrupt vector byte
**
**
** INITIALIZATION COMMAND WORD 3 FORMAT (ICW3):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 1 |  | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 0 |
** +---+  +---+---+---+---+---+---+---+---+
**                              ^   ^
**                              |   |
**                              |   +-------- Slave ID: 00110011 (INT2)
**                              |
**                              +------------ IRQ2 input has a slave (INT1)
**
** INITIALIZATION COMMAND WORD 4 FORMAT (ICW4):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 1 |  | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
** +---+  +---+---+---+---+---+---+---+---+
**                      ^   ^   ^   ^   ^
**                      |   |   |   |   |
**                      |   +-+-+   |   +---- 80C86/80C88 mode
**                      |     |     |
**                      |     |     +-------- Normal end of interrupt (EOI)
**                      |     |
**                      |     +-------------- Non-buffered mode
**                      |    
**                      +-------------------- Not special fully nested mode
**
** OPERATION CONTROL WORD 1 FORMAT (OCW1):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 1 |  | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
** +---+  +---+---+---+---+---+---+---+---+
**          ^   ^   ^   ^   ^   ^   ^   ^
**          |   |   |   |   |   |   |   |
**          +---+---+---+---+---+---+---+---- Interrupt mask
**                                            All channels inhibited
**
** OPERATION CONTROL WORD 2 FORMAT (OCW2):
**
**   A0     D7  D6  D5  D4  D3  D2  D1  D0
** +---+  +---+---+---+---+---+---+---+---+
** | 1 |  | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 0 |
** +---+  +---+---+---+---+---+---+---+---+
**          ^   ^   ^
**          |   |   |
**          +---+---+
**              |
**              +---------------------------- Non-specific EOI command
**
**
** See the EB64 Engineering Specification for information on memory 
** address bit definitions and encodings for accessing ISA I/O space.
**
*/

/*
** Initialize the 82C59A priority interrupt controller (PIC)
*/
	OutPortByte(PIC2_ICW1,0x11,t0,t1)
	OutPortByte(PIC2_ICW2,0x08,t0,t1)
	OutPortByte(PIC2_ICW3,0x02,t0,t1)
	OutPortByte(PIC2_ICW4,0x01,t0,t1)

	OutPortByte(PIC2_OCW1,0xFF,t0,t1)

	OutPortByte(PIC1_ICW1,0x11,t0,t1)
	OutPortByte(PIC1_ICW2,0x00,t0,t1)
	OutPortByte(PIC1_ICW3,0x04,t0,t1)
	OutPortByte(PIC1_ICW4,0x01,t0,t1)

	OutPortByte(PIC1_OCW1,0xFF,t0,t1)

/*
** Send -INTA pulses to clear any pending interrupts ...
*/
	IACK(t0,t1)

/*
** Finish writing the 82C59A PIC Operation Control Words ...
*/
	OutPortByte(PIC2_OCW2,0x20,t0,t1)
	OutPortByte(PIC1_OCW2,0x20,t0,t1)

/*
** Initialize the real-time clock (RTC)
**
** Index into RTC Control Register A to set periodic interrupt rate
** to 976.562 and set it running.
*/

	OutPortByte(RTCADD,0x0A,t0,t1)
	OutPortByte(RTCDAT,0x26,t0,t1)

/*
** Index into RTC Control Register B and then enable periodic
** interrupts.
*/
	OutPortByte(RTCADD,0x0B,t0,t1)
	InPortByte(RTCDAT,t2)
	lda	t0, 0x40(zero)		// Periodic interrupt bit
	bis	t2, t0, t2		// Merge it in

	OutPortByte(RTCADD,0x0B,t0,t1)
	OutPortByteReg(RTCDAT,t2,t0,t1)

pvc$osf69$5104.1:
	ret	zero, (ra)		// Return to caller.

	ALIGN_PAGE

pal_impure:

