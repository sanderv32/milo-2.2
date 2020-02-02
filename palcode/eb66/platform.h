        //
        // VID: [T1.2] PT: [Mon Aug 29 13:13:01 1994] SF: [platform.h]
        //  TI: [/sae_users/samberg/palarea/tools/vice -iplatform.s -l// -p# -DDC21066 -DEB66 -h -m -aeb66 ]
        //
#define	__PLATFORM_LOADED	1
/*
*****************************************************************************
**                                                                          *
**  Copyright © 1993, 1994	       					    *
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
**	platform.h
**
**  MODULE DESCRIPTION:
**
**      Platform specific definitions.
**
**  AUTHOR: ER
**
**  CREATION DATE:  17-Nov-1992
**
**  $Id: platform.h,v 1.1.1.1 1995/08/01 17:33:55 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: platform.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:55  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:31:31  rusling
 * PALcode sources for the EB66 system (21066 based).
 *
 * Revision 1.3  1995/03/30  09:48:00  rusling
 * Switched back to old (1.4) EB version.
 *
**  Revision 2.8  1994/08/29  17:02:18  samberg
**  Eliminate SIO CFIG init, someone else already worries about it
**  The conditional reverse was incorrect anyway
**
**  Revision 2.7  1994/08/17  14:00:57  samberg
**  EB64P pass1 and pass2 SIO_CFIG were reversed.
**
**  Revision 2.6  1994/08/10  17:06:11  samberg
**  Added MCHK_K_SIO_... for SIO NMI
**
**  Revision 2.5  1994/06/16  15:29:30  samberg
**  Move cserve definitions out (to cserve.h)
**
**  Revision 2.4  1994/06/16  14:48:54  samberg
**  For ANSI, changed $ to _, except for pvc and reg def
**
**  Revision 2.3  1994/05/12  17:34:38  ericr
**  Added INDEX2 and DATA2 register port definitions for
**  remapping of RTC registers on EB64.
**
**  Revision 2.2  1994/04/04  20:33:12  ericr
**  Added new I/O address space map for pass 2 EB66
**
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.14  1994/03/31  18:06:25  ericr
**  Added interrupt masks for pass 2 EB66 and EB64+
**
**  Revision 1.13  1994/03/30  16:36:16  ericr
**  Moved LDLI macro to macros.h
**
**  Revision 1.12  1994/03/28  22:45:29  ericr
**  General cleanup of SIO definitions
**  Removed Define_IO_Port macro - not used
**
**  Revision 1.11  1994/03/24  20:51:24  ericr
**  Changed APECS_PASS1 conditional to PASS1.
**
**  Revision 1.10  1994/03/18  19:41:27  samberg
**  Combined the cflush flows
**
**  Revision 1.9  1994/03/18  13:23:09  jeffw
**  more cleanup of definitions and remove IN(), OUT(), PORT_SETUP()
**
**  Revision 1.8  1994/03/17  21:16:12  jeffw
**  Consolidate sio definitions and convert most to use DefinePort
**
**  Revision 1.7  1994/03/16  21:50:31  samberg
**  start cleaning up macros - DefinePort, OutPortByte, OutPortByteReg...
**
**  Revision 1.6  1994/03/14  15:15:34  ericr
**  Added CSERVE function constant definitions
**  Removed HWRPB constants - no longer needed
**
**  Revision 1.5  1994/03/11  23:56:28  ericr
**  Changed CONSOLE_ADDR to CONSOLE_ENTRY
**  Added Memory and I/O Controller enable bits to INT for EB66
**
**  Revision 1.4  1994/03/09  23:02:44  ericr
**  Removed definitions for initial ABOX_CTL, ICCSR,
**  and BIU_CTL values.
**
**  Revision 1.3  1994/03/08  00:14:34  ericr
**  Added CONSOLE_ADDR definition.
**  Removed EGORE and DEBUG_MONITOR conditionals.
**
**  Revision 1.2  1994/02/28  22:14:29  jeffw
**  change SIO timer defines to be port number only, and caps
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
**
*/

