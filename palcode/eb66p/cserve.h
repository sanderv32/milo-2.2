/*
 *      VID: [T1.2] PT: [Wed Jul  5 13:35:48 1995] SF: [cserve.h]
 *       TI: [/sae_share/apps/bin/vice -iplatform.s -l// -p# -DDC21066 -DEB66 -h -m -aeb66 ]
 */
#define	__CSERVE_LOADED	1
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
**	cserve.h
**
**  MODULE DESCRIPTION:
**
**      Platform specific cserve definitions.
**
**  AUTHOR: ES
**
**  CREATION DATE:  16-Jun-1994
**
**  $Id: cserve.h,v 1.1.1.1 1995/08/01 17:33:55 paradis Exp $
**
**  MODIFICATION HISTORY:
**
**  $Log: cserve.h,v $
 * Revision 1.1.1.1  1995/08/01  17:33:55  paradis
 * Linux 1.3.10 with NoName and miniloader updates
 *
 * Revision 1.2  1995/07/25  19:25:39  paradis
 * Updates for Blade 2
 *
**  Revision 1.1  1994/06/16  15:32:31  samberg
**  Initial revision
**
**
*/

/*
** Console Service (cserve) sub-function codes:
*/
#define CSERVE_K_LDQP           0x01
#define CSERVE_K_STQP           0x02
#define CSERVE_K_RD_ABOX        0x03
#define CSERVE_K_RD_ESR         0x04
#define CSERVE_K_RD_ICCSR       0x05
#define CSERVE_K_WR_ABOX        0x06
#define CSERVE_K_WR_ESR         0x07
#define CSERVE_K_WR_ICCSR       0x08
#define CSERVE_K_JTOPAL         0x09
#define CSERVE_K_WR_INT         0x0A
#define CSERVE_K_RD_IMPURE      0x0B
#define CSERVE_K_PUTC           0x0F


