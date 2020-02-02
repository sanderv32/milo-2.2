        //
        // VID: [T1.2] PT: [Fri Apr  1 17:00:22 1994] SF: [platform.h]
        //  TI: [/sae_users/ericr/tools/vice -iplatform.s -l// -p# -DDC21064 -DEB64P -DPASS1 -h -m -aeb64p_p1 ]
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
**  $Id: platform.h,v 1.1.1.1 1995/08/01 17:33:54 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: platform.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:54  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:28:55  rusling
 * PALcode for the rev 1 EB64+.
 *
 * Revision 1.2  1995/03/28  09:20:49  rusling
 * updated to latest EB baselevel.
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
#define BC$V_SIZE	21
/*======================================================================*/
/*           DECchip 21064/21071 EVALUATION BOARD DEFINITIONS           */
/*======================================================================*/
/*
** EB64+ Address Space Partitioning
**
**	Address Bit
**   33 32 31 30 29 28          Description
**   -----------------		-----------
**    0  1  1  0  0  -		DECchip 21071-CA CSRs
**    0  1  1  0  1  0		DECchip 21071-DA CSRs
**    0  1  1  0  1  1		PCI Interrupt Ack
**    0  1  1  1  0  0		PCI Sparse I/O
**    0  1  1  1  1  -		PCI Configuration
**    1  0  -  -  -  -          PCI Sparse Memory
**    1  1  -  -  -  -          PCI Dense Memory
*/

/*
** The following definitions need to be shifted left 28 bits
** to obtain their respective reference points.
*/

#define IOB_BASE		0x1A    // I/O bridge base address
#define I_ACK_BASE		0x1B	// PCI Interrupt acknowledge
#define IO_BASE			0x1C	// PCI I/O base address
#define CFIG_BASE		0x1E	// PCI Configuration base address

#define BYTE_ENABLE_SHIFT	5

/*
** Interrupt Mask definitions:
**
** EB64+ specific IRQ pins are
**
**  IRQ0   IOB     ipl <= 3
**  IRQ1   RTC     ipl <= 4
**  IRQ2   NMI     ipl <= 6
**  IRQ3   SIO     ipl <= 2
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

#define INT$K_MASK      0x0008080C0E1E1E1E


/*
** SIO Configuration Register Definitions:
*/

#define SIO_CFIG 	(0x1000000 >> (BYTE_ENABLE_SHIFT))
#define SIO$B_PAC       0x41    // PCI Arbiter Control
#define SIO$B_IADCON    0x48    // ISA Address Decoder Control
#define SIO$B_IADRBE    0x49    // ISA Address Decoder ROM Block Enable
#define SIO$B_UBCSA     0x4E    // Utility Bus Chip Select A
#define SIO$B_UBCSB     0x4F    // Utility Bus Chip Select B

#define SIO_PAC         DefinePort(CFIG_BASE, SIO_CFIG+SIO$B_PAC)
#define SIO_IADCON      DefinePort(CFIG_BASE, SIO_CFIG+SIO$B_IADCON)
#define SIO_IADRBE      DefinePort(CFIG_BASE, SIO_CFIG+SIO$B_IADRBE)
#define SIO_UBCSA       DefinePort(CFIG_BASE, SIO_CFIG+SIO$B_UBCSA)
#define SIO_UBCSB       DefinePort(CFIG_BASE, SIO_CFIG+SIO$B_UBCSB)

/*
** PAC - PCI Arbiter Control Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	 <7:5>	  3	RAZ	Reserved
**	 <4:3>	  2	MRT	Master Retry Timer
**	   <2>	  1	BP	Bus Park Enable
**	   <1>	  1	BL	Bus Lock Select 
**	   <0>	  1	GAT	Guaranteed Access Time Enable
*/

#define SIO_PAC$V_GAT		0
#define SIO_PAC$M_GAT		(1<<SIO_PAC$V_GAT)
#define SIO_PAC$V_BP		2
#define SIO_PAC$M_BP		(1<<SIO_PAC$V_BP)

#define SIO_PAC$M_INIT  	(0)

/*
** IADCON - ISA Address Decoder Control Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	 <7:4>    4     TOP	Top of Memory
**	 <3:0>	  4	MEMC	ISA/DMA Memory Cycles to PCI Bus Enables
*/