#if !defined(CONSOLE_ENTRY)
#define CONSOLE_ENTRY	        0x10000
#endif /* CONSOLE_ENTRY */

/*
**  External cache size is 2 to the power of BC_V_SIZE 
**  (used in call_pal cflush)
*/
#define BC_V_SIZE	20
/*======================================================================*/
/*              DECchip 21066 EVALUATION BOARD DEFINITIONS              */
/*======================================================================*/
/*
** EB66 Address Space Partitioning
**
**	   Address Bit
**	33 32 31 30 29 28	Description
**	-----------------	-------------------------
**	 0  1  0  0  1  0	Memory Controller CSRs
**	 0  1  1  0  0  0	I/O Controller CSRs
**	 0  1  1  0  1  -	PCI Interrupt ACK
**	 0  1  1  1  0  -	PCI I/O
**	 0  1  1  1  1  -	PCI Configuration
**	 1  0  -  -  -  -	PCI Sparse Memory
**	 1  1  -  -  -  -	PCI Dense Memory
**
*/

/*
** The following definitions need to be shifted left 28 bits
** to obtain their respective reference points.
*/

#define I_ACK_BASE		0x1A	// PCI Interrupt acknowledge
#define CFIG_BASE		0x1E	// PCI Configuration base address
#define IO_BASE			0x1C    // PCI I/O base address

#define BYTE_ENABLE_SHIFT	5

/*
** Interrupt Mask definitions:
**
** EB66 specific IRQ pins are
**
**  IRQ0   SIO     ipl <= 2
**  IRQ1   RTC     ipl <= 4
**  IRQ2   NMI     ipl <= 6
**  IRQ3   IOC
**  IRQ4   MERR
**  IRQ5   N/A
**
**  The mask contains one byte for each IPL level, with IPL0 in the
**  least significant (right-most) byte and IPL7 in the most
**  significant (left-most) byte.  Each byte in the mask contains
**  the following bit summary:
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	  <7>	  1	PC1	Performance Counter 1 enable
**	  <6>	  1	IRQ5    IRQ 5 enable
**	  <5>	  1	IRQ4    IRQ 4 enable
**	  <4>	  1	IRQ3    IRQ 3 enable
**	  <3>	  1	IRQ2	IRQ 2 enable
**	  <2>     1     IRQ1	IRQ 1 enable
**	  <1>     1	IRQ0	IRQ 0 enable
**	  <0>     1     PC0     Performance Counter 0 enable
**
**  The mask is written to <15:8> of the HIER register, where the
**  actual IRQ enables are in <14:9>, and <15> and <8> are PC1 and PC0
**  respectively.
*/

#define INT_K_MASK	0x0038383C3C3E3E3E


/*
** SIO Control Register Definitions:
*/

#define SIO_B_NMISC	0x61	// NMI Status and Control
#define SIO_B_NMI	0x70	// NMI Enable

#define SIO_NMISC       DefinePort(IO_BASE, SIO_B_NMISC)
#define SIO_NMI         DefinePort(IO_BASE, SIO_B_NMI)

/*
** NMISC - NMI Status and Control Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	   <7>    1     SERR	System Error
**	   <6>    1	IOCHK	IOCHK asserted on the ISA/SIO bus
**	   <3>	  1	IOCHK_E IOCHK Enable
**	   <2>	  1 	SERR_E  SERR Enable
*/

#define SIO_NMISC_V_SERR	7
#define SIO_NMISC_M_SERR	(1<<SIO_NMISC_V_SERR)
#define SIO_NMISC_V_IOCHK	6
#define SIO_NMISC_M_IOCHK	(1<<SIO_NMISC_V_IOCHK)
#define SIO_NMISC_V_IOCHK_EN    3
#define SIO_NMISC_M_IOCHK_EN	(1<<SIO_NMISC_V_IOCHK_EN)
#define SIO_NMISC_V_SERR_EN	2
#define SIO_NMISC_M_SERR_EN	(1<<SIO_NMISC_V_SERR_EN)

/*
** Define machine check codes for SIO NMI
*/

#define MCHK_K_SIO_SERR		0x204
#define MCHK_K_SIO_IOCHK	0x206

