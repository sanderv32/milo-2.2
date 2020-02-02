        //
        // VID: [T1.2] PT: [Fri Apr  1 17:00:05 1994] SF: [platform.h]
        //  TI: [/sae_users/ericr/tools/vice -iplatform.s -l// -p# -DDC21064 -DEB64 -h -m -aeb64 ]
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
**  $Id: platform.h,v 1.1.1.1 1995/08/01 17:33:53 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: platform.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:53  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:27:40  rusling
 * EB64 PALcode sources.
 *
 * Revision 1.2  1995/03/28  09:15:27  rusling
 * Updated to latest EB baselevel.
 *
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
** Console Service (cserve) sub-function codes:
*/
#define CSERVE$K_LDQP           0x01
#define CSERVE$K_STQP           0x02
#define CSERVE$K_RD_ABOX        0x03
#define CSERVE$K_RD_BIU         0x04
#define CSERVE$K_RD_ICCSR       0x05
#define CSERVE$K_WR_ABOX        0x06
#define CSERVE$K_WR_BIU         0x07
#define CSERVE$K_WR_ICCSR       0x08
#define CSERVE$K_JTOPAL         0x09
#define CSERVE$K_WR_INT         0x0A
#define CSERVE$K_RD_IMPURE      0x0B
#define CSERVE$K_PUTC           0x0F

/*
**  External cache size is 2 to the power of BC$V_SIZE 
**  (used in call_pal cflush)
*/
#define BC$V_SIZE	19
/*======================================================================*/
/*              DECchip 21064 EVALUATION BOARD DEFINITIONS              */
/*======================================================================*/
/*
** EB64 Address Space Partitioning
**
**	Address Bit
**	33 32 31 30             Description
**	-----------		-----------
**	 1  0  0  0		Debug ROM/System Register
**	 1  0  0  1		Interrupt ACK
**	 1  0  1  0		VL82C486 Memory
**	 1  0  1  1		VL82C486 I/O
**	 1  1  -  -		Expansion Connector
*/

/*
** The following definitions need to be shifted left 28 bits
** to obtain their respective reference points.
*/

#define I_ACK_BASE		0x24	// Interrupt acknowledge
#define IO_BASE			0x2C	// I/O base address

#define BYTE_ENABLE_SHIFT	7

/*
** Interrupt Mask definitions:
**
** EB64 specific IRQ pins are
**
**  IRQ0   PIC     ipl <= 2
**  IRQ1   NMI     disabled
**  IRQ2   RTC     ipl <= 4
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

#define INT$K_MASK      0x00000008087A7A7A


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
** VLSI VL82C113 Real-Time Clock (RTC) Defintions:
*/

#define RTCADD     	DefinePort(IO_BASE,0x170) // RTC address register
#define RTCDAT     	DefinePort(IO_BASE,0x171) // RTC data register

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

#define DLA$K_BRG		12	// Baud rate divisor = 9600

#define LSR$V_THRE		5	// Xmit holding register empty bit

#define LCR$M_WLS		3	// Word length select mask
#define LCR$M_STB		4	// Number of stop bits mask
#define LCR$M_PEN		8	// Parity enable mask
#define LCR$M_DLAB		128	// Divisor latch access bit mask

#define LCR$K_INIT	      	(LCR$M_WLS | LCR$M_STB)

#define MCR$M_DTR		1	// Data terminal ready mask
#define MCR$M_RTS		2	// Request to send mask
#define MCR$M_OUT1		4	// Output 1 control mask
#define MCR$M_OUT2		8	// UART interrupt mask enable

#define MCR$K_INIT	      	(MCR$M_DTR  | \
				 MCR$M_RTS  | \
				 MCR$M_OUT1 | \
				 MCR$M_OUT2)

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
** A typical 82C59A interrupt acknowledge sequence is as follows:
** Any unmasked interrupt generates an interrupt request to the
** CPU. The interrupt controller megacells then respond to a
** series of interrupt acknowledge pulses from the CPU. On the
** first iAck cycle, the cascading priority is resolved to detemine
** which of the two megacells will output the interrupt vector
** onto the data bus. On the second iAck cycle, the appropriate
** megacell drives the data bus with an 8-bit pointer to the
** correct interrupt vector for the highest priority interrupt
**
**	Read port to initiate 1st iACK cycle
**	Wait 100 cycles
**	Read port to initiate 2nd iACK cycle
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

#define IACK(tmp0,tmp1) \
	InPortByte(I_ACK,tmp0); \
	rpcc	tmp1; \
	addl	tmp1, zero, tmp1; \
0:	rpcc	tmp0; \
	addl	tmp0, zero, tmp0; \
	subq	tmp0, tmp1, tmp0; \
	cmplt	tmp0, 100, tmp0; \
	blbs	tmp0, 0b; \
	InPortByte(I_ACK,tmp0)

