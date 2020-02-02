        //
        // VID: [T1.2] PT: [Fri Apr  1 17:00:24 1994] SF: [impure.h]
        //  TI: [/sae_users/ericr/tools/vice -iplatform.s -l// -p# -DDC21064 -DEB64P -DPASS1 -h -m -aeb64p_p1 ]
        //
#define	__IMPURE_LOADED	    1
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
**	impure.h
**
**  MODULE DESCRIPTION:
**
**      PAL impure scratch and logout area data structure 
**	definitions
**
**  AUTHOR: ER
**
**  CREATION DATE:  23-Feb-1994
**
**  $Id: impure.h,v 1.1.1.1 1995/08/01 17:33:54 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: impure.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:54  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:28:50  rusling
 * PALcode for the rev 1 EB64+.
 *
 * Revision 1.2  1995/03/28  09:20:44  rusling
 * updated to latest EB baselevel.
 *
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.6  1994/03/28  17:52:28  ericr
**  Changed SAVE_FPRS macro to clear all DTB entries
**
**  Revision 1.5  1994/03/23  19:55:01  ericr
**  Moved SAVE_FPR macro call to the end of SAVE_STATE
**
**  Revision 1.4  1994/03/14  20:47:50  ericr
**  Added common area offset definitions.
**  Changed base of saved state area from 0x40 to 0x100
**  Created SAVE_GPRS, SAVE_FPRS, and SAVE_PAL macros
**  Changed SAVE_STATE to be a composite of SAVE macros
**
**  Revision 1.3  1994/03/08  20:28:49  ericr
**  Fixed bug in RESTORE_GPRS
**
**  Revision 1.2  1994/03/08  00:12:27  ericr
**  Moved SAVE_STATE macro to here from macros.h
**  Added RESTORE_GPRS macro
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
*/

/*
** NOTE:
** Offset definitions must be 12-bits or less, as the HW_LD
** and HW_ST instructions only support a 12-bit signed displacement.
*/

/*
** Common Area Offset Definitions:
*/

#define CNS$Q_RCS_ID		0
#define CNS$Q_SCRATCH		0xA0
#define CNS$Q_BASE		0x100	/* Base of saved state area */

/*
** Processor Saved State Area Offset Definitions:
** 
** These offsets are relative to the CNS$Q_BASE offset 
** defined in the common area.
*/

#define CNS$Q_GPR		0x0
#define CNS$Q_FPR		0x100
#define CNS$Q_PT		0x200
#define CNS$Q_EXC_ADDR		0x300
#define CNS$Q_PS		0x308
#define CNS$Q_PAL_BASE		0x310
#define CNS$Q_HIER		0x318
#define CNS$Q_SIRR		0x320
#define CNS$Q_ICCSR		0x328
#define CNS$Q_ABOX_CTL		0x330

#define CNS$Q_BIU_CTL		0x338

#define CNS$Q_MEM_SIZE          0x340
#define CNS$Q_CYCLE_CNT         0x348
#define CNS$Q_SIGNATURE         0x350
#define CNS$Q_PROC_MASK         0x358
#define CNS$Q_SYSCTX            0x360

#define	CNS$K_SIZE		0x368

/*
** Short Corrected Error Logout Frame Offset Definitions:
*/

#define LAS$Q_BASE		(CNS$Q_BASE+CNS$K_SIZE)

#define	LAS$L_FRAME		0x0000
#define	LAS$L_FLAG		0x0004
#define	LAS$L_CPU		0x0008
#define	LAS$L_SYS		0x000C
#define	LAS$Q_MCHK_CODE		0x0010

#define	LAS$Q_BIU_STAT		0x0018
#define	LAS$Q_BIU_ADDR		0x0020
#define	LAS$Q_DC_STAT		0x0028
#define	LAS$Q_FILL_SYNDROME	0x0030
#define	LAS$Q_FILL_ADDR		0x0038
#define	LAS$Q_BC_TAG		0x0040

#define	LAS$K_SIZE		0x0048	/* Frame size */

/*
** Long Machine Check Error Logout Frame Offset Definitions:
*/

#define LAF$Q_BASE		(LAS$Q_BASE+LAS$K_SIZE)

