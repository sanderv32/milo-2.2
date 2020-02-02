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
 *  Evaluation board flash updating code.
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
#include "flash.h"
#include "28f008sa.h"

#ifdef I28F008SA

static int eb_init(void);
static void eb_list(void);
static void eb_program(void);
static void eb_delete_image(char *image);
static void eb_delete_block(int blockno);

static void eb_env(int blockno);

flash_t Flash = {
	"Evaluation Board",

	eb_init,
	eb_program,

	eb_list,
	eb_env,
	eb_delete_block,
	eb_delete_image,
};


static char *blocks = NULL;

#define BLOCK_IN_USE(n) (blocks[n] != ((char) -1))

/*
 *  The evaluation boards with flash (EB66+, AlphaPC64, EB164)
 *  all follow the following layout of their 16 block flash...
 *
 *       Block 0 - 2     Debug Monitor
 *       Block 3 - 7     ARC NT Firmware
 *       Block 8 - 13    SRM Console Firmware
 *       Block 14        SRM Environment
 *       Block 15        NT Environment
 */
/*
 *  Image types are:
 *
 *  0   - dbm
 *  1   - NT
 *  2/3 - SRM console
 *  7   - Milo/Linux
 *
 */
static char *names[8] = {
	"Alpha Evaluation Board Debug Monitor",
	"Windows NT ARC",
	"VMS",
	"Digital Unix",
	"Unknown",
	"Unknown",
	"Unknown",
	"Milo/Linux",
};

extern unsigned char flash_image[];
extern unsigned int flash_image_size;
extern int yes_no(char *string);

static void print_user(int id)
{
	switch (id) {
	case 0:
		printk("DBM");
		break;
	case 1:
		printk("WNT");
		break;
	case 2:
	case 3:
		printk("SRM");
		break;
	case 7:
		printk("MILO");
		break;
	default:
		printk("U");
		break;
	}
	printk(" ");
}

/* look for an image more than one block long */
static int find_image(int imageno)
{
	int i;

	for (i = 0; i < FLASH_BLOCKS; i++) {
		if (blocks[i] == imageno) {
			if (i == FLASH_BLOCKS - 1)
				return -1;
			if (blocks[i + 1] == imageno)
				return i;
		}
	}

	return -1;
}

int valid_flash_header(romheader_t * header)
{
	if ((header->romh.V0.signature == ROM_H_SIGNATURE) &&
	    (header->romh.V0.csignature ==
	     (unsigned int) ~ROM_H_SIGNATURE)) return TRUE;
	else
		return FALSE;
}

void read_flash_header(romheader_t * header, int offset)
{
	unsigned char *buffer = (unsigned char *) header;
	int j;

	for (j = 0; j < sizeof(romheader_t); j++) {
		buffer[j] = read_flash_byte(offset + j);
	}
}

static void print_flash_header(romheader_t * header)
{
	printk("    Firmware Id: %2d (%s)\n", header->romh.V1.fw_id,
	       names[header->romh.V1.fw_id & 7]);
	printk("    Image size is %d bytes (%d blocks)\n",
	       header->romh.V1.rimage_size,
	       ((header->romh.V1.rimage_size + header->romh.V0.hsize)
		/ FLASH_BLOCK_SIZE) + 1);
	printk("    Executing at 0x%x\n", header->romh.V0.destination.low);
}

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

