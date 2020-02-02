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

#ifdef I28F008SA
#ifndef __I28F008SA
#define __I28F008SA 1

/* ---------- F L A S H --- D E F I N I T I O N S ----------------------------*/
#define FLASH_BASE 0xfff80000
#define FLASH_BLOCK_SIZE (64*1024)
#define FLASH_BLOCKS 16
#define FLASH_BIT_SWITCH_OFFSET 0x80000

extern unsigned char read_flash_byte(int offset);
extern int write_flash_byte(char data, int offset);
extern void delete_flash_block(int blockno);
extern int init_28f008sa(void);

#endif
#endif