#define	LAF$L_FRAME		0x0000
#define	LAF$L_FLAG		0x0004
#define	LAF$L_CPU		0x0008
#define	LAF$L_SYS		0x000C

#define	LAF$Q_PT0		0x0010
#define	LAF$Q_PT1		0x0018
#define	LAF$Q_PT2		0x0020
#define	LAF$Q_PT3		0x0028
#define	LAF$Q_PT4		0x0030
#define	LAF$Q_PT5		0x0038
#define	LAF$Q_PT6		0x0040
#define	LAF$Q_PT7		0x0048
#define	LAF$Q_PT8		0x0050
#define	LAF$Q_PT9		0x0058
#define	LAF$Q_PT10		0x0060
#define	LAF$Q_PT11		0x0068
#define	LAF$Q_PT12		0x0070
#define	LAF$Q_PT13		0x0078
#define	LAF$Q_PT14		0x0080
#define	LAF$Q_PT15		0x0088
#define	LAF$Q_PT16		0x0090
#define	LAF$Q_PT17		0x0098
#define	LAF$Q_PT18		0x00A0
#define	LAF$Q_PT19		0x00A8
#define	LAF$Q_PT20		0x00B0
#define	LAF$Q_PT21		0x00B8
#define	LAF$Q_PT22		0x00C0
#define	LAF$Q_PT23		0x00C8
#define	LAF$Q_PT24		0x00D0
#define	LAF$Q_PT25		0x00D8
#define	LAF$Q_PT26		0x00E0
#define	LAF$Q_PT27		0x00E8
#define	LAF$Q_PT28		0x00F0
#define	LAF$Q_PT29		0x00F8
#define	LAF$Q_PT30		0x0100
#define	LAF$Q_PT31		0x0108

#define LAF$Q_EXC_ADDR		0x0110
#define LAF$Q_PAL_BASE		0x0130
#define LAF$Q_HIER		0x0138
#define LAF$Q_HIRR		0x0140
#define LAF$Q_MM_CSR		0x0148
#define LAF$Q_DC_STAT		0x0150
#define LAF$Q_DC_ADDR		0x0158
#define LAF$Q_ABOX_CTL		0x0160

#define LAF$Q_BIU_STAT		0x0168
#define LAF$Q_BIU_ADDR		0x0170
#define LAF$Q_BIU_CTL		0x0178
#define LAF$Q_FILL_SYNDROME	0x0180
#define LAF$Q_FILL_ADDR		0x0188
#define LAF$Q_VA		0x0190
#define LAF$Q_BC_TAG		0x0198

#define LAF$K_SIZE		0x01A0	    /* Frame size */

#define LAF$Q_SYS_BASE		LAF$K_SIZE


/*
** Macro to save the internal state of the general purpose
** register file.
**
** Register Usage Conventions:
**
**      t0 (r1) - Saved in pt0.
**      t1 (r2) - Base address of the save state area.
*/
#define SAVE_GPRS \
	stq_p   r0,  (CNS$Q_GPR + 0x00)(t1); \
	mfpr    r0, pt0; \
	stq_p   r0,  (CNS$Q_GPR + 0x08)(t1); \
	mfpr    r0, pt1; \
	stq_p   r0,  (CNS$Q_GPR + 0x10)(t1); \
	stq_p   r3,  (CNS$Q_GPR + 0x18)(t1); \
	stq_p   r4,  (CNS$Q_GPR + 0x20)(t1); \
	stq_p   r5,  (CNS$Q_GPR + 0x28)(t1); \
	stq_p   r6,  (CNS$Q_GPR + 0x30)(t1); \
	stq_p   r7,  (CNS$Q_GPR + 0x38)(t1); \
	stq_p   r8,  (CNS$Q_GPR + 0x40)(t1); \
	stq_p   r9,  (CNS$Q_GPR + 0x48)(t1); \
	stq_p   r10, (CNS$Q_GPR + 0x50)(t1); \
	stq_p   r11, (CNS$Q_GPR + 0x58)(t1); \
	stq_p   r12, (CNS$Q_GPR + 0x60)(t1); \
	stq_p   r13, (CNS$Q_GPR + 0x68)(t1); \
	stq_p   r14, (CNS$Q_GPR + 0x70)(t1); \
	stq_p   r15, (CNS$Q_GPR + 0x78)(t1); \
	stq_p   r16, (CNS$Q_GPR + 0x80)(t1); \
	stq_p   r17, (CNS$Q_GPR + 0x88)(t1); \
	stq_p   r18, (CNS$Q_GPR + 0x90)(t1); \
	stq_p   r19, (CNS$Q_GPR + 0x98)(t1); \
	stq_p   r20, (CNS$Q_GPR + 0xA0)(t1); \
	stq_p   r21, (CNS$Q_GPR + 0xA8)(t1); \
	stq_p   r22, (CNS$Q_GPR + 0xB0)(t1); \
	stq_p   r23, (CNS$Q_GPR + 0xB8)(t1); \
	stq_p   r24, (CNS$Q_GPR + 0xC0)(t1); \
	stq_p   r25, (CNS$Q_GPR + 0xC8)(t1); \
	stq_p   r26, (CNS$Q_GPR + 0xD0)(t1); \
	stq_p   r27, (CNS$Q_GPR + 0xD8)(t1); \
	stq_p   r28, (CNS$Q_GPR + 0xE0)(t1); \
	stq_p   r29, (CNS$Q_GPR + 0xE8)(t1); \
	stq_p   r30, (CNS$Q_GPR + 0xF0)(t1); \
	stq_p   r31, (CNS$Q_GPR + 0xF8)(t1)

