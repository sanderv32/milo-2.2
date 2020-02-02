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
 *  Entry point for the Milo stub code.  This code does a few initialising
 *  jobs, unzips Milo and then jumps to it.
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
#include "zip/gzip.h"
#include "smc.h"

/*****************************************************************************
 *  Macros.                                                                  *
 *****************************************************************************/

#define STUB_VERSION	"1.2"

#ifdef DEBUG_STUB
#define STUBDEBUG(x...)	printk(x)
#else
#define STUBDEBUG(x...)	;
#endif

#define SHIFT  0xfffffc0018000000UL
#define BANK   3
void swap_banks(unsigned long bank);

struct bootvideo *milo_video = NULL;

extern char flash_image[];
extern unsigned int flash_image_size;

extern long bytes_to_copy;

unsigned char *inbuf;
unsigned char *window;
unsigned outcnt;
unsigned insize;
unsigned inptr;
unsigned long bytes_out;
int method;

static char *dest_addr;

extern unsigned long long _end;
char *free_memory = NULL;

#ifdef DEBUG_MALLOC
void *__kmalloc(size_t size, int priority, const char *file,
		const int line)
#else
void *kmalloc(size_t size, int priority)
#endif
{
	char *result;

	size += 8 - (size & 0x7);	/* align to an 8 byte boundary */
	result = free_memory;
	free_memory += size;

#ifdef DEBUG_MALLOC
	printk("kmalloc: returning 0x%x\n", result);
#endif

	return (void *) (result);
}

void free(void *where)
{
}

void video_putchar(unsigned char c)
{
}

void boot_main_cont(void)
{
}

/* needed by error () */
int kbd_getc()
{
	int i;

	for (i = (1 << 27); i != 0; i--);
	return 0;
}

/*****************************************************************************
 *   stubs to the unzipping code                                             *
 *****************************************************************************/
/*
 * Clear input and output buffers
 */
void clear_bufs(void)
{
	outcnt = 0;
	insize = inptr = 0;
	bytes_out = 0;
}

/*
 * Check the magic number of the input file and update ofname if an
 * original name was given and to_stdout is not set.
 * Return the compression method, -1 for error, -2 for warning.
 * Set inptr to the offset of the next byte to be processed.
 * This function may be called repeatedly for an input file consisting
 * of several contiguous gzip'ed members.
 * IN assertions: there is at least one remaining compressed member.
 *   If the member is a zip file, it must be the only one.
 */
static int get_method(void)
{
	unsigned char flags;
	char magic[2];		/* magic header */

	magic[0] = get_byte();
	magic[1] = get_byte();

	method = -1;		/* unknown yet */
	if (memcmp(magic, GZIP_MAGIC, 2) == 0
	    || memcmp(magic, OLD_GZIP_MAGIC, 2) == 0) {

		method = get_byte();
		flags = get_byte();
		if ((flags & ENCRYPTED) != 0)
			error("Input is encrypted\n");
		if ((flags & CONTINUATION) != 0)
			error("Multi part input\n");
		if ((flags & RESERVED) != 0)
			error("Input has invalid flags\n");
		get_byte();	/* skip over timestamp */
		get_byte();
		get_byte();
		get_byte();

		get_byte();	/* skip extra flags */
		get_byte();	/* skip OS type */

		if ((flags & EXTRA_FIELD) != 0) {
			unsigned len = get_byte();

			len |= get_byte() << 8;
			while (len--)
				get_byte();
		}

		/* Get original file name if it was truncated */
		if ((flags & ORIG_NAME) != 0) {
			/* skip name */
			while (get_byte() != 0)	/* null */
				;
		}

		/* Discard file comment if any */
		if ((flags & COMMENT) != 0) {
			while (get_byte() != 0)	/* null */
				;
		}
	} else {
		error("unknown compression method");
	}
	return method;
}


/*
 * Fill the input buffer and return the first byte in it. This is called
 * only when the buffer is empty and at least one byte is really needed.
 */
static int block_number = 0;
int fill_inbuf(void)
{

#ifdef DEBUG_STUB
	if ((block_number % 5) == 0)
		printk("#");
#endif

	memcpy(inbuf, flash_image + (block_number * INBUFSIZ), INBUFSIZ);
	block_number++;
	insize = INBUFSIZ;

	inptr = 1;
	return inbuf[0];
}


/*
 * Write the output window window[0..outcnt-1] holding uncompressed
 * data and update crc.
 */
void flush_window(void)
{
	STUBDEBUG("flush_window: called, dest_addr = 0x%lx\n",
		  (unsigned long) dest_addr);

	if (!outcnt) {
		return;
	}

	bytes_out += outcnt;

	if (dest_addr) {

		/* Simply copy adjacent to previous block: */
		memcpy((char *) dest_addr, window, outcnt);
		dest_addr += outcnt;
	}
}

void uncompress_milo(void *where)
{
	STUBDEBUG("Allocating memory for unzip\n");

	inbuf = (unsigned char *) kmalloc(INBUFSIZ, 0);
	window = (unsigned char *) kmalloc(WSIZE, 0);

	STUBDEBUG("uncompress_milo: inbuf @ 0x%lx, window @ 0x%lx\n",
		  (unsigned long) inbuf, (unsigned long) window);

	dest_addr = (char *) where;

	clear_bufs();

	method = get_method();
	unzip(0, 0);

	STUBDEBUG("done.\n");
}

/*****************************************************************************
 *   Main entry point for system primary bootstrap code                      *
 *****************************************************************************/
void __main(void)
{
	void (*milo_address) (void);

/*****************************************************************************
 *  Initialize.                                                              *
 *****************************************************************************/

	/* let's get explicit about some variables we care about. */
	block_number = 0;
	milo_address = (void (*)(void)) LOADER_AT;
	/* pick someplace we know is empty, for the decompression buffers. */
	/* START_ADDR is OK: that's where the full MILO will put kernel text. */
	free_memory = (char *) LOADER_AT - 0x100000;
	milo_video = NULL;

#ifdef DEBUG_STUB
#if defined(MINI_ALPHA_PC164) || defined(MINI_ALPHA_LX164)
/*
 *  The PC164 employs the SMC FDC37C93X Ultra I/O Controller.
 *  After reset all of the devices are disabled. SMCInit() will
 *  autodetect the FDC37C93X and enable each of the devices.
 */
	SMCInit();
#endif
#ifdef MINI_SERIAL_ECHO
	/* we can only output to the serial port *after* we initialise it */
	uart_init();
#endif
#endif

	STUBDEBUG("Milo Stub: V" STUB_VERSION "\n");

	/*
	memcpy(SHIFT, IDENT_ADDR, 0x1000000);
	STUBDEBUG("Now swapping banks...\n");
	swap_banks(BANK);
	*/

	/* unzip milo into the appropriate place in memory */
	STUBDEBUG("Unzipping Milo into position @ 0x%lx\n", LOADER_AT);
	uncompress_milo((void *) LOADER_AT);

	/* now jump to it */
	STUBDEBUG("Jumping to Milo...\n");
	(milo_address) ();
}
