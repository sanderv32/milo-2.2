        //
        // VID: [T1.2] PT: [Fri Apr  1 17:00:55 1994] SF: [dc21064.h]
        //  TI: [/sae_users/ericr/tools/vice -iplatform.s -l// -p# -DDC21066 -DEB66 -DPASS1 -h -m -aeb66_p1 ]
        //
#define	__DC21064_LOADED	1
/*
*****************************************************************************
**                                                                          *
**  Copyright © 1993, 1994						    *
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
**	dc21064.h
**
**  MODULE DESCRIPTION:
**
**      DECchip 21064/21066 specific definitions
**
**  AUTHOR: ER
**
**  CREATION DATE:  29-Oct-1992
**
**  $Id: dc21064.h,v 1.1.1.1 1995/08/01 17:33:55 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: dc21064.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:55  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:29:37  rusling
 * PALcode sources for the rev 1 EB66 system.
 *
 * Revision 1.2  1995/03/28  09:22:37  rusling
 * Updated to latest EB baselevel.
 *
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.6  1994/03/29  20:53:23  ericr
**  Fixed comments for ESR mask definitions NOT_CEE and ERR_NOT_CEE
**
**  Revision 1.5  1994/03/29  19:32:22  ericr
**  Fixed IOC_STAT0<ERR> bit definition and mask.
**
**  Revision 1.4  1994/03/14  20:45:58  ericr
**  Added FPR definitions
**
**  Revision 1.3  1994/03/14  16:38:44  samberg
**  Use LOAD_REGION_BASE macro instead of individual load_x_csr macros
**
**  Revision 1.2  1994/03/08  20:27:52  ericr
**  Replaced DEBUG_MONITOR conditional with KDEBUG
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
**
*/

/*======================================================================*/
/*                INTERNAL PROCESSOR REGISTER DEFINITIONS		*/
/*======================================================================*/

#define IPR$V_PAL	7
#define IPR$M_PAL	(1<<IPR$V_PAL)
#define IPR$V_ABX	6
#define IPR$M_ABX	(1<<IPR$V_ABX)
#define IPR$V_IBX	5
#define IPR$M_IBX	(1<<IPR$V_IBX)
#define IPR$V_INDEX	0
#define IPR$M_INDEX	(0x1F<<IPR$V_INDEX)

/*
**  Ibox IPR Definitions
*/

#define tbTag		IPR$M_IBX + 0x0
#define itbPte		IPR$M_IBX + 0x1
#define iccsr		IPR$M_IBX + 0x2
#define itbPteTemp	IPR$M_IBX + 0x3
#define excAddr		IPR$M_IBX + 0x4
#define slRcv		IPR$M_IBX + 0x5
#define itbZap		IPR$M_IBX + 0x6
#define itbAsm		IPR$M_IBX + 0x7
#define itbIs		IPR$M_IBX + 0x8
#define ps		IPR$M_IBX + 0x9
#define excSum		IPR$M_IBX + 0xA
#define palBase		IPR$M_IBX + 0xB
#define hirr		IPR$M_IBX + 0xC
#define sirr		IPR$M_IBX + 0xD
#define astrr		IPR$M_IBX + 0xE
#define hier		IPR$M_IBX + 0x10
#define sier		IPR$M_IBX + 0x11
#define aster		IPR$M_IBX + 0x12
#define slClr		IPR$M_IBX + 0x13
#define slXmit		IPR$M_IBX + 0x16

/*
**  Instruction Cache Control and Status Register (ICCSR) Bit Summary
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	   <45>	  1	PME1	Performance Monitor Enable 1
**	   <44>	  1	PME0	Performance Monitor Enable 0
**	   <42>	  1	FPE	Floating Point Enable
**	   <41>	  1	MAP	I-stream superpage mapping enable
**	   <40>	  1	HWE	Allow PALRES to be issued in kernel mode
**	   <39>	  1	DI	Dual Issue enable
**	   <38>	  1	BHE	Branch History Enable
**	   <37>	  1	JSE	JSR Stack Enable
**	   <36>	  1	BPE	Branch Prediction Enable
**	   <35>	  1	PIPE	Pipeline enable
**	<34:32>	  3	MUX1	Performance Counter 1 Select
**	<11:08>	  3	MUX0	Performance Counter 0 Select
**	    <3>   1	PC0	Performance Counter 0 Interrupt Enable
**	    <0>	  1	PC1	Performance Counter 1 Interrupt Enable
*/