static void scan_blocks(void)
{
	int blockno;
	romheader_t header;
	int size;
	int i;

	printk("Scanning Flash blocks for usage\n");

	/* assume the worst */
	for (blockno = 0; blockno < FLASH_BLOCKS; blockno++)
		blocks[blockno] = (char) -1;

	/* look at every block in turn, trying to find a header. */
	for (blockno = 0; blockno < FLASH_BLOCKS; blockno++) {
		read_flash_header(&header, blockno * FLASH_BLOCK_SIZE);
		if (valid_flash_header(&header) == TRUE) {
			size =
			    ((header.romh.V1.rimage_size +
			      header.romh.V0.hsize) / FLASH_BLOCK_SIZE) +
			    1;
			for (i = blockno; i < (blockno + size); i++)
				blocks[i] = header.romh.V1.fw_id;
		}
	}

	/* depending on what's there, some blocks are not free */
	blockno = find_image(1);	/* windows NT */
	if (blockno >= 0)
		blocks[15] = 1;

	blockno = find_image(2);	/* VMS */
	if (blockno >= 0)
		blocks[14] = 2;

	blockno = find_image(3);	/* Digital Unix */
	if (blockno >= 0)
		blocks[14] = 3;

	i = find_nvram();
	if (i > 0)
		blocks[i] = 7;
}

/* ---------- F L A S H --- S U P P O R T --- R O U T I N E S ----------------*/
static void eb_delete_block(int blockno)
{
	if ((blockno < 0) || (blockno >= FLASH_BLOCKS)) {
		printk("ERROR: invalid block number\n");
		return;
	}

	if (blocks[blockno] == 0) {
		printk("ERROR: you cannot delete the Debug Monitor\n");
		return;
	}

	if (!yes_no("Do you really want to do this (y/N)? ")) {
		return;
	}

	printk("%d[D]", blockno);
	delete_flash_block(blockno);
	printk("\n");
	scan_blocks();
}

static void eb_delete_image(char *image)
{
	int imageno = -1;
	int blockno;

	if (strcmp(image, "dbm") == 0)
		imageno = 0;
	if (strcmp(image, "wnt") == 0)
		imageno = 1;
	if (strcmp(image, "milo") == 0)
		imageno = 7;

	if (imageno < 0)
		imageno = atoi(image);

	if (imageno == 0) {
		printk
		    ("ERROR: you cannot delete the Debug Monitor image\n");
		return;
	}

	blockno = find_image(imageno);
	if (blockno < 0) {
		printk("ERROR: no such image\n");
		return;
	}

	if (!yes_no("Do you really want to do this (y/N)? ")) {
		return;
	}

	for (blockno = blockno; blockno < FLASH_BLOCKS; blockno++) {
		if (blocks[blockno] == imageno) {
			printk("%d[D] ", blockno);
			delete_flash_block(blockno);
		} else
			break;
	}
	printk("\n");
	scan_blocks();
}