/*
** Macro to save the internal state of the floating point
** register file.
**
** Register Usage Conventions:
**
**	t0 (r1) - Scratch
**      t1 (r2) - Base address of console save state area.
**	t2 (r3) - Scratch
**	t3 (r4) - Scratch
*/
#define SAVE_FPRS \
	mtpr	zero, dtbZap; \
	mfpr	t0, pt2; \
	bis	zero, 1, t3; \
	sll	t3, ICCSR$V_FPE, t3; \
	bis	t0, t3, t3; \
	mtpr	t3, pt2_iccsr; \
	mfpr	t2, ptPtbr; \
	bis	t2, 1, t3; \
	mtpr	t3, ptPtbr; \
	stt     f0,  (CNS$Q_FPR + 0x00)(t1); \
	stt     f1,  (CNS$Q_FPR + 0x08)(t1); \
	stt     f2,  (CNS$Q_FPR + 0x10)(t1); \
	stt     f3,  (CNS$Q_FPR + 0x18)(t1); \
	stt     f4,  (CNS$Q_FPR + 0x20)(t1); \
	stt     f5,  (CNS$Q_FPR + 0x28)(t1); \
	stt     f6,  (CNS$Q_FPR + 0x30)(t1); \
	stt     f7,  (CNS$Q_FPR + 0x38)(t1); \
	stt     f8,  (CNS$Q_FPR + 0x40)(t1); \
	stt     f9,  (CNS$Q_FPR + 0x48)(t1); \
	stt     f10, (CNS$Q_FPR + 0x50)(t1); \
	stt     f11, (CNS$Q_FPR + 0x58)(t1); \
	stt     f12, (CNS$Q_FPR + 0x60)(t1); \
	stt     f13, (CNS$Q_FPR + 0x68)(t1); \
	stt     f14, (CNS$Q_FPR + 0x70)(t1); \
	stt     f15, (CNS$Q_FPR + 0x78)(t1); \
	stt     f16, (CNS$Q_FPR + 0x80)(t1); \
	stt     f17, (CNS$Q_FPR + 0x88)(t1); \
	stt     f18, (CNS$Q_FPR + 0x90)(t1); \
	stt     f19, (CNS$Q_FPR + 0x98)(t1); \
	stt     f20, (CNS$Q_FPR + 0xA0)(t1); \
	stt     f21, (CNS$Q_FPR + 0xA8)(t1); \
	stt     f22, (CNS$Q_FPR + 0xB0)(t1); \
	stt     f23, (CNS$Q_FPR + 0xB8)(t1); \
	stt     f24, (CNS$Q_FPR + 0xC0)(t1); \
	stt     f25, (CNS$Q_FPR + 0xC8)(t1); \
	stt     f26, (CNS$Q_FPR + 0xD0)(t1); \
	stt     f27, (CNS$Q_FPR + 0xD8)(t1); \
	stt     f28, (CNS$Q_FPR + 0xE0)(t1); \
	stt     f29, (CNS$Q_FPR + 0xE8)(t1); \
	stt     f30, (CNS$Q_FPR + 0xF0)(t1); \
	stt     f31, (CNS$Q_FPR + 0xF8)(t1); \
	mtpr	zero, dtbZap; \
	mtpr	t2, ptPtbr; \
	mtpr	t0, pt2_iccsr