#define ICCSR$V_PME1	    45
#define ICCSR$M_PME1	    (1<<(ICCSR$V_PME1-32))
#define ICCSR$V_PME0	    44
#define ICCSR$M_PME0	    (1<<(ICCSR$V_PME0-32))
#define ICCSR$V_FPE	    42
#define ICCSR$M_FPE	    (1<<(ICCSR$V_FPE-32))
#define ICCSR$V_MAP	    41
#define ICCSR$M_MAP	    (1<<(ICCSR$V_MAP-32))
#define ICCSR$V_HWE	    40
#define ICCSR$M_HWE	    (1<<(ICCSR$V_HWE-32))
#define ICCSR$V_DI	    39
#define ICCSR$M_DI	    (1<<(ICCSR$V_DI-32))
#define ICCSR$V_BHE	    38
#define ICCSR$M_BHE	    (1<<(ICCSR$V_BHE-32))
#define ICCSR$V_JSE	    37
#define ICCSR$M_JSE	    (1<<(ICCSR$V_JSE-32))
#define ICCSR$V_BPE	    36
#define ICCSR$M_BPE	    (1<<(ICCSR$V_BPE-32))
#define ICCSR$V_PIPE	    35
#define ICCSR$M_PIPE	    (1<<(ICCSR$V_PIPE-32))
#define ICCSR$V_MUX1	    32
#define ICCSR$M_MUX1	    (7<<(ICCSR$V_MUX1-32))
#define ICCSR$V_MUX0	    8
#define ICCSR$M_MUX0	    (0xF<<ICCSR$V_MUX0)
#define ICCSR$V_PC0	    3
#define ICCSR$M_PC0	    (1<<ICCSR$V_PC0)
#define ICCSR$V_PC1	    0
#define ICCSR$M_PC1	    (1<<ICCSR$V_PC1)

/*
**  Exception Summary Register (EXC_SUM) Bit Summary
**
**	 Loc	Size	Name	Function
**	-----	----	----	------------------------------------
**	 <33>	 1	MSK	Exception Register Write Mask window
**	  <8>	 1	IOV	Integer overflow
**	  <7>	 1	INE	Inexact result
**	  <6>	 1	UNF	Underflow
**	  <5>	 1	OVF	Overflow
**	  <4>	 1	DZE	Division by zero
**	  <3>	 1	INV	Invalid operation
**	  <2>	 1	SWC	Software completion
*/

#define EXC$V_MSK	33
#define EXC$M_MSK	(1<<(EXC$V_MSK-32))
#define EXC$V_IOV	8
#define EXC$M_IOV	(1<<EXC$V_IOV)
#define EXC$V_INE	7
#define EXC$M_INE	(1<<EXC$V_INE)
#define EXC$V_UNF	6
#define EXC$M_UNF	(1<<EXC$V_UNF)
#define EXC$V_OVF	5
#define EXC$M_OVF	(1<<EXC$V_OVF)
#define EXC$V_DZE	4
#define	EXC$M_DZE	(1<<EXC$V_DZE)
#define EXC$V_INV	3
#define EXC$M_INV	(1<<EXC$V_INV)
#define	EXC$V_SWC	2
#define EXC$M_SWC	(1<<EXC$V_SWC)

/*
**  Hardware Interrupt Request Register (HIRR) Bit Summary
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	  <13>	  1	SLR	Serial Line interrupt
**     <12:10>    3     IRQ     Corresponds to pins Irq_h[2:0]
**	   <9>	  1	PC0	Performance Counter 0 interrupt
**	   <8>	  1	PC1	Performance Counter 1 interrupt
**         <6>    1     MERR    Memory error interrupt
**         <5>	  1	IERR    IOC error interrupt
**	   <4>	  1	CRR	Correctable read data interrupt
**	   <3>	  1	ATR	AST interrupt
**	   <2>	  1	SWR	Software interrupt
**	   <1>	  1	HWR	Hardware interrupt
*/
#define HIRR$V_SLR	13
#define HIRR$M_SLR	(1<<HIRR$V_SLR)

#define HIRR$V_IRQ2	12
#define HIRR$M_IRQ2	(1<<HIRR$V_IRQ2)
#define HIRR$V_IRQ1	11
#define HIRR$M_IRQ1	(1<<HIRR$V_IRQ1)
#define HIRR$V_IRQ0	10
#define HIRR$M_IRQ0	(1<<HIRR$V_IRQ0)

