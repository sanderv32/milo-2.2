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
/* Routines to access the NVRAM on NoName/EB64/EB66.  The NVRAM on these
 * platforms is a strange creature... it is essentially a bank-switched
 * device, with 8Kb of data divided into 32 256-byte banks.  The bank
 * number is selected by writing it to I/O port address 0xC00.  The byte
 * within that bank is then selected by reading or writing to an I/O port
 * in the range 0x800 - 0x8ff.
 */

#include <linux/config.h>
#include "milo.h"

#if defined(MINI_NVRAM)
#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <linux/kernel.h>
#include <linux/pci.h>

#include "flash/28f008sa.h"

#ifdef MINI_ALPHA_NONAME

#define NVRAM_ADDRHI	0xC00
#define NVRAM_ADDRLO	0x800

#define NVRAM_BASE      1024*5

/* Copy data in from NVRAM.
 *
 *	offset - Offset in the NVRAM to start copying
 *	dest   - Destination address for the data
 *	size   - Number of bytes to copy.
 */
void nvram_copyin(int offset, char *dest, int size)
{
	int i;
	char *dp;
	char ubcs_b;
	char verify;

	offset += NVRAM_BASE;

	/* We have enable I/O to the NVRAM ports.  To do this,
	 * we have to set an enable bit in the SIO's Utility
	 * Bus Chip Select Enable register.
	 */
	pcibios_read_config_byte(0, 0x38, 0x4f, &ubcs_b);
	ubcs_b |= 0x80;
	pcibios_write_config_byte(0, 0x38, 0x4f, ubcs_b);
	pcibios_read_config_byte(0, 0x38, 0x4f, &verify);
	if (verify != ubcs_b) {
		printk
		    ("nvram_copyin: SIO config failed: expect 0x%x got 0x%x\n",
		     ubcs_b, verify);
	}

	for (i = 0, dp = dest; i < size; i++, dp++, offset++) {
		outb((offset >> 8) & 0x1f, NVRAM_ADDRHI);
		*dp = inb(NVRAM_ADDRLO + (offset & 0xff));
	}
}

/* Copy data out to NVRAM.
 *
 *	offset - Offset in the NVRAM to start copying
 *	src    - Address of the data to copy out.
 *	size   - Number of bytes to copy.
 */
void nvram_copyout(int offset, char *src, int size)
{
	int i;
	char *dp;
	char ubcs_b;
	char verify;

	offset += NVRAM_BASE;

	/* We have enable I/O to the NVRAM ports.  To do this,
	 * we have to set an enable bit in the SIO's Utility
	 * Bus Chip Select Enable register.
	 */
	pcibios_read_config_byte(0, 0x38, 0x4f, &ubcs_b);
	ubcs_b |= 0x80;
	pcibios_write_config_byte(0, 0x38, 0x4f, ubcs_b);
	pcibios_read_config_byte(0, 0x38, 0x4f, &verify);
	if (verify != ubcs_b) {
		printk
		    ("nvram_copyin: SIO config failed: expect 0x%x got 0x%x\n",
		     ubcs_b, verify);
	}


	for (i = 0, dp = src; i < size; i++, dp++, offset++) {
		outb((offset >> 8) & 0x1f, NVRAM_ADDRHI);
		outb(*dp, NVRAM_ADDRLO + (offset & 0xff));
	}
}

#else				/* not NONAME */

/* find the stuff */
static int find_nvram()
{
	int block, i, offset;
	unsigned char buffer[80];
	char *id = ENVIRON_ID;

	offset = 0;
	for (block = 0; block < FLASH_BLOCKS; block++) {
		for (i = 0; i < strlen(id); i++) {
			buffer[i] = read_flash_byte(offset + i);
		}
		if (strncmp(buffer, id, strlen(id)) == 0)
			return block;
		offset += FLASH_BLOCK_SIZE;
	}
	return -1;
}
/* Copy data in from NVRAM.
 *
 *	offset - Offset in the NVRAM to start copying
 *	dest   - Destination address for the data
 *	size   - Number of bytes to copy.
 */
static int flash_block = -1;
void nvram_copyin(int offset, char *dest, int size)
{
	int where;

	if (flash_block < 0) {
		/* first init the flash */
		if (!(init_28f008sa())) {
#if 0
			printk("ERROR: failed to setup the NVRAM\n");
#endif
			return;
		}

		flash_block = find_nvram();
		if (flash_block < 0) {
#if 0
			printk("ERROR: failed to find the NVRAM\n");
#endif
			return;
		}
	}

	/* now copy it */
	printk("Found NVRAM in block %d\n", flash_block);
	where = flash_block * FLASH_BLOCK_SIZE;
	while (size--)
		*dest++ = read_flash_byte(where++);
}

/* Copy data out to NVRAM.
 *
 *	offset - Offset in the NVRAM to start copying
 *	src    - Address of the data to copy out.
 *	size   - Number of bytes to copy.
 */
void nvram_copyout(int offset, char *src, int size)
{
	int where;
	int status;

	if (flash_block >= 0) {
		/* now copy it */
		where = flash_block * FLASH_BLOCK_SIZE;
		delete_flash_block(flash_block);
		while (size--) {
			status = write_flash_byte(*src++, where++);
			if (!status) {
				printk
				    ("Failed to write NVRAM variables\n");
				return;
			}
		}
	}
}
#endif				/* not NONAME */
#endif
