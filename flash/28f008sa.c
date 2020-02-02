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
 * Intel 28f008sa flash chip code.
 *
 *  david.rusling@reo.mts.dec.com
 */
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/pci.h>
#include <linux/version.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/console.h>
#include <asm/hwrpb.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <stdarg.h>

#include "milo.h"
#include "impure.h"
#include "uart.h"
#include "fs.h"
#include "romhead.h"
#include "28f008sa.h"

#ifdef I28F008SA

/* ---------- F L A S H ---- I O ----- R O U T I N E S -----------------------*/
static inline void OUTFLASH(unsigned char d, unsigned int offset)
{
	if (offset >= FLASH_BIT_SWITCH_OFFSET) {
		outb(0x01, 0x800);
		offset -= FLASH_BIT_SWITCH_OFFSET;
	} else
		outb(0x00, 0x800);

	/* write the byte */
	writeb(d, FLASH_BASE + offset);
}

static inline unsigned char INFLASH(unsigned int offset)
{
	unsigned char value;

	if (offset >= FLASH_BIT_SWITCH_OFFSET) {
		outb(0x01, 0x800);
		offset -= FLASH_BIT_SWITCH_OFFSET;
	} else
		outb(0x00, 0x800);

	/* read the byte */
	value = readb(FLASH_BASE + offset);

	return value;
}

/* relies on flash_base to be correctly set */
unsigned char read_flash_byte(int offset)
{

	/* set the device into read mode */
	OUTFLASH(0xff, 0);
	return INFLASH(offset);
}

/* relies on flash_base to be correctly set */
int write_flash_byte(char data, int offset)
{
	int status;
	int tries;

	tries = 3;
	while (tries--) {

		/* set the device into write mode */
		OUTFLASH(0x40, offset);

		/* write the data */
		OUTFLASH(data, offset);

		/* successful? */
		do {
			OUTFLASH(0x70, offset);
			status = INFLASH(offset);
		} while ((status & 0x80) != 0x80);

		/* a write error? */
		if ((status & 0x10) == 0x10) {
			status = FALSE;
		} else
			return TRUE;
	}

	printk("ERROR: writing byte, status is 0x%02x\n", status);
	return status;
}

void delete_flash_block(int blockno)
{
	unsigned int status;
	unsigned int offset = (blockno * FLASH_BLOCK_SIZE);

	/* write delete block command followed by confirm */
	OUTFLASH(0x20, offset);
	OUTFLASH(0xd0, offset);

	/* check the status */
	do {
		OUTFLASH(0x70, offset);
		status = INFLASH(offset);
	} while ((status & 0x80) != 0x80);
	if ((status & 0x20) == 0x20) {
		printk("ERROR: deleting flash block, status is 0x%02x\n",
		       status);
		return;
	}
}

/* ---------- F L A S H --- S U P P O R T --- R O U T I N E S ----------------*/
/*
 *  Check to see if there is some flash at the location specified.
 */
static int check_flash_id(void)
{
	unsigned char man, id;

	/* read the manufacturer's id */
	OUTFLASH(0x90, 0);
	man = INFLASH(0);

	/* correct manufacturer's id? */
	if (man != 0x89) {
		return FALSE;
	}

	/* read the device id */
	id = INFLASH(1);

	/* correct device id? */
	if (id < 0xA1) {
		return FALSE;
	}

	/* whoopee, we found it */
	printk("Flash device is an Intel 28f008SA\n");
	printk("  %d segments, each of 0x%x (%d) bytes\n", FLASH_BLOCKS,
	       FLASH_BLOCK_SIZE, FLASH_BLOCK_SIZE);
	return TRUE;
}

int init_28f008sa(void)
{
	int status;

	/* set the HAXR */
	set_hae(FLASH_BASE);

	/* set address bit 19 */
	outb(0x00, 0x800);

	/* try and figure out the flash id */
	status = check_flash_id();

	return status;
}

#endif