#define HIRR$V_PC0	9
#define HIRR$M_PC0	(1<<HIRR$V_PC0)
#define HIRR$V_PC1	8
#define HIRR$M_PC1	(1<<HIRR$V_PC1)

#define HIRR$V_MERR	6
#define HIRR$M_MERR	(1<<HIRR$V_MERR)
#define HIRR$V_IERR	5
#define HIRR$M_IERR	(1<<HIRR$V_IERR)
#define HIRR$V_CRR	4
#define HIRR$M_CRR	(1<<HIRR$V_CRR)
#define HIRR$V_ATR	3
#define HIRR$M_ATR	(1<<HIRR$V_ATR)
#define HIRR$V_SWR	2
#define HIRR$M_SWR	(1<<HIRR$V_SWR)
#define HIRR$V_HWR	1
#define HIRR$M_HWR	(1<<HIRR$V_HWR)

/*
**  Hardware Interrupt Enable Register (HIER) Write Bit Summary
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	  <32>	  1	SLE	Serial Line interrupt enable
**	  <15>	  1	PC1	Performance Counter 1 interrupt enable
**	<14:9>	  6	HIER	Interrupt enables for Irq_h<5:0>
**	   <8>	  1	PC0	Performance Counter 0 interrupt enable
**	   <2>	  1	CRE	Correctable read data interrupt enable
*/

#define HIERW$V_SLE	32
#define HIERW$V_PC1	15
#define HIERW$V_PC0	8
#define HIERW$V_CRE	2
#define HIERW$M_CRE     (1<<HIERW$V_CRE)

#define HIERR$V_CRE	4
#define HIERR$M_CRE     (1<<HIERR$V_CRE)

/*
**  Clear Serial Line Interrupt Register (SL_CLR) Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	   <32>	  1	SLC	    W0C -- Clear serial line int request
**	   <15>	  1	PC1	    W0C -- Clear PC1 interrupt request
**	    <8>	  1	PC0	    W0C -- Clear PC0 interrupt request
**	    <2>	  1	CRD	    W0C -- Clear CRD interrupt request
*/

#define SL_CLR$V_SLC	    32
#define SL_CLR$V_PC1	    15
#define SL_CLR$V_PC0	    8
#define SL_CLR$V_CRD	    2

/*
**  Abox IPR Definitions
*/

#define dtbCtl		IPR$M_ABX + 0x0
#define tbCtl		IPR$M_ABX + 0x0
#define dtbPte		IPR$M_ABX + 0x2
#define dtbPteTemp	IPR$M_ABX + 0x3
#define mmcsr		IPR$M_ABX + 0x4
#define va		IPR$M_ABX + 0x5
#define dtbZap		IPR$M_ABX + 0x6
#define dtbAsm		IPR$M_ABX + 0x7
#define dtbIs		IPR$M_ABX + 0x8

#define dcAddr		IPR$M_ABX + 0xB
#define dcStat		IPR$M_ABX + 0xC

#define aboxCtl		IPR$M_ABX + 0xE
#define altMode		IPR$M_ABX + 0xF
#define cc		IPR$M_ABX + 0x10
#define ccCtl		IPR$M_ABX + 0x11

#define flushIc		IPR$M_ABX + 0x15
#define flushIcAsm	IPR$M_ABX + 0x17
#define xtbZap		IPR$M_ABX + IPR$M_IBX + 0x6
#define xtbAsm		IPR$M_ABX + IPR$M_IBX + 0x7

/*
**  Memory Management Control and Status Register (MM_CSR) Bit Summary
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	<14:9>	  6	OPC	Opcode of faulting instruction
**	 <8:4>	  5	RA	Ra field of faulting instruction
**	   <3>	  1	FOW	Fault on write
**	   <2>	  1	FOR	Fault on read
**	   <1>	  1	ACV	Access violation
**	   <0>	  1	WR	Faulting reference is a write
*/

#define	MMCSR$V_OPC	9
#define MMCSR$M_OPC	(0x7E<<MMCSR$V_OPC)
#define MMCSR$V_RA	4
#define MMCSR$M_RA	(0x1F<<MMCSR$V_RA)
#define MMCSR$V_FOW	3
#define MMCSR$M_FOW	(1<<MMCSR$V_FOW)
#define MMCSR$V_FOR	2
#define MMCSR$M_FOR	(1<<MMCSR$V_FOR)
#define MMCSR$V_ACV	1
#define MMCSR$M_ACV	(1<<MMCSR$V_ACV)
#define MMCSR$V_WR	0
#define MMCSR$M_WR	(1<<MMCSR$V_WR)