/* 
** Intel 82C59A Priority Interrupt Controller (PIC) Definitions:
*/

#define PIC1		0x20	// INT0 megacell address
#define PIC2		0xA0	// INT1 megacell address

#define ICW1		0	// Initialization control word 1 offset
#define ICW2		1	// Initialization control word 2 offset
#define ICW3		1	// Initialization control word 3 offset
#define ICW4		1	// Initialization control word 4 offset

#define OCW1		1	// Operation control word 1 offset
#define OCW2		0	// Operation control word 2 offset
#define OCW3		0	// Operation control word 3 offset

#define PIC1_ICW1	DefinePort(IO_BASE,PIC1+ICW1)
#define PIC1_ICW2	DefinePort(IO_BASE,PIC1+ICW2)
#define PIC1_ICW3	DefinePort(IO_BASE,PIC1+ICW3)
#define PIC1_ICW4	DefinePort(IO_BASE,PIC1+ICW4)
#define PIC1_OCW1	DefinePort(IO_BASE,PIC1+OCW1)
#define PIC1_OCW2	DefinePort(IO_BASE,PIC1+OCW2)
#define PIC1_OCW3	DefinePort(IO_BASE,PIC1+OCW3)

#define PIC2_ICW1	DefinePort(IO_BASE,PIC2+ICW1)
#define PIC2_ICW2	DefinePort(IO_BASE,PIC2+ICW2)
#define PIC2_ICW3	DefinePort(IO_BASE,PIC2+ICW3)
#define PIC2_ICW4	DefinePort(IO_BASE,PIC2+ICW4)
#define PIC2_OCW1	DefinePort(IO_BASE,PIC2+OCW1)
#define PIC2_OCW2	DefinePort(IO_BASE,PIC2+OCW2)
#define PIC2_OCW3	DefinePort(IO_BASE,PIC2+OCW3)

#define I_ACK   	DefinePort(I_ACK_BASE,0)

/*
** Dallas DS1287A Real-Time Clock (RTC) Definitions:
*/

#define RTCADD     	DefinePort(IO_BASE,0x70) // RTC address register
#define RTCDAT     	DefinePort(IO_BASE,0x71) // RTC data register

/*
** Serial Port (COM) Definitions:
*/

#define COM1			0x3F8	// COM1 serial line port address
#define COM2			0x2F8	// COM2 serial line port address

#define RBR			0	// Receive buffer register offset
#define THR			0	// Xmit holding register offset
#define DLL			0	// Divisor latch (LS) offset
#define DLH			1	// Divisor latch (MS) offset
#define IER			0x1	// Interrupt enable register offset
#define IIR			0x2	// Interrupt ID register offset
#define LCR			0x3	// Line control register offset
#define MCR			0x4	// Modem control register offset
#define LSR			0x5	// Line status register offset
#define MSR			0x6	// Modem status register offset
#define SCR			0x7	// Scratch register offset

#define DLA_K_BRG		12	// Baud rate divisor = 9600

#define LSR_V_THRE		5	// Xmit holding register empty bit

#define LCR_M_WLS		3	// Word length select mask
#define LCR_M_STB		4	// Number of stop bits mask
#define LCR_M_PEN		8	// Parity enable mask
#define LCR_M_DLAB		128	// Divisor latch access bit mask

#define LCR_K_INIT	      	(LCR_M_WLS | LCR_M_STB)

#define MCR_M_DTR		1	// Data terminal ready mask
#define MCR_M_RTS		2	// Request to send mask
#define MCR_M_OUT1		4	// Output 1 control mask
#define MCR_M_OUT2		8	// UART interrupt mask enable

#define MCR_K_INIT	      	(MCR_M_DTR  | \
				 MCR_M_RTS  | \
				 MCR_M_OUT1 | \
				 MCR_M_OUT2)