#define	SIO_IADCON$V_TOP	4
#define SIO_IADCON$M_TOP	(0xF<<SIO_IADCON$V_TOP)
#define SIO_IADCON$V_MEMC	0
#define SIO_IADCON$M_MEMC	(0xF<<SIO_IADCON$V_MTP)

#define SIO_IADCON$M_INIT       (SIO_IADCON$M_TOP)

/*
** IADRBE - ISA Address Decoder ROM Block Enable Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	   <7>    1	RBE7	880-896K Memory Enable
**	   <6>    1	RBE6	864-880K Memory Enable
**	   <5>	  1	RBE5	848-864K Memory Enable
**	   <4>	  1	RBE4	832-848K Memory Enable
**	   <3>	  1	RBE3	816-832K Memory Enable
**	   <2>	  1	RBE2	800-816K Memory Enable
**	   <1>	  1	RBE1	784-800K Memory Enable
**	   <0>	  1 	RBE0	768-784K Memory Enable
*/

#define SIO_IADRBE$V_RBE7	7
#define SIO_IADRBE$M_RBE7	(1<<SIO_IADRBE$V_RBE7)
#define SIO_IADRBE$V_RBE6	6
#define SIO_IADRBE$M_RBE6	(1<<SIO_IADRBE$V_RBE6)
#define SIO_IADRBE$V_RBE5	5
#define SIO_IADRBE$M_RBE5	(1<<SIO_IADRBE$V_RBE5)
#define SIO_IADRBE$V_RBE4	4
#define SIO_IADRBE$M_RBE4	(1<<SIO_IADRBE$V_RBE4)
#define SIO_IADRBE$V_RBE3	3
#define SIO_IADRBE$M_RBE3	(1<<SIO_IADRBE$V_RBE3)
#define SIO_IADRBE$V_RBE2	2
#define SIO_IADRBE$M_RBE2	(1<<SIO_IADRBE$V_RBE2)
#define SIO_IADRBE$V_RBE1	1
#define SIO_IADRBE$M_RBE1	(1<<SIO_IADRBE$V_RBE1)
#define SIO_IADRBE$V_RBE0	0
#define SIO_IADRBE$M_RBE0	(1<<SIO_IADRBE$V_RBE0)

#define SIO_IADRBE$M_INIT       (SIO_IADRBE$M_RBE7 | \
				 SIO_IADRBE$M_RBE6 | \
				 SIO_IADRBE$M_RBE5 | \
				 SIO_IADRBE$M_RBE4 | \
				 SIO_IADRBE$M_RBE3 | \
				 SIO_IADRBE$M_RBE2 | \
				 SIO_IADRBE$M_RBE1 | \
				 SIO_IADRBE$M_RBE0)
/*
** UBCSA - Utility Bus Chip Select A Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	   <7>	  1	XBIOS	Extended BIOS Enable
**	   <6>	  1	LBIOS	Lower BIOS Enable
**	   <5>    1     ADDR    Primary/Secondary Address Range Select
**	   <4>	  1	IDE	IDE Decode Enable
**	 <3:2>    2	FDA	Floppy Disk Address Locations Enable
**	   <1>	  1	KBA	Keyboard Controller Address Location Enable
**	   <0>	  1	RTC	RTC Address Location Enable
*/

#define SIO_UBCSA$V_RTCALE      0
#define SIO_UBCSA$M_RTCALE      (1<<SIO_UBCSA$V_RTCALE)
#define SIO_UBCSA$V_KBCALE      1
#define SIO_UBCSA$M_KBCALE      (1<<SIO_UBCSA$V_KBCALE)
#define SIO_UBCSA$V_FDALE       2
#define SIO_UBCSA$M_FDALE_3F0   (0x2<<SIO_UBCSA$V_FDALE)
#define SIO_UBCSA$M_FDALE_3F2   (0x1<<SIO_UBCSA$V_FDALE)
#define SIO_UBCSA$M_FDALE_370   (0xA<<SIO_UBCSA$V_FDALE)
#define SIO_UBCSA$M_FDALE_372   (0x9<<SIO_UBCSA$V_FDALE)
#define SIO_UBCSA$V_IDEDE       4
#define SIO_UBCSA$M_IDEDE       (1<<SIO_UBCSA$V_IDEDE)
#define SIO_UBCSA$V_LBIOSE      6
#define SIO_UBCSA$M_LBIOSE      (1<<SIO_UBCSA$V_LBIOSE)
#define SIO_UBCSA$V_XBIOSE      7
#define SIO_UBCSA$M_XBIOSE      (1<<SIO_UBCSA$V_XBIOSE)