#define MMCSR$M_FAULT	0x000E

/*
** Abox Control Register (ABOX_CTL) Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	  <14>	  1	NOCHK_PAR   EV45 - Set to disable checking of 
**				    primary cache parity
**	  <13>    1	F_TAG_ERR   EV45 - Set to generate bad primary 
**				    cache tag parity
**	  <12>    1	DC_16K	    EV45 - Set to select 16KB cache
**	  <11>	  1	DC_FHIT	    Dcache Force Hit
**	  <10>	  1	DC_ENA	    Dcache Enable
**	   <6>	  1	EMD_EN	    Limited big endian support enable
**	   <5>	  1	SPE_2	    D-stream superpage 1 enable
**	   <4>	  1	SPE_1	    D-stream superpage 2 enable
**	   <3>	  1	IC_SBUF_EN  Icache Stream Buffer Enable
**	   <2>	  1	CRD_EN	    Corrected Read Data Enable
**	   <1>	  1	MCHK_EN	    Machine Check Enable
**	   <0>	  1	WB_DIS	    Write Buffer unload Disable
*/

#define ABOX$V_NOCHK_PAR    14
#define ABOX$M_NOCHK_PAR    (1<<ABOX$V_NOCHK_PAR)
#define ABOX$V_F_TAG_ERR    13
#define ABOX$M_F_TAG_ERR    (1<<ABOX$V_F_TAG_ERR)
#define ABOX$V_DC_16K	    12
#define ABOX$M_DC_16K	    (1<<ABOX$V_DC_16K)
#define ABOX$V_DC_FHIT	    11
#define ABOX$M_DC_FHIT	    (1<<ABOX$V_DC_FHIT)
#define ABOX$V_DC_ENA	    10
#define	ABOX$M_DC_ENA	    (1<<ABOX$V_DC_ENA)
#define ABOX$V_EMD_EN	    6
#define ABOX$M_EMD_EN	    (1<<ABOX$V_EMD_EN)
#define ABOX$V_SPE_2	    5
#define ABOX$M_SPE_2	    (1<<ABOX$V_SPE_2)
#define ABOX$V_SPE_1	    4
#define ABOX$M_SPE_1	    (1<<ABOX$V_SPE_1)
#define ABOX$V_IC_SBUF_EN   3
#define ABOX$M_IC_SBUF_EN   (1<<ABOX$V_IC_SBUF_EN)
#define ABOX$V_CRD_EN	    2
#define ABOX$M_CRD_EN	    (1<<ABOX$V_CRD_EN)
#define ABOX$V_MCHK_EN	    1
#define ABOX$M_MCHK_EN	    (1<<ABOX$V_MCHK_EN)
#define ABOX$V_WB_DIS	    0
#define	ABOX$M_WB_DIS	    (1<<ABOX$V_WB_DIS)


/*======================================================================*/
/*                DC21066 MEMORY MAPPED CSR DEFINITIONS			*/
/*======================================================================*/

/*
**  Macro to create the base of a region of physical address space
**  The inputs are:
**      reg     the register to which the physical base address is written
**      base    base<5:0> to be shifted left 28 bits into <33:28>
*/ 

#define LOAD_REGION_BASE(reg,base) \
  lda   reg, base(r31); \
  sll   reg, 28, reg

/*
**  Memory Controller (MEM_CTL) Register Definitions
*/

#define MEM_CSR_BASE 0x12       /* Bits <33:28> of physical address base */

/* 
** Offsets to Memory Controller CSRs
*/

#define	bcr0	0x0	/* Bank Configuration 0 */
#define	bcr1	0x8	/* Bank Configuration 1 */
#define	bcr2	0x10	/* Bank Configuration 2 */
#define	bcr3	0x18	/* Bank Configuration 3 */
#define	bmr0	0x20	/* Bank Mask 0 */
#define	bmr1	0x28	/* Bank Mask 1 */
#define	bmr2	0x30	/* Bank Mask 2 */
#define	bmr3	0x38	/* Bank Mask 3 */
#define	btr0	0x40	/* Bank Timing 0 */
#define	btr1	0x48	/* Bank Timing 1 */
#define	btr2	0x50	/* Bank Timing 2 */
#define	btr3	0x58	/* Bank Timing 3 */
#define gtr	0x60	/* Global Timing */
#define esr	0x68	/* Error Status */
#define ear	0x70	/* Error Address */
#define car	0x78	/* Backup Cache Control */
#define vgr	0x80	/* Video and Graphics Control */
#define plm	0x88	/* Plane Mask */
#define for	0x90	/* Foreground */