#define COM1_RBR		DefinePort(IO_BASE,COM1+RBR)
#define COM1_THR		DefinePort(IO_BASE,COM1+THR)
#define COM1_DLL		DefinePort(IO_BASE,COM1+DLL)
#define COM1_DLH		DefinePort(IO_BASE,COM1+DLH)
#define COM1_IER		DefinePort(IO_BASE,COM1+IER)
#define COM1_IIR		DefinePort(IO_BASE,COM1+IIR)
#define COM1_LCR		DefinePort(IO_BASE,COM1+LCR)
#define COM1_MCR		DefinePort(IO_BASE,COM1+MCR)
#define COM1_LSR		DefinePort(IO_BASE,COM1+LSR)
#define COM1_MSR		DefinePort(IO_BASE,COM1+MSR)
#define COM1_SCR		DefinePort(IO_BASE,COM1+SCR)

#define COM2_RBR		DefinePort(IO_BASE,COM2+RBR)
#define COM2_THR		DefinePort(IO_BASE,COM2+THR)
#define COM2_DLL		DefinePort(IO_BASE,COM2+DLL)
#define COM2_DLH		DefinePort(IO_BASE,COM2+DLH)
#define COM2_IER		DefinePort(IO_BASE,COM2+IER)
#define COM2_IIR		DefinePort(IO_BASE,COM2+IIR)
#define COM2_LCR		DefinePort(IO_BASE,COM2+LCR)
#define COM2_MCR		DefinePort(IO_BASE,COM2+MCR)
#define COM2_LSR		DefinePort(IO_BASE,COM2+LSR)
#define COM2_MSR		DefinePort(IO_BASE,COM2+MSR)
#define COM2_SCR		DefinePort(IO_BASE,COM2+SCR)

/*
** Macro to define a port address
*/
#define IO_MASK 	0x7FFFFFF

#define DefinePort(Base,PortAddr) \
	((Base<<(28-4)) | ((IO_MASK&PortAddr) << (BYTE_ENABLE_SHIFT-4)))
/*
** Macro to write a byte literal to a specified port
*/
#define OutPortByte(port,val,tmp0,tmp1) \
	LDLI	(tmp0, port); \
	sll	tmp0, 4, tmp0; \
	lda	tmp1, (val)(zero); \
	insbl	tmp1, ((port>>(BYTE_ENABLE_SHIFT-4))&3), tmp1; \
	stl_p	tmp1, 0x00(tmp0); \
	mb
/*
** Macro to write a byte from a register to a specified port
*/
#define OutPortByteReg(port,reg,tmp0,tmp1) \
	LDLI	(tmp0, port); \
	sll	tmp0, 4, tmp0; \
	and	reg, 0xFF, tmp1; \
	insbl	tmp1, ((port>>(BYTE_ENABLE_SHIFT-4))&3), tmp1; \
	stl_p	tmp1, 0x00(tmp0); \
	mb
/*
** Macro to read a byte from a specified port
*/
#define InPortByte(port,tmp0) \
	LDLI	(tmp0, port); \
	sll	tmp0, 4, tmp0; \
	ldl_p	tmp0, 0x00(tmp0); \
	srl	tmp0, (8*((port>>(BYTE_ENABLE_SHIFT-4))&3)), tmp0; \
	zap	tmp0, 0xfe, tmp0

/*
** Macro to acknowledge interrupts
**
** A typical SIO interrupt acknowledge sequence is as follows:
** The CPU generates an interrupt acknowledge cycle that is
** translated into a single PCI command and broadcast across
** the PCI bus to the SIO.  The SIO interrupt controller 
** translates this command into the two INTA# pulses expected
** by the 82C59A interrupt controller subsystem.
**
** On the first iAck cycle, the cascading priority is resolved 
** to detemine which of the two megacells will output the 
** interrupt vector onto the data bus. On the second iAck cycle, 
** the appropriate megacell drives the data bus with an 8-bit
** pointer to the correct interrupt vector for the highest priority 
** interrupt
**
**	Read port to initiate PCI iACK cycle
**
** INPUT PARAMETERS:
**
**	tmp0	scratch
**	tmp1	scratch
**
** OUTPUT PARAMETERS:
**
**	tmp0	interrupt vector
*/

#define IACK(tmp0,tmp1)	\
	InPortByte(I_ACK,tmp0)

