/*****************************************************************************

       Copyright © 1995, 1996 Digital Equipment Corporation,
                       Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, provided  
that the copyright notice and this permission notice appear in all copies  
of software and supporting documentation, and that the name of Digital not  
be used in advertising or publicity pertaining to distribution of the software 
without specific, written prior permission. Digital grants this permission 
provided that you prominently mark, as not part of the original, any 
modifications made to this software or documentation.

Digital Equipment Corporation disclaims all warranties and/or guarantees  
with regard to this software, including all implied warranties of fitness for 
a particular purpose and merchantability, and makes no representations 
regarding the use of, or the results of the use of, the software and 
documentation in terms of correctness, accuracy, reliability, currentness or
otherwise; and you rely on the software, documentation and results solely at 
your own risk. 

******************************************************************************/

/*
 *
 *  david.rusling@reo.mts.dec.com
 *
 */

#ifndef MINIFLOPPY_H
#define MINIFLOPPY_H 1

/* fda */
#define FDADCR	0x03F2
#define FDAMSR	0x03F4
#define FDADR	0x03F5
#define FDADRR	0x03F7
#define FDADCB	0x03F7

/* dcr */
#define DCRSELA	0x00
#define DCRSELB	0x01
#define DCRSELC	0x02
#define DCRSELD	0x03
#define DCRNRES	0x04
#define DCRENAB	0x08
#define DCRMTRA	0x10
#define DCRMTRB	0x20
#define DCRMTRC	0x40
#define DCRMTRD	0x80

/* msr */
#define MSRDIO	0x40
#define MSRRQM	0x80

/* drr */
#define DRR500	0x00
#define DRR250	0x02

/* dcb */
#define DCBNCHG	0x80

/* st0 */
#define ST0IC	0xC0
#define ST0NT	0x00
#define ST0NR	0x08

/* st1 */
#define ST1NW	0x02

/* cmd */
#define NREAD	0x66
#define NWRITE	0x45
#define NRECAL	0x07
#define NSEEK	0x0F
#define NSINTS	0x08
#define NSPEC	0x03

#define FDTOMEM	0
#define MEMTOFD	1

#define NCMD	9
#define NSTS	7

#define UNITNUM	0
#define UNITSEL	(DCRSELA)
#define UNITMTR	(DCRMTRA)

#define lSRT	0xE0		/* 4 mS                          */
#define lHUT	0x08		/* 256 mS                        */

#define hSRT	0xC0		/* 4 mS                          */
#define hHUT	0x0F		/* 256 mS                        */

#define HLT	0x02		/* 4 mS                          */
#define ND	0x00		/* Use DMA                       */

#define SRT	0xE0		/* 4 mS                          */
#define HUT	0x08		/* 256 mS                        */
#define HLT	0x02		/* 4 mS                          */
#define ND	0x00		/* Use DMA                       */

#endif