/*
** Macro to save the internal state of the PAL temporary 
** registers.
**
** Register Usage Conventions:
**
**      t1 (r2) - Base address of the save state area.
*/
#define SAVE_PALS \
	mfpr    r0, pt0; \
	stq_p   r0, (CNS$Q_PT + 0x00)(t1); \
	mfpr    r0, pt1; \
	stq_p   r0, (CNS$Q_PT + 0x08)(t1); \
	mfpr    r0, pt2; \
	stq_p   r0, (CNS$Q_PT + 0x10)(t1); \
	mfpr    r0, pt3; \
	stq_p   r0, (CNS$Q_PT + 0x18)(t1); \
	mfpr    r0, pt4; \
	stq_p   r0, (CNS$Q_PT + 0x20)(t1); \
	mfpr    r0, pt5; \
	stq_p   r0, (CNS$Q_PT + 0x28)(t1); \
	mfpr    r0, pt6; \
	stq_p   r0, (CNS$Q_PT + 0x30)(t1); \
	mfpr    r0, pt7; \
	stq_p   r0, (CNS$Q_PT + 0x38)(t1); \
	mfpr    r0, pt8; \
	stq_p   r0, (CNS$Q_PT + 0x40)(t1); \
	mfpr    r0, pt9; \
	stq_p   r0, (CNS$Q_PT + 0x48)(t1); \
	mfpr    r0, pt10; \
	stq_p   r0, (CNS$Q_PT + 0x50)(t1); \
	mfpr    r0, pt11; \
	stq_p   r0, (CNS$Q_PT + 0x58)(t1); \
	mfpr    r0, pt12; \
	stq_p   r0, (CNS$Q_PT + 0x60)(t1); \
	mfpr    r0, pt13; \
	stq_p   r0, (CNS$Q_PT + 0x68)(t1); \
	mfpr    r0, pt14; \
	stq_p   r0, (CNS$Q_PT + 0x70)(t1); \
	mfpr    r0, pt15; \
	stq_p   r0, (CNS$Q_PT + 0x78)(t1); \
	mfpr    r0, pt16; \
	stq_p   r0, (CNS$Q_PT + 0x80)(t1); \
	mfpr    r0, pt17; \
	stq_p   r0, (CNS$Q_PT + 0x88)(t1); \
	mfpr    r0, pt18; \
	stq_p   r0, (CNS$Q_PT + 0x90)(t1); \
	mfpr    r0, pt19; \
	stq_p   r0, (CNS$Q_PT + 0x98)(t1); \
	mfpr    r0, pt20; \
	stq_p   r0, (CNS$Q_PT + 0xA0)(t1); \
	mfpr    r0, pt21; \
	stq_p   r0, (CNS$Q_PT + 0xA8)(t1); \
	mfpr    r0, pt22; \
	stq_p   r0, (CNS$Q_PT + 0xB0)(t1); \
	mfpr    r0, pt23; \
	stq_p   r0, (CNS$Q_PT + 0xB8)(t1); \
	mfpr    r0, pt24; \
	stq_p   r0, (CNS$Q_PT + 0xC0)(t1); \
	mfpr    r0, pt25; \
	stq_p   r0, (CNS$Q_PT + 0xC8)(t1); \
	mfpr    r0, pt26; \
	stq_p   r0, (CNS$Q_PT + 0xD0)(t1); \
	mfpr    r0, pt27; \
	stq_p   r0, (CNS$Q_PT + 0xD8)(t1); \
	mfpr    r0, pt28; \
	stq_p   r0, (CNS$Q_PT + 0xE0)(t1); \
	mfpr    r0, pt29; \
	stq_p   r0, (CNS$Q_PT + 0xE8)(t1); \
	mfpr    r0, pt30; \
	stq_p   r0, (CNS$Q_PT + 0xF0)(t1); \
	mfpr    r0, pt31; \
	stq_p   r0, (CNS$Q_PT + 0xF8)(t1)

