/*
 *      VID: [T1.2] PT: [Fri Apr 21 14:23:32 1995] SF: [osf.h]
 *       TI: [/sae_users/fdh/bin/vice -iplatform.s -l// -p# -DDC21066 -DEB66 -DPASS1 -h -m -aeb66_p1 ]
 */
#define	__OSF_LOADED	1
/*
*****************************************************************************
**                                                                          *
**  Copyright © 1993, 1994  						    *
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
**	osf.h
**
**  MODULE DESCRIPTION:
**
**      OSF/1 specific definitions
**
**  AUTHOR: ER
**
**  CREATION DATE:  29-Oct-1992
**
**  $Id: osf.h,v 1.1.1.1 1995/08/01 17:33:56 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: osf.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:56  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.2  1995/07/25  19:25:47  paradis
 * Updates for Blade 2
 *
**  Revision 2.6  1995/04/21  02:18:25  fdh
**  Replaced C++ style comments with Standard C style comments.
**
**  Revision 2.5  1994/08/10  17:05:31  samberg
**  Added MCHK_K_SYSTEM and SCB_Q_SYSMCHK for SIO NMI
**
**  Revision 2.4  1994/06/20  19:57:40  fdh
**  Simplified definitions for Unpriviledged call_pals so that
**  they can be handled by the ASAXP assembler.
**
**  Revision 2.3  1994/06/16  14:48:38  samberg
**  For ANSI, changed $ to _, except for pvc and reg def
**
**  Revision 2.2  1994/05/19  17:47:20  samberg
**  Swap flag is now a 4 byte constant 'SWAP'
**
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.3  1994/03/15  23:48:48  ericr
**  Moved BUILD_STACK_FRAME macro from osfpal.s to here.
**
**  Revision 1.2  1994/03/08  20:28:02  ericr
**  Replaced DEBUG_MONITOR conditional with KDEBUG
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
**
*/

/*
**  Seg0 and Seg1 Virtual Address (VA) Format
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	<42:33>  10	SEG1	First level page table offset
**	<32:23>  10	SEG2	Second level page table offset
**	<22:13>  10	SEG3	Third level page table offset
**	 <12:0>  13	OFFSET	Byte within page offset
*/

#define VA_V_SEG1	33
#define	VA_M_SEG1	(0x3FF<<VA_V_SEG1)
#define VA_V_SEG2	23
#define VA_M_SEG2	(0x3FF<<VA_V_SEG2)
#define VA_V_SEG3	13
#define VA_M_SEG3	(0x3FF<<VA_V_SEG3)
#define VA_V_OFFSET	0
#define VA_M_OFFSET	0x1FFF

/*
**  Virtual Address Options: 8K byte page size
*/

#define	VA_S_SIZE	43
#define	VA_S_OFF	13
#define VA_S_SEG	10
#define VA_S_PAGE_SIZE	8192

/*
**  Page Table Entry (PTE) Format
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	<63:32>	  32	PFN	Page Frame Number
**	<31:16>	  16	SW	Reserved for software
**	<15:14>	   2	RSV0	Reserved for hardware SBZ
**	   <13>	   1	UWE	User Write Enable
**	   <12>	   1	KWE	Kernel Write Enable
**	<11:10>	   2	RSV1	Reserved for hardware SBZ
**	    <9>	   1	URE	User Read Enable
**	    <8>	   1	KRE	Kernel Read Enable
**	    <7>	   1	RSV2	Reserved for hardware SBZ
**	  <6:5>	   2	GH	Granularity Hint
**	    <4>	   1	ASM	Address Space Match
**	    <3>	   1	FOE	Fault On Execute
**	    <2>	   1	FOW	Fault On Write
**	    <1>	   1	FOR	Fault On Read
**	    <0>	   1	V	Valid
*/