/* 
** Bank Configuration Register (BCR) Bit Summary
**
**	  Loc	Size	Name    Function
**	 -----	----	----	---------------------------------
**     <28:20>    9	BASE	Bank Base Address
**	  <14>	  1	BAV	Base Address Valid
**	  <13>	  1	SBE	Split (dual) Bank Enable
**	  <12>	  1	BWE	Byte Write Enable
**	  <11>	  1	WRM	Write Mode
**        <10>    1     ERM	Error Mode
**	 <9:6>	  4	RAS	Row Address Select
*/

#define bcr$v_ras   	6
#define bcr$m_ras	(0xF<<bcr$v_ras)
#define bcr$v_erm   	10
#define bcr$m_erm	(1<<bcr$v_erm)
#define bcr$v_wrm   	11
#define bcr$m_wrm	(1<<bcr$v_wrm)
#define bcr$v_bwe   	12
#define bcr$m_bwe	(1<<bcr$v_bwe)
#define bcr$v_sbe   	13
#define bcr$m_sbe	(1<<bcr$v_sbe)
#define bcr$v_bav   	14
#define bcr$m_bav	(1<<bcr$v_bav)
#define bcr$v_base  	20

/*
** Bank Address Mask Register (BMR) Bit Summary
**
**	  Loc	Size	Name    Function
**	 -----	----	----	---------------------------------
**     <28:20>    9	MASK	Bank Address Mask
*/

#define bmr$v_mask  20

/* 
** Cache Register (CAR) Bit Summary
**
**	  Loc	Size	Name    Function
**	 -----	----	----	---------------------------------
**	  <31>	  1	HIT	Backup Cache Hit
**	  <15>	  1	PWR	Power Saving
**        <14>    1	WHD	Write Hold Time
**     <13:11>	  3	WRS	Backup Cache Write Speed
**     <10:08>	  3	RDS	Backup Cache Read Speed
**     <07:05>    3	SIZE	Backup Cache Size
**	   <4>    1	ECE	Backup Cache ECC
**	   <3>	  1	WWP	Write Wrong Tag Parity
**	   <2>	  1	ETP	Enable Tag Parity
**	   <0>	  1	BCE	Backup Cache Enable               
*/

#define CAR$V_BCE	0
#define CAR$M_BCE	(1<<CAR$V_BCE)
#define CAR$V_ETP   	2
#define CAR$M_ETP	(1<<CAR$V_ETP)
#define CAR$V_WWP   	3
#define CAR$M_WWP	(1<<CAR$V_WWP)
#define CAR$V_ECE   	4
#define CAR$M_ECE	(1<<CAR$V_ECE)
#define CAR$V_SIZE  	5
#define CAR$M_SIZE	(7<<CAR$V_SIZE)
#define CAR$V_RDS   	8
#define CAR$M_RDS	(7<<CAR$V_RDS)
#define CAR$V_WRS  	11
#define CAR$M_WRS	(7<<CAR$V_WRS)
#define CAR$V_WHD  	14
#define CAR$M_WHD	(1<<CAR$V_WHD)
#define CAR$V_PWR	15
#define CAR$M_PWR	(1<<CAR$V_PWR)
#define CAR$V_HIT	31
#define CAR$M_HIT	(1<<CAR$V_HIT)

/*
**  Error Status Register (ESR) Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	   <12>	  1	NXM   	    Non-existant memory address
**	   <11>	  1	ICE         Ignore corrected errors
**	   <10>	  1	MHE    	    Multiple hard errors
**	    <9>   1	MSE         Multiple soft errors
**	    <7>	  1	CTE         Cache tag parity error
**	    <4>	  1	SOR         Error source (0=cache, 1=DRAM)
**	    <3>	  1	WRE         Error access type (0=read, 1=write)
**	    <2>	  1	UEE	    Uncorrectable ECC error
**	    <1>	  1	CEE	    Correctable ECC error
**	    <0>	  1	EAV	    Error address valid
*/