static void eb_program(void)
{
	int blockno, image_blocks, i, j, flash_offset, image_offset;
	romheader_t *header = (romheader_t *) flash_image;
	int status;

	/* check for a valid image */
	if (!valid_flash_header(header)) {
		printk("ERROR: invalid flash image\n");
		return;
	}

	/* print out the header */
	printk("Image is:\n");
	print_flash_header(header);

	/* is there an existing image? */
	blockno = find_image(header->romh.V1.fw_id);

	if (blockno >= 0) {
		printk("Found existing image at block %d\n", blockno);
		if (!yes_no("Overwrite existing image? (N/y)? "))
			blockno = -1;
		else {
			for (i = 0; i < FLASH_BLOCKS; i++) {
				if (blocks[i] == header->romh.V1.fw_id)
					blocks[i] = (char) -1;
			}
		}
	}
	if (blockno < 0) {
		char ibuff[20];

		printk("Please enter block number ");
		kbd_gets(ibuff, 20);
		blockno = atoi(ibuff);
		if ((blockno < 0) || (blockno > (FLASH_BLOCKS - 1))) {
			printk("ERROR: invalid block number\n");
			return;
		}
	}

	/* check to see if there's room for this image */
	image_blocks =
	    ((header->romh.V1.rimage_size + header->romh.V0.hsize) /
	     FLASH_BLOCK_SIZE) + 1;

	if ((blockno + image_blocks) > FLASH_BLOCKS) {
		printk("ERROR: not enough room for the image\n");
		scan_blocks();
		return;
	}

	/* check to see if the image would overwrite another image */
	for (i = blockno; i < (blockno + image_blocks); i++) {
		if BLOCK_IN_USE
			(i) {
			printk
			    ("ERROR: this would overwrite another image\n");
			return;
			}
	}

	/* does the user _really_ want to do this? */
	if (!yes_no("Do you really want to do this (y/N)? ")) {
		scan_blocks();
		return;
	}

	/* program the image a flash block at a time */
	printk("Programming image into flash\n");
	image_offset = 0;
	for (i = blockno; i < (blockno + image_blocks); i++) {

		/* first delete the block */
		printk("%d [D]", i);
		delete_flash_block(i);
		blocks[i] = (char) -1;

		/* now program it */
		printk("[P]");
		flash_offset = i * FLASH_BLOCK_SIZE;
		for (j = 0; j < FLASH_BLOCK_SIZE; j++) {
			status =
			    write_flash_byte(flash_image[image_offset],
					     flash_offset);
			if (!status) {
				printk
				    ("\nERROR: writing flash at offset %d\n",
				     flash_offset);
				scan_blocks();
				return;
			}

			if (read_flash_byte(flash_offset) !=
			    flash_image[image_offset]) {
				printk
				    ("ERROR: read byte (0x%x) does not match written byte (0x%x)\n",
				     read_flash_byte(flash_offset),
				     flash_image[image_offset]);
				printk("  at offset %d into the image\n",
				       image_offset);
				return;
			}

			image_offset++;
			flash_offset++;
		}

		/* move on the variables */
		printk(" ");
	}
	printk("\n");

	/* scan again */
	scan_blocks();
}

static void eb_list(void)
{
	int block;
	int imageno;
	romheader_t header;

	printk("Flash blocks: ");
	for (block = 0; block < FLASH_BLOCKS; block++) {
		printk("%2d:", block);
		print_user(blocks[block]);
	}
	printk("\n");

	printk("Listing flash Images\n");
	for (imageno = 0; imageno < 31; imageno++) {
		block = find_image(imageno);
		if (block >= 0) {
			printk("  Flash image starting at block %d:\n",
			       block);
			read_flash_header(&header,
					  block * FLASH_BLOCK_SIZE);
			print_flash_header(&header);
		}
	}
}

static void create_environment(int i)
{
	int offset;
	int status;
	char *marker = ENVIRON_ID;
	int j;

	/* try and find somewhere to put the environment variables */

	printk("Using block %d for environment variables\n", i);
	delete_flash_block(i);
	offset = i * FLASH_BLOCK_SIZE;
	for (j = 0; j < strlen(marker); j++) {
		status = write_flash_byte(marker[j], offset);
		if (!status) {
			printk("ERROR: writing environment block\n");
			scan_blocks();
			return;
		}
		offset++;
	}
	blocks[i] = 7;
}

static void eb_env(int blockno)
{
	int i;

	/* valid block number? */
	if ((blockno < 0) || (blockno >= FLASH_BLOCKS)) {
		printk("ERROR: invalid block number\n");
		return;
	}

	/* find the current nvram block, if there is one */
	i = find_nvram();
	if (i >= 0) {
		printk
		    ("Block %d already contains the environment variables\n",
		     i);
		return;
	}

	/* already in use? */
	if BLOCK_IN_USE
		(blockno) {
		printk("ERROR: that block is already in use\n");
		return;
		}

	/* make an environment block */
	create_environment(blockno);
}

static int eb_init(void)
{
	int status;
	int block;

	status = init_28f008sa();

	if (status) {

		/* scan the flash blocks for images */
		blocks = (char *) kmalloc(FLASH_BLOCKS, 1);
		scan_blocks();

		/* look for some environment variables */
		block = find_nvram();
		if (block >= 0) {
			printk
			    ("Block %d contains the environment variables\n",
			     block);
			blocks[block] = 7;
		}
	}

	return status;
}

#endif