#define	PTE_V_PFN	32
#define PTE_V_SW	16
#define PTE_V_UWE	13
#define PTE_M_UWE	(1<<PTE_V_UWE)
#define PTE_V_KWE	12
#define PTE_M_KWE	(1<<PTE_V_KWE)
#define PTE_V_URE	9
#define PTE_M_URE	(1<<PTE_V_URE)
#define PTE_V_KRE	8
#define PTE_M_KRE	(1<<PTE_V_KRE)
#define PTE_V_GH	5
#define PTE_M_GH	(3<<PTE_V_GH)
#define PTE_V_ASM	4
#define PTE_M_ASM	(1<<PTE_V_ASM)
#define PTE_V_FOE	3
#define PTE_M_FOE	(1<<PTE_V_FOE)
#define PTE_V_FOW	2
#define PTE_M_FOW	(1<<PTE_V_FOW)
#define PTE_V_FOR	1
#define PTE_M_FOR	(1<<PTE_V_FOR)
#define PTE_V_VALID	0
#define PTE_M_VALID	(1<<PTE_V_VALID)

#define PTE_M_KSEG	0x1111
#define PTE_M_PROT	0x3300

/*
**  System Entry Instruction Fault (entIF) Constants:
*/

#define	IF_K_BPT	0x0
#define IF_K_BUGCHK	0x1
#define IF_K_GENTRAP	0x2
#define IF_K_FEN	0x3
#define IF_K_OPCDEC	0x4

/*
**  System Entry Hardware Interrupt (entInt) Constants:
*/

#define INT_K_IP	0x0
#define INT_K_CLK	0x1
#define INT_K_MCHK	0x2
#define INT_K_DEV	0x3
#define INT_K_PERF	0x4

/*
**  System Entry MM Fault (entMM) Constants:
*/

#define	MM_K_TNV	0x0
#define MM_K_ACV	0x1
#define MM_K_FOR	0x2
#define MM_K_FOE	0x3
#define MM_K_FOW	0x4

/*
**  Process Control Block (PCB) Offsets
*/

#define PCB_Q_KSP	0x0000
#define PCB_Q_USP	0x0008
#define PCB_Q_PTBR	0x0010
#define PCB_L_PCC	0x0018
#define PCB_L_ASN	0x001C
#define PCB_Q_UNIQUE	0x0020
#define PCB_Q_FEN	0x0028
#define PCB_Q_RSV0	0x0030
#define PCB_Q_RSV1	0x0038

/*
**  Processor Status Register (PS) Bit Summary
**
**	 Loc	Size	Name	Function
**	-----	----	----	---------------------------------
**	  <3>	 1	CM	Current Mode
**	<2:0>	 3	IPL	Interrupt Priority Level
**/

#define	PS_V_CM		3
#define PS_M_CM		(1<<PS_V_CM)
#define	PS_V_IPL	0
#define	PS_M_IPL	(7<<PS_V_IPL)

#define	PS_K_KERN	0x0
#define PS_K_USER	0x1

#define	IPL_K_ZERO	0x0
#define IPL_K_SW0	0x1
#define IPL_K_SW1	0x2
#define IPL_K_DEV0	0x3
#define IPL_K_DEV1	0x4
#define IPL_K_CLK	0x5
#define IPL_K_RT	0x6
#define IPL_K_PERF	0x6
#define IPL_K_MCHK	0x7

#define IPL_K_LOW	0x0
#define IPL_K_HIGH	0x7

/*
**  Stack Frame (FRM) Offsets:
**
**  There are two types of system entries for OSF/1 - those for the
**  callsys CALL_PAL function and those for exceptions and interrupts.
**  Both entry types use the same stack frame layout.  The stack frame
**  contains space for the PC, the PS, the saved GP, and the saved
**  argument registers a0, a1, and a2.  On entry, SP points to the
**  saved PS.
*/

#define	FRM_Q_PS	0x0000
#define FRM_Q_PC	0x0008
#define FRM_Q_GP	0x0010
#define FRM_Q_A0	0x0018
#define FRM_Q_A1	0x0020
#define FRM_Q_A2	0x0028

#define FRM_K_SIZE	48

#define BUILD_STACK_FRAME(ps,pc)    \
    sll	    ps, 63-PS_V_CM, ps;	    \
    bge	    ps, 0f;		    \
    mtpr    sp, ptUsp;		    \
    mfpr    sp, ptKsp;		    \
    mtpr    zero, pt9_ps;	    \
