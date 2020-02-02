        //
        // VID: [T1.2] PT: [Fri Apr  1 17:00:05 1994] SF: [macros.h]
        //  TI: [/sae_users/ericr/tools/vice -iplatform.s -l// -p# -DDC21064 -DEB64 -h -m -aeb64 ]
        //
#define	__MACROS_LOADED	    1
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
**	macros.h
**
**  MODULE DESCRIPTION:
**
**      Common macro definitions
**
**  AUTHOR: ER
**
**  CREATION DATE:  11-Dec-1992
**
**  $Id: macros.h,v 1.1.1.1 1995/08/01 17:33:53 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: macros.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:53  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.1  1995/05/22  09:27:38  rusling
 * EB64 PALcode sources.
 *
 * Revision 1.2  1995/03/28  09:15:24  rusling
 * Updated to latest EB baselevel.
 *
**  Revision 2.1  1994/04/01  21:55:51  ericr
**  1-APR-1994 V2 OSF/1 PALcode
**
**  Revision 1.4  1994/03/30  16:33:58  ericr
**  Substituted LDLI macro in place of GET_ADDR
**
**  Revision 1.3  1994/03/09  15:53:16  ericr
**  Fixed ldah base bug in GET_ADDR
**
**  Revision 1.2  1994/03/08  00:12:54  ericr
**  Moved SAVE_STATE macro to impure.h
**
**  Revision 1.1  1994/02/28  18:23:46  ericr
**  Initial revision
**
**
*/

#define	STALL \
    mtpr    r31, 0

#define	NOP \
    bis	    r31, r31, r31

/*
** Align code on an 8K byte page boundary.
*/
#define	ALIGN_PAGE \
    .align  13

/*
** Align code on a 32 byte cache block boundary.
*/
#define	ALIGN_CACHE_BLOCK \
    .align  5

/*
** Align code on a quadword boundary.
*/
#define ALIGN_BRANCH_TARGET \
    .align  3

/*
** Hardware vectors go in .text 0 sub-segment.
*/
#define	HDW_VECTOR(offset) \
    . = offset

/*
** Privileged CALL_PAL functions are in .text 1 sub-segment.
*/
#define	CALL_PAL_PRIV(vector) \
    . = (PAL$CALL_PAL_PRIV_ENTRY+(vector<<6))

/*
** Unprivileged CALL_PAL functions are in .text 1 sub-segment,
** privileged bit is removed from these vectors.
*/
#define CALL_PAL_UNPRIV(vector) \
    . = (PAL$CALL_PAL_UNPRIV_ENTRY+((vector&0x3F)<<6))

/* 
** Implements a load "immediate" longword function 
*/
#define LDLI(reg,val) \
	ldah	reg, ((val+0x8000) >> 16)(zero); \
	lda	reg, (val&0xffff)(reg)

