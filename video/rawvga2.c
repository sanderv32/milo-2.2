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

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/bios32.h>
#include <asm/io.h>

#include "milo.h"
#include "video.h"
#include "vgavideo.h"

static int vgaraw_init(void);

struct bootvideo vgavideo2 = {
	"VGA",
	vgaraw_init,
	vga_putchar
};

#define INB(port)		inb((unsigned long)(port))
#define OUTB(port, val)		{outb_p((val), (unsigned long)(port)); mb();}
#define OUTW(port, val)		{outw((val), (unsigned long)(port)); mb();}

#define MINW(port)		readw((unsigned long)(port))
#define MOUTW(port, val)	{writew((val), (unsigned long)(port)); mb();}

static int vgaraw_init()
{
	unsigned char bus, devfn;
	unsigned int i, j;
	/* unsigned int oldmode, oldmisc, oldmask, oldmem; */
	unsigned int *ip;

	static unsigned char attrcontr[0x15] = {
		0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23,
		0x04, 0x00, 0x0f, 0x08, 0x00
	};
	static unsigned int crtc[] = {
		0x4f015f00, 0x82035002, 0x81055504, 0x1f07bf06,
		0x4f090008, 0x0f0b0e0a, 0x000d000c, 0x000f000e,
		0x8e119c10, 0x28138f12, 0x96151f14, 0xa317b916,
		0
	};
	static unsigned int palette[] = {
		0x000000, 0x2a0000, 0x002a00, 0x2a2a00,
		0x00002a, 0x2a002a, 0x002a2a, 0x2a2a2a,
		0x150000, 0x3f0000, 0x152a00, 0x3f2a00,
		0x15002a, 0x3f002a, 0x152a2a, 0x3f2a2a,
		0x001500, 0x2a1500, 0x003f00, 0x2a3f00,
		0x00152a, 0x2a152a, 0x003f2a, 0x2a3f2a,
		0x151500, 0x3f1500, 0x153f00, 0x3f3f00,
		0x15152a, 0x3f152a, 0x153f2a, 0x3f3f2a,
		0x000015, 0x2a0015, 0x002a15, 0x2a2a15,
		0x00003f, 0x2a003f, 0x002a3f, 0x2a2a3f,
		0x150015, 0x3f0015, 0x152a15, 0x3f2a15,
		0x15003f, 0x3f003f, 0x152a3f, 0x3f2a3f,
		0x001515, 0x2a1515, 0x003f15, 0x2a3f15,
		0x00153f, 0x2a153f, 0x003f3f, 0x2a3f3f,
		0x151515, 0x3f1515, 0x153f15, 0x3f3f15,
		0x15153f, 0x3f153f, 0x153f3f, 0x3f3f3f
	};

	printk("Entering vgaraw_init (alt.)\n");

	if (pcibios_find_class
	    (PCI_CLASS_NOT_DEFINED_VGA << 8, 0, &bus,
	     &devfn) == PCIBIOS_SUCCESSFUL
	    || pcibios_find_class(PCI_CLASS_DISPLAY_VGA << 8, 0, &bus,
				  &devfn) == PCIBIOS_SUCCESSFUL) {
		pcibios_read_config_dword(0, devfn, 0x00, &i);
		printk("device/vendor id=%x\n", i);
		pcibios_write_config_dword(bus, devfn, PCI_ROM_ADDRESS,
					   0xc0000 |
					   PCI_ROM_ADDRESS_ENABLE);
	}
	if (readw(0xc0000) != 0xaa55) {
		printk("vgaraw_init: sorry, there seems to be no BIOS\n");
		return FALSE;
	}

	OUTB(0x3c2, 0x67);	/* Miscellaneous output register */
	OUTB(0x3c6, 0xFF);	/* PEL mask */
	OUTB(0x3da, 0x00);	/* Feature control register */

	OUTW(0x3c4, 0x0300);	/* Sequencer */
	OUTW(0x3c4, 0x0001);	/* Sequencer: clocking mode */
	OUTW(0x3c4, 0x0402);	/* Sequencer: map mask */
	OUTW(0x3c4, 0x0003);	/* Sequencer: character map select */
	OUTW(0x3c4, 0x0604);	/* Sequencer: memory mode */

	OUTW(0x3d4, 0x0e11);	/* CRTC: enable writing to CRTC registers */
	for (i = 0; (j = crtc[i]); i++) {
		OUTW(0x3d4, j & 0xffff);
		OUTW(0x3d4, j >> 16);
	}
	OUTW(0x3d4, 0xff18);

	OUTW(0x3ce, 0x0000);	/* Graphics */
	OUTW(0x3ce, 0x0001);	/* Graphics */
	OUTW(0x3ce, 0x0002);	/* Graphics */
	OUTW(0x3ce, 0x0003);	/* Graphics: data rotate */
	OUTW(0x3ce, 0x0004);	/* Graphics */
	OUTW(0x3ce, 0x1005);	/* Graphics: mode */
	OUTW(0x3ce, 0x0406);	/* Graphics: miscellaneous */
	OUTW(0x3ce, 0x0007);	/* Graphics */
	OUTW(0x3ce, 0xff08);	/* Graphics: bit mask */

	INB(0x3da);
	mb();			/* Reset the 3c0 flip-flop */

	for (i = 0; i < sizeof attrcontr / sizeof(*attrcontr); i++) {
		OUTB(0x3c0, i);	/* Attribute controller */
		OUTB(0x3c0, attrcontr[i]);
	}

	OUTB(0x3c0, 0x20);	/* Enable screen output */
	INB(0x3da);
	mb();			/* Reset the 3c0 flip-flop */

	/* Write the palette */
	OUTB(0x3c8, 0);		/* PEL address write */
	for (i = 0; i < 64; i++) {
		j = palette[i];
		OUTB(0x3c9, j & 0xff);
		OUTB(0x3c9, (j >> 8) & 0xff);
		OUTB(0x3c9, j >> 16);
	}

	ip = (unsigned int *) console_font;
	for (j = 0xa0000; j < 0xa2000; ip += 4, j += 32) {
		__writel(ip[0], j);
		__writel(ip[1], j + 4);
		__writel(ip[2], j + 8);
		__writel(ip[3], j + 12);
	}

	/* Reset plane mapping for text mode */
	OUTW(0x3ce, 0x0e06);
	OUTW(0x3c4, 0x0302);
	OUTW(0x3c4, 0x0204);

	for (i = 0; i < 25 * 80; i++)
		writew(' ' | (7 << 8), 0xb8000U + i * 2);
	vga.row = vga.col = 0;
	vga.nrows = 25;
	vga.ncols = 80;
	vga.video_base = 0xb8000UL;

	return TRUE;
}