0:  srl	    ps, 63-PS_V_CM, ps;	    \
    lda	    sp, 0-FRM_K_SIZE(sp);   \
    stq_a   ps, FRM_Q_PS(sp);	    \
    stq_a   pc, FRM_Q_PC(sp);	    \
    stq_a   gp, FRM_Q_GP(sp);	    \
    stq_a   a0, FRM_Q_A0(sp);	    \
    stq_a   a1, FRM_Q_A1(sp);	    \
    stq_a   a2,	FRM_Q_A2(sp)

/*
**  Halt codes:
*/

#define HLT_K_RESET	    0x0000
#define HLT_K_HW_HALT	    0x0001
#define HLT_K_KSP_INVAL	    0x0002
#define HLT_K_SCBB_INVAL    0x0003
#define HLT_K_PTBR_INVAL    0x0004
#define HLT_K_SW_HALT	    0x0005
#define HLT_K_DBL_MCHK	    0x0006
#define HLT_K_MCHK_FROM_PAL 0x0007

/*
**  Processor Machine check codes:
*/

#define MCHK_K_REV	    0x0001  /* Machine Check revision level */

#define MCHK_K_TPERR	    0x0080
#define MCHK_K_TCPERR	    0x0082
#define MCHK_K_HERR	    0x0084
#define MCHK_K_ECC_C	    0x0086
#define MCHK_K_ECC_NC	    0x0088
#define MCHK_K_UNKNOWN	    0x008A
#define MCHK_K_CACKSOFT	    0x008C
#define MCHK_K_BUGCHECK	    0x008E
#define MCHK_K_OS_BUGCHECK  0x0090
#define MCHK_K_DCPERR       0x0092
#define MCHK_K_ICPERR	    0x0094

/*
** Base value for system machine checks. All system machine check values
** are found in platform.h and will have a value of 0x0200 or greater.
*/

#define MCHK_K_SYSTEM       0x0200  /* Base value for system machine checks*/

/*
**  SCB offsets:
*/

#define SCB_Q_SYSERR        0x0620  /* Offset for sce  */
#define SCB_Q_PROCERR	    0x0630  /* Offset for pce  */
#define SCB_Q_SYSMCHK       0x0660  /* Offset for system mchk */
#define SCB_Q_PROCMCHK      0x0670  /* Offset for processor mchk */

/*
**  Machine Check Error Status Summary (MCES) Register Format
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	    <0>	   1	MIP	Machine check in progress
**	    <1>	   1	SCE	System correctable error in progress
**	    <2>	   1	PCE	Processor correctable error in progress
**	    <3>	   1	DPC	Disable PCE error reporting
**	    <4>	   1	DSC	Disable SCE error reporting
*/

#define MCES_V_MIP	0
#define MCES_M_MIP	(1<<MCES_V_MIP)
#define MCES_V_SCE	1
#define MCES_M_SCE	(1<<MCES_V_SCE)
#define MCES_V_PCE	2
#define MCES_M_PCE	(1<<MCES_V_PCE)
#define MCES_V_DPC	3
#define MCES_M_DPC	(1<<MCES_V_DPC)
#define MCES_V_DSC	4
#define MCES_M_DSC	(1<<MCES_V_DSC)

/*
**  Who-Am-I (WHAMI) Register Format
**
**	  Loc	Size	Name	Function
**	 -----	----	----	---------------------------------
**	 <7:0>	  8	ID	Who-Am-I identifier
**	 <63:32> 32	SWAP	Swap flag
*/
#define WHAMI_V_SWAP	32
#define WHAMI_K_SWAP    0x53574150      /* <63:56>='S' */
					/* <55:48>='W' */
					/* <47:40>='A' */
					/* <39:32>='P' */
#define WHAMI_V_ID	0
#define WHAMI_M_ID	(0xFF<<WHAMI_V_ID)

/*
**  Conventional Register Usage Definitions
**
**  Assembler temporary `at' is `AT' so it doesn't conflict with the
**  `.set at' assembler directive.
*/