#define	ESR$V_NXM	    12
#define ESR$M_NXM	    (1<<ESR$V_NXM)
#define ESR$V_ICE      	    11
#define ESR$M_ICE      	    (1<<ESR$V_ICE)
#define ESR$V_MHE    	    10
#define ESR$M_MHE      	    (1<<ESR$V_MHE)
#define ESR$V_MSE      	    9 
#define ESR$M_MSE     	    (1<<ESR$V_MSE)
#define ESR$V_CTE      	    7 
#define ESR$M_CTE           (1<<ESR$V_CTE)
#define ESR$V_SOR           4
#define ESR$M_SOR      	    (1<<ESR$V_SOR)
#define ESR$V_WRE      	    3
#define ESR$M_WRE      	    (1<<ESR$V_WRE)
#define ESR$V_UEE    	    2
#define ESR$M_UEE    	    (1<<ESR$V_UEE)
#define ESR$V_CEE	    1
#define ESR$M_CEE	    (1<<ESR$V_CEE)
#define ESR$V_EAV	    0
#define ESR$M_EAV	    (1<<ESR$V_EAV)

/* 
** Mask to set all the write-1-to-clear bits
*/

#define ESR$M_INIT	    ESR$M_CEE | \
                            ESR$M_UEE | \
                            ESR$M_CTE | \
                            ESR$M_NXM | \
                            ESR$M_MSE | \
                            ESR$M_MHE | \
                            ESR$M_ICE
/* 
** Mask to set all the write-1-to-clear bits except cee and mse. 
*/

#define ESR$M_ERR_NOT_CEE   ESR$M_UEE | \
                            ESR$M_CTE | \
                            ESR$M_NXM | \
                            ESR$M_MHE
/* 
** Mask to set all the write-1-to-clear bits except cee and mse, 
** plus other bits which could be non-zero.
*/

#define ESR$M_NOT_CEE       ESR$M_UEE | \
                            ESR$M_CTE | \
                            ESR$M_NXM | \
                            ESR$M_MHE | \
                            ESR$M_EAV | \
                            ESR$M_WRE | \
                            ESR$M_SOR | \
                            ESR$M_ICE
/*
**  Error Address Register (EAR)
*/

#define ear_max_bit       29


/*
** I/O Controller (IOC) Register Definitions
*/

#define IOC_CSR_BASE	0x18    /* Bits <33:28> of physical address base */

/* 
** Offsets to I/O controller CSRs 
*/

#define	ioc_hae		0x0	/* Host address extension */
#define	ioc_conf	0x20	/* Configuration cycle type */
#define	ioc_stat0	0x40	/* Error status */
#define	ioc_stat1	0x60	/* Error address */
#define	ioc_tbia	0x80	/* Scatter gather TB invalidate */
#define	ioc_tben	0xa0	/* Scatter gather TB enable */
#define	ioc_pci_rst	0xc0	/* PCI reset */
#define	ioc_w_base0	0x100	/* Window Base 0*/
#define	ioc_w_base1	0x120	/* Window Base 1*/
#define	ioc_w_mask0	0x140	/* Window Mask 0*/
#define	ioc_w_mask1	0x160	/* Window Mask 1*/
#define	ioc_t_base0	0x180	/* Translated Base 0*/
#define	ioc_t_base1	0x1a0	/* Translated Base 1*/

/*
**  Error Status Register (ESR) Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	<31:13>	 19 	NBR   	    Error address
**	 <10:8>	  3	CODE	    Error type
**	    <7>	  1	TREF	    Target window reference indicator
**	    <6>	  1	THIT	    TB hit indicator
**	    <5>	  1	LOST	    Lost error
**	    <4>	  1	ERR         Error status valid
**	  <3:0>	  4	CMD	    PCI command field of error cycle
*/

#define	IOC$V_NBR	    13
#define IOC$V_CODE          8
#define IOC$M_CODE          (7<<IOC$V_CODE)
#define IOC$V_TREF    	    7
#define IOC$M_TREF	    (1<<IOC$V_TREF)
#define IOC$V_THIT	    6
#define IOC$M_THIT     	    (1<<IOC$V_THIT)
#define IOC$V_LOST	    5 
#define IOC$M_LOST          (1<<IOC$V_LOST)
#define IOC$V_ERR      	    4 
#define IOC$M_ERR           (1<<IOC$V_ERR)
#define IOC$V_CMD           0
#define IOC$M_CMD      	    (0xF<<IOC$V_CMD)
               