/*
** Macro to save the internal state of the machine to the 
** impure scratch area.
**
** Register Usage Conventions:
**
**      t0 (r1) - Saved in pt0.
**      t1 (r2) - Base address of impure scratch area.
*/
#define SAVE_STATE \
        lda	t1, CNS$Q_BASE(t1); \
        SAVE_GPRS; \
        SAVE_PALS; \
	mfpr    r0, excAddr; \
	stq_p   r0, CNS$Q_EXC_ADDR(t1); \
	mfpr    r0, ps; \
	stq_p   r0, CNS$Q_PS(t1); \
	mfpr    r0, palBase; \
	stq_p   r0, CNS$Q_PAL_BASE(t1); \
	mfpr    r0, hier; \
	stq_p   r0, CNS$Q_HIER(t1); \
	mfpr    r0, sirr; \
	stq_p   r0, CNS$Q_SIRR(t1); \
	mfpr    r0, iccsr; \
	stq_p   r0, CNS$Q_ICCSR(t1); \
        SAVE_FPRS; \
        STALL; \
        STALL

/*
** Macro to restore the internal state of the general purpose
** register file from the impure scratch area.
**
** Register Usage Conventions:
**
**      t1 (r2) - Base address of the save state area.
*/
#define RESTORE_GPRS \
	ldq_p   r0,  (CNS$Q_GPR + 0x00)(t1); \
	ldq_p   r1,  (CNS$Q_GPR + 0x08)(t1); \
	ldq_p   r3,  (CNS$Q_GPR + 0x18)(t1); \
	ldq_p   r4,  (CNS$Q_GPR + 0x20)(t1); \
	ldq_p   r5,  (CNS$Q_GPR + 0x28)(t1); \
	ldq_p   r6,  (CNS$Q_GPR + 0x30)(t1); \
	ldq_p   r7,  (CNS$Q_GPR + 0x38)(t1); \
	ldq_p   r8,  (CNS$Q_GPR + 0x40)(t1); \
	ldq_p   r9,  (CNS$Q_GPR + 0x48)(t1); \
	ldq_p   r10, (CNS$Q_GPR + 0x50)(t1); \
	ldq_p   r11, (CNS$Q_GPR + 0x58)(t1); \
	ldq_p   r12, (CNS$Q_GPR + 0x60)(t1); \
	ldq_p   r13, (CNS$Q_GPR + 0x68)(t1); \
	ldq_p   r14, (CNS$Q_GPR + 0x70)(t1); \
	ldq_p   r15, (CNS$Q_GPR + 0x78)(t1); \
	ldq_p   r16, (CNS$Q_GPR + 0x80)(t1); \
	ldq_p   r17, (CNS$Q_GPR + 0x88)(t1); \
	ldq_p   r18, (CNS$Q_GPR + 0x90)(t1); \
	ldq_p   r19, (CNS$Q_GPR + 0x98)(t1); \
	ldq_p   r20, (CNS$Q_GPR + 0xA0)(t1); \
	ldq_p   r21, (CNS$Q_GPR + 0xA8)(t1); \
	ldq_p   r22, (CNS$Q_GPR + 0xB0)(t1); \
	ldq_p   r23, (CNS$Q_GPR + 0xB8)(t1); \
	ldq_p   r24, (CNS$Q_GPR + 0xC0)(t1); \
	ldq_p   r25, (CNS$Q_GPR + 0xC8)(t1); \
	ldq_p   r26, (CNS$Q_GPR + 0xD0)(t1); \
	ldq_p   r27, (CNS$Q_GPR + 0xD8)(t1); \
	ldq_p   r28, (CNS$Q_GPR + 0xE0)(t1); \
	ldq_p   r29, (CNS$Q_GPR + 0xE8)(t1); \
	ldq_p   r30, (CNS$Q_GPR + 0xF0)(t1); \
	ldq_p   r31, (CNS$Q_GPR + 0xF8)(t1); \
	ldq_p   r2,  (CNS$Q_GPR + 0x10)(t1)