#define v0		$0	/* Function return value register	    */
#define t0		$1	/* Scratch (temporary) registers ...	    */
#define t1		$2	/*					    */
#define t2		$3	/*					    */
#define t3		$4	/*					    */
#define t4		$5	/*					    */
#define t5		$6	/*					    */
#define t6		$7	/*					    */
#define t7		$8	/*					    */
#define s0		$9	/* Saved (non-volatile) registers ...	    */
#define s1		$10	/*					    */
#define s2		$11	/*					    */
#define s3		$12	/*					    */
#define s4		$13	/*					    */
#define s5		$14	/*					    */
#define fp		$15	/* Frame pointer register, or s6	    */
#define s6		$15	/*					    */
#define a0		$16	/* Argument registers ...		    */
#define a1		$17	/*					    */
#define a2		$18	/*					    */
#define a3		$19	/*					    */
#define a4		$20	/*					    */
#define a5		$21	/*					    */
#define t8		$22	/* Scratch (temporary) registers ...	    */
#define t9		$23	/*					    */
#define t10		$24	/*					    */
#define t11		$25	/*					    */
#define ra		$26	/* Return address register		    */
#define pv		$27	/* Procedure value register, or t12	    */
#define t12		$27	/*					    */
#define AT		$28	/* Assembler temporary (volatile) register  */
#define gp		$29	/* Global pointer register		    */
#define sp		$30	/* Stack pointer register		    */
#define zero		$31	/* Zero register			    */

/*
**  OSF/1 Unprivileged CALL_PAL Entry Points
**
**	Entry Name	    Offset (Hex)
**
**	bpt		     0080
**	bugchk		     0081
**	callsys		     0083
**	imb		     0086
**	rdunique	     009E
**	wrunique	     009F
**	gentrap		     00AA
**	dbgstop		     00AD
*/

#define	PAL_BPT_ENTRY		    0x80
#define PAL_BUGCHK_ENTRY	    0x81
#define PAL_CALLSYS_ENTRY	    0x83
#define PAL_IMB_ENTRY		    0x86
#define PAL_RDUNIQUE_ENTRY	    0x9E
#define PAL_WRUNIQUE_ENTRY	    0x9F
#define PAL_GENTRAP_ENTRY	    0xAA

#if defined(KDEBUG)
#define	PAL_DBGSTOP_ENTRY	    0xAD
#endif /* KDEBUG */

#if defined(NPHALT)
#define PAL_NPHALT_ENTRY	    0xBF
#endif /* NPHALT */

/*
**  OSF/1 Privileged CALL_PAL Entry Points
**
**	Entry Name	    Offset (Hex)
**
**	halt		     0000
**	cflush		     0001
**	draina		     0002
**	cserve		     0009
**	swppal		     000A
**	wripir		     000D
**	rdmces		     0010
**	wrmces		     0011
**	wrfen		     002B
**	wrvptptr	     002D
**	swpctx		     0030
**	wrval		     0031
**	rdval		     0032
**	tbi		     0033
**	wrent		     0034
**	swpipl		     0035
**	rdps		     0036
**	wrkgp		     0037
**	wrusp		     0038
**	wrperfmon	     0039
**	rdusp		     003A
**	whami		     003C
**	retsys		     003D
**	rti		     003F
*/

#define PAL_HALT_ENTRY	    0x0000
#define PAL_CFLUSH_ENTRY    0x0001
#define PAL_DRAINA_ENTRY    0x0002
#define PAL_CSERVE_ENTRY    0x0009
#define PAL_SWPPAL_ENTRY    0x000A
#define PAL_WRIPIR_ENTRY    0x000D
#define PAL_RDMCES_ENTRY    0x0010
#define PAL_WRMCES_ENTRY    0x0011
#define PAL_WRFEN_ENTRY	    0x002B
#define PAL_WRVPTPTR_ENTRY  0x002D
#define PAL_SWPCTX_ENTRY    0x0030
#define PAL_WRVAL_ENTRY	    0x0031
#define PAL_RDVAL_ENTRY	    0x0032
#define PAL_TBI_ENTRY	    0x0033
#define PAL_WRENT_ENTRY	    0x0034
#define PAL_SWPIPL_ENTRY    0x0035
#define PAL_RDPS_ENTRY	    0x0036
#define PAL_WRKGP_ENTRY	    0x0037
#define PAL_WRUSP_ENTRY	    0x0038
#define PAL_WRPERFMON_ENTRY 0x0039
#define PAL_RDUSP_ENTRY	    0x003A
#define PAL_WHAMI_ENTRY	    0x003C
#define PAL_RETSYS_ENTRY    0x003D
#define PAL_RTI_ENTRY	    0x003F