/* 
** Mask to set all the write-1-to-clear bits 
*/

#define IOC$M_INIT	    (IOC$M_LOST | IOC$M_ERR)

#define IOC$V_HAE	    27

/*
**  Window Base Register Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	   <33>	  1	WEN	    Window Enable
**	   <32>	  1	SG	    Scatter Gather Enable 
**	<31:20>	 19 	WBASE	    Window Base
*/

#define	IOC$V_WEN	    33

/*
**  PCI Reset Bit Summary
**
**	  Loc	Size	Name	    Function
**	 -----	----	----	    ---------------------------------
**	    <6>	  1	PCI_RST	    PCI Reset Bit
*/

#define	IOC$V_PCI_RST	    6
#define IOC$M_PCI_RST       0x4

/*======================================================================*/
/*                 GENERAL PURPOSE REGISTER DEFINITIONS			*/
/*======================================================================*/

#define	r0		$0
#define r1		$1
#define r2		$2
#define r3		$3
#define r4		$4
#define r5		$5
#define r6		$6
#define r7		$7
#define r8		$8
#define r9		$9
#define r10		$10
#define r11		$11
#define r12		$12
#define r13		$13
#define r14		$14
#define	r15		$15
#define	r16		$16
#define	r17		$17
#define	r18		$18
#define	r19		$19
#define	r20		$20
#define	r21		$21
#define r22		$22
#define r23		$23
#define r24		$24
#define r25		$25
#define r26		$26
#define r27		$27
#define r28		$28
#define r29		$29
#define r30		$30
#define r31		$31

/*======================================================================*/
/*                 FLOATING POINT REGISTER DEFINITIONS			*/
/*======================================================================*/

#define	f0		$f0
#define f1		$f1
#define f2		$f2
#define f3		$f3
#define f4		$f4
#define f5		$f5
#define f6		$f6
#define f7		$f7
#define f8		$f8
#define f9		$f9
#define f10		$f10
#define f11		$f11
#define f12		$f12
#define f13		$f13
#define f14		$f14
#define	f15		$f15
#define	f16		$f16
#define	f17		$f17
#define	f18		$f18
#define	f19		$f19
#define	f20		$f20
#define	f21		$f21
#define f22		$f22
#define f23		$f23
#define f24		$f24
#define f25		$f25
#define f26		$f26
#define f27		$f27
#define f28		$f28
#define f29		$f29
#define f30		$f30
#define f31		$f31

/*======================================================================*/
/*                  PAL TEMPORARY REGISTER DEFINITIONS			*/
/*======================================================================*/

#define	pt0		IPR$M_PAL + 0x0 
#define	pt1		IPR$M_PAL + 0x1
#define	pt2		IPR$M_PAL + 0x2
#define	pt3		IPR$M_PAL + 0x3
#define	pt4		IPR$M_PAL + 0x4
#define	pt5		IPR$M_PAL + 0x5
#define	pt6		IPR$M_PAL + 0x6
#define	pt7		IPR$M_PAL + 0x7
#define	pt8		IPR$M_PAL + 0x8
#define	pt9		IPR$M_PAL + 0x9
#define	pt10		IPR$M_PAL + 0xA
#define	pt11		IPR$M_PAL + 0xB
#define	pt12		IPR$M_PAL + 0xC
#define	pt13		IPR$M_PAL + 0xD
#define	pt14		IPR$M_PAL + 0XE
#define	pt15		IPR$M_PAL + 0xF
#define	pt16		IPR$M_PAL + 0x10
#define	pt17		IPR$M_PAL + 0x11
#define	pt18		IPR$M_PAL + 0x12
#define	pt19		IPR$M_PAL + 0x13
#define	pt20		IPR$M_PAL + 0x14
#define	pt21		IPR$M_PAL + 0x15
#define	pt22		IPR$M_PAL + 0x16
#define	pt23		IPR$M_PAL + 0x17
#define	pt24		IPR$M_PAL + 0x18
#define	pt25		IPR$M_PAL + 0x19
#define	pt26		IPR$M_PAL + 0x1A
#define	pt27		IPR$M_PAL + 0x1B
#define	pt28		IPR$M_PAL + 0x1C
#define	pt29		IPR$M_PAL + 0x1D
#define	pt30		IPR$M_PAL + 0x1E
#define	pt31		IPR$M_PAL + 0x1F

