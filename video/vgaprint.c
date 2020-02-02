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
/* The following code was stolen from OSF, but it's really Thomas Roell's
 * VGA code...
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 */

/*
 * Changes by Nikita Schmidt; please diff this file against the original source
 * code at gatekeeper.dec.com to see the changes made.
 */

#include <asm/io.h>

#include "vgavideo.h"

#define OUTB(port, val)		{outb_p((val), (unsigned long)(port)); mb();}
#define MINW(port)		readw((unsigned long)(port))
#define MOUTW(port, val)	{writew((val), (unsigned long)(port)); mb();}

struct vga vga;

static void vga_scrollscreen(void)
{
	int i;
	unsigned int ioword;
	unsigned long source_tx_addr, video_addr;

	/* Scroll the screen by copying the entire text buffer up one line. */
	video_addr = vga.video_base;
	source_tx_addr = video_addr + vga.ncols * 2;

	for (i = 0; i < (vga.nrows - 1) * vga.ncols; i++) {
		ioword = MINW(source_tx_addr);
		MOUTW(video_addr, ioword);
		video_addr += 2;
		source_tx_addr += 2;
	}

	/* Clear the last row */
	ioword = (unsigned short int) ' ';
	ioword = ioword | (0x07 << 8);

	for (i = 0; i < vga.ncols; i++) {
		MOUTW(video_addr, ioword);
		video_addr += 2;
	}

	/* Position cursor at bottom of cleared row */
	vga.row = (vga.nrows - 1);
	vga.col = 0;
}

/*
 *-----------------------------------------------------------------------
 * vgabios_putchar --
 *      Put a character out to screen memory.
 *-----------------------------------------------------------------------
 */
void vga_putchar(unsigned char c)
{
	int cursorloc;
	unsigned int outword;
	unsigned long video_addr;

	if (c == '\n') {

		/* just go onto the next row */
		vga.row++;
		vga.col = 0;
	} else if (c == '\r') {

		/* Go to beginning of current row */
		vga.col = 0;
	} else if (c == '\b') {

		/* Backspace: go back one col unless we're already at beginning of
		 * row... */
		if (vga.col > 0)
			vga.col--;
	} else {

		/* build the character */
		outword = (unsigned short int) c;
		outword = outword | (0x07 << 8);

		/* output it to the correct location */
		video_addr =
		    vga.video_base + (vga.row * vga.ncols + vga.col) * 2;
		MOUTW(video_addr, outword);

		/* move on where to write the next character */
		vga.col++;
	}

	/* check for rows and cols overflowing */
	if (vga.col > vga.ncols - 1) {
		vga.col = 0;
		vga.row++;
	}
	if (vga.row > vga.nrows - 1) {
		vga_scrollscreen();
	}

	/* Put the cursor at the right spot... */
	cursorloc = ((vga.row * vga.ncols) + vga.col);
	OUTB(0x3d4, 14);
	OUTB(0x3d5, (cursorloc & 0xff00) >> 8);
	OUTB(0x3d4, 15);
	OUTB(0x3d5, (cursorloc & 0xff));

}
