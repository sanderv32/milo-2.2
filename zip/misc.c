/*
 * misc.c
 * 
 * This is a collection of several routines from gzip-1.0.3 
 * adapted for Linux.
 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 * puts by Nick Holloway 1993
 *
 * Adapted to Linux/Alpha boot by David Mosberger (davidm@cs.arizona.edu).
 */
#include <linux/kernel.h>

#include <asm/hwrpb.h>
#include <asm/page.h>
#include <asm/system.h>

#include "gzip.h"
#include "milo.h"
#include "fs.h"

extern struct bootfs *bfs;
extern char *dest_addr;
extern long bytes_to_copy;

unsigned char *inbuf;
unsigned char *window;
unsigned outcnt;
unsigned insize;
unsigned inptr;
unsigned long bytes_out;
int method;

static char *dest_addr;
static int input_fd = -1;

/*
 * Clear input and output buffers
 */
static int block_number = 0;
void clear_bufs(void)
{
	block_number = 0;
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
		if ((flags & ENCRYPTED) != 0) {
			printk("UNZIP: Input is encrypted\n");
			return -1;
		}
		if ((flags & CONTINUATION) != 0) {
			printk("UNZIP: Multi part input\n");
			return -1;
		}
		if ((flags & RESERVED) != 0) {
			printk("UNZIP: input has invalid flags\n");
			return -1;
		}
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
	}
	return method;
}


/*
 * Fill the input buffer and return the first byte in it. This is called
 * only when the buffer is empty and at least one byte is really needed.
 */

int fill_inbuf(void)
{
#ifdef BOOT_VERBOSE
	if ((block_number % 5) == 0)
		printk("#");
#endif

	__bread(input_fd, block_number++, inbuf);

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
#if 0
	printk("flush_window: called\n");
#endif

	if (!outcnt) {
		return;
	}

	bytes_out += outcnt;

	if (dest_addr) {
		/*
		 * Simply copy adjacent to previous block:
		 */
		memcpy((char *) dest_addr, window, outcnt);
		dest_addr += outcnt;
	}
}

/*
 * We have to be careful with the memory-layout during uncompression.
 * The stack we're currently executing on lies somewhere between the
 * end of this program (given by _end) and lastfree.  However, as I
 * understand it, there is no guarantee that the stack occupies the
 * lowest page-frame following the page-frames occupied by this code.
 *
 * Thus, we are stuck allocating memory towards decreasing addresses,
 * starting with lastfree.  Unfortunately, to know the size of the
 * kernel-code, we need to uncompress the image and we have a circular
 * dependency.  To make the long story short: we put a limit on
 * the maximum kernel size at MAX_KERNEL_SIZE and allocate dynamic
 * memory starting at (lastfree << ALPHA_PG_SHIFT) - MAX_KERNEL_SIZE.
 */
int uncompress_kernel(int fd, void *where)
{
	input_fd = fd;

	inbuf = (unsigned char *) kmalloc(INBUFSIZ, 0);
	window = (unsigned char *) kmalloc(WSIZE, 0);
	dest_addr = (char *) where;

	clear_bufs();

	method = get_method();
	if (method < 0) {
		printk("UNZIP: this is not a zipped file\n");
		return -1;
	}
#ifdef BOOT_VERBOSE
	printk("UNZIP: this is a zipped file\n");
#endif

	unzip(0, 0);

#ifdef BOOT_VERBOSE
	printk("UNZIP: done.\n");
#endif
	return 0;
}