/*======================================================================*/
/*   DECchip 21064/21066 Privileged Architecture Library Entry Points	*/
/*======================================================================*/

/*
**	Entry Name	    Offset (Hex)	Length (Instructions)
**
**	RESET			0000		    8
**	MCHK			0020		   16
**	ARITH			0060		   32
**	INTERRUPT		00E0		   64
**	D_FAULT			01E0		  128
**	ITB_MISS		03E0		  256
**	ITB_ACV			07E0		   64
**	DTB_MISS (Native)	08E0		   64
**	DTB_MISS (PAL)		09E0		  512
**	UNALIGN			11E0		  128
**	OPCDEC			13E0		  256
**	FEN			17E0		  520
**	CALL_PAL (Privileged)	2000
**	CALL_PAL (Unprivileged) 3000
*/

#define PAL$RESET_ENTRY		    0x0000
#define PAL$MCHK_ENTRY		    0x0020
#define PAL$ARITH_ENTRY		    0x0060
#define PAL$INTERRUPT_ENTRY	    0x00E0
#define PAL$D_FAULT_ENTRY	    0x01E0
#define PAL$ITB_MISS_ENTRY	    0x03E0
#define PAL$ITB_ACV_ENTRY	    0x07E0
#define PAL$NDTB_MISS_ENTRY	    0x08E0
#define PAL$PDTB_MISS_ENTRY	    0x09E0
#define PAL$UNALIGN_ENTRY	    0x11E0
#define PAL$OPCDEC_ENTRY	    0x13E0
#define PAL$FEN_ENTRY		    0x17E0
#define PAL$CALL_PAL_PRIV_ENTRY	    0x2000
#define PAL$CALL_PAL_UNPRIV_ENTRY   0x3000

/*
** Architecturally Reserved Opcode Definitions
*/

#define	mtpr	    hw_mtpr
#define	mfpr	    hw_mfpr

#define	ldl_a	    hw_ldl/a
#define ldq_a	    hw_ldq/a
#define stq_a	    hw_stq/a
#define stl_a	    hw_stl/a

#define ldl_p	    hw_ldl/p
#define ldq_p	    hw_ldq/p
#define stl_p	    hw_stl/p
#define stq_p	    hw_stq/p

/*
** Physical mode load-lock and store-conditional variants of
** HW_LD and HW_ST.
*/

#define ldl_pa	    hw_ldl/pa
#define ldq_pa	    hw_ldq/pa
#define stl_pa	    hw_stl/pa
#define stq_pa	    hw_stq/pa

/*
**  This table is an accounting of the DECchip 21064/21066 storage
**  used to implement the SRM defined state for OSF/1.
*/

#define pt2_iccsr	IPR$M_PAL + IPR$M_IBX + 0x2 /* ICCSR shadow register*/

#define pt9_ps		IPR$M_PAL + IPR$M_IBX + 0x9 /* PS shadow register   */

#define ptEntInt	pt10	/* Entry point to HW interrupt dispatch	    */

#if defined(KDEBUG)

#define ptEntDbg	pt11	/* Entry point to kernel debugger           */

#endif /* KDEBUG */

#define	ptEntArith	pt12	/* Entry point to arithmetic trap dispatch  */
#define ptEntMM		pt13	/* Entry point to MM fault dispatch	    */
#define ptEntUna	pt14	/* Entry point to unaligned access dispatch */
#define ptEntSys	pt15	/* Entry point to syscall dispatch	    */
#define ptEntIF		pt16	/* Entry point to instruction fault dispatch*/
#define ptImpure	pt17	/* Pointer to common impure area	    */
#define ptUsp		pt18	/* User stack pointer			    */
#define ptKsp		pt19	/* Kernel stack pointer			    */
#define ptKgp		pt20	/* Kernel global pointer		    */

#define ptIntMask	pt22	/* Interrupt enable masks for IRQ_L<7:0>    */

#define ptSysVal	pt24	/* Per-processor system value		    */
#define ptMces		pt25	/* Machine check error status		    */
#define ptWhami		pt27	/* Who-Am-I ... and why am I here! ;^)	    */
#define ptPtbr		pt28	/* Page table base register		    */
#define ptVptPtr	pt29	/* Virtual page table pointer		    */

#define ptPrevPal	pt30	/* Previous PAL base			    */
#define ptPrcb		pt31	/* Pointer to process control block	    */