#define SIO_UBCSA$M_INIT        (SIO_UBCSA$M_XBIOSE | \
				 SIO_UBCSA$M_LBIOSE | \
                                 SIO_UBCSA$M_KBCALE | \
				 SIO_UBCSA$M_RTCALE)
/*
** UBCSB - Utility Bus Chip Select B Register
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	   <7>	  1	CRAM	Configuration RAM Decode Enable
**	   <6>	  1	PORT92	Port 92 Enable
**	 <5:4>	  2	PAR	Parallel Port Enable
**	 <3:2>	  2	SERB	Serial Port B Enable
**	 <1:0>	  2	SERA	Serial Port A Enalbe
*/

#define SIO_UBCSB$V_SERA        0
#define SIO_UBCSB$M_SERA_COM1   (0<<SIO_UBCSB$V_SERA)
#define SIO_UBCSB$M_SERA_COM2   (1<<SIO_UBCSB$V_SERA)
#define SIO_UBCSB$M_SERA_DIS    (3<<SIO_UBCSB$V_SERA)
#define SIO_UBCSB$V_SERB        2
#define SIO_UBCSB$M_SERB_COM1   (0<<SIO_UBCSB$V_SERB)
#define SIO_UBCSB$M_SERB_COM2   (1<<SIO_UBCSB$V_SERB)
#define SIO_UBCSB$M_SERB_DIS    (3<<SIO_UBCSB$V_SERB)
#define SIO_UBCSB$V_PAR         4
#define SIO_UBCSB$M_PAR_LPT1    (0<<SIO_UBCSB$V_PAR)
#define SIO_UBCSB$M_PAR_LPT2    (1<<SIO_UBCSB$V_PAR)
#define SIO_UBCSB$M_PAR_LPT3    (2<<SIO_UBCSB$V_PAR)
#define SIO_UBCSB$M_PAR_DIS     (3<<SIO_UBCSB$V_PAR)
#define SIO_UBCSB$V_PORT92      6
#define SIO_UBCSB$M_PORT92      (1<<SIO_UBCSB$V_PORT92)
#define SIO_UBCSB$V_CRAMDE      7
#define SIO_UBCSB$M_CRAMDE      (1<<SIO_UBCSB$V_CRAMDE)

#define SIO_UBCSB$M_INIT        (SIO_UBCSB$M_PAR_DIS  | \
				 SIO_UBCSB$M_SERB_DIS | \
                                 SIO_UBCSB$M_SERA_DIS)
/*
** SIO DMA Register Definitions:
*/

#define SIO$B_DCM	0xD6	// DMA Channel Mode

#define SIO_DCM 	DefinePort(IO_BASE, SIO$B_DCM)

/*
** SIO Control Register Definitions:
*/

#define SIO$B_NMISC	0x61	// NMI Status and Control
#define SIO$B_NMI	0x70	// NMI Enable

#define SIO_NMISC       DefinePort(IO_BASE, SIO$B_NMISC)
#define SIO_NMI         DefinePort(IO_BASE, SIO$B_NMI)

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

#define SIO_NMISC$V_SERR	7
#define SIO_NMISC$M_SERR	(1<<SIO_NMISC$V_SERR)
#define SIO_NMISC$V_IOCHK	6
#define SIO_NMISC$M_IOCHK	(1<<SIO_NMISC$V_IOCHK)
#define SIO_NMISC$V_IOCHK_EN    3
#define SIO_NMISC$M_IOCHK_EN	(1<<SIO_NMISC$V_IOCHK_EN)
#define SIO_NMISC$V_SERR_EN	2
#define SIO_NMISC$M_SERR_EN	(1<<SIO_NMISC$V_SERR_EN)

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

