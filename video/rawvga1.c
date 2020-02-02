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

/* Actually, a bunch of the code below has been hacked and reverse
   engineered such that it no longer requires BIOS emulation to fire
   up a VGA card.  It uses direct register sets.

   Copyright 1998 by Andrew P. Lentvorski, Jr.
   Digital Equipment Corporation
*/

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/bios32.h>
#include <asm/io.h>

#include "milo.h"
#include "video.h"
#include "vgavideo.h"

static int vgaraw_init(void);

struct bootvideo vgavideo = {
	"RAW VGA",
	&vgaraw_init,
	&vga_putchar
};

#define VGARAW_FONT 0xA0000UL
#define VGARAW_TEXT 0xB8000UL

/* These should probably be in a header somewhere */
#define VGARAW_MISC_OUTPUT_WRITE 0x3C2

/* Color Emulation Enabled */
#define VGARAW_FEATURE_CTL_WRITE 0x3DA
#define VGARAW_FEATURE_CTL_READ 0x3CA

#define VGARAW_INPUT_STAT1_READ 0x3DA

#define VGARAW_MISC_OUTPUT_READ 0x3CC

#define VGARAW_SEQ_INDEX_WRITE 0x3C4
#define VGARAW_SEQ_DATA_WRITE  0x3C5

#define VGARAW_SEQ_INDEX_READ 0x3C4
#define VGARAW_SEQ_DATA_READ  0x3C5

/* Color Emulation Enabled */
#define VGARAW_CRTC_INDEX_WRITE 0x3D4
#define VGARAW_CRTC_DATA_WRITE 0x3D5

#define VGARAW_CRTC_INDEX_READ 0x3D4
#define VGARAW_CRTC_DATA_READ 0x3D5

#define VGARAW_CRTC_VRETRACE_END_INDEX 0x11
#define VGARAW_CRTC_CURSOR_HIGH_INDEX 0x0E
#define VGARAW_CRTC_CURSOR_LOW_INDEX 0x0F

#define VGARAW_GCTL_INDEX_WRITE 0x3CE
#define VGARAW_GCTL_DATA_WRITE 0x3CF

#define VGARAW_GCTL_INDEX_READ 0x3CE
#define VGARAW_GCTL_DATA_READ 0x3CF

#define VGARAW_ACTL_WRITE 0x3C0
#define VGARAW_ACTL_READ 0x3C1


/* These init values have been extracted from
   Mode 7 (Text, 80x25, 8x16 Font) initiated
   by a BIOS call on an 8MB Millenium II card.
   Hey, it works for me.  If you want to add
   more VGA detection code, feel free
*/

static unsigned char vgaraw_seq_data[5] = { 0x03, 0x00, 0x03, 0x00, 0x02 };

static int vgaraw_crtc_data[25] = {
	0x60, 0x4f, 0x50, 0x83, 0x55, 0x81, 0xbf, 0x1f,
	0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x02, 0x68,
	0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
	0xff
};

static int vgaraw_gctl_data[9] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00,
	0xff
};

static int vgaraw_actl_data[21] = {
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x04, 0x00, 0x0f, 0x08, 0x00
};

static inline unsigned char INB(unsigned long port)
{
	return inb(port);
}

static inline void OUTB(unsigned long port, unsigned char byte)
{
	outb_p(byte, port);
	mb();
}

static int reset_vga_regs()
{
	unsigned char byte1, byte2, byteval, index, currdata;
	unsigned long portnum;

#ifdef DEBUG_PRINT
	printk("Attempting VGA Mode 7 Full Init...\n");
#endif

	portnum = VGARAW_MISC_OUTPUT_WRITE;
	byteval = 0x67;
#ifdef DEBUG_PRINT
	printk("Writing %02X to port %04lX...", byteval, portnum);
#endif
	OUTB(portnum, byteval);
#ifdef DEBUG_PRINT
	printk("done.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Attempting Sequence Register Inits...Mode 7\n");
#endif
	for (index = 0; index < 5; index++) {
#ifdef DEBUG_PRINT
		printk("%02X:%02X...", index, vgaraw_seq_data[index]);
#endif
		OUTB(VGARAW_SEQ_INDEX_WRITE, index);
		OUTB(VGARAW_SEQ_DATA_WRITE, vgaraw_seq_data[index]);

		byte1 = INB(VGARAW_SEQ_DATA_READ);

		if (byte1 != vgaraw_seq_data[index]) {
			printk("SEQ Comparison Fail: W:A %02X:%02X \n",
			       vgaraw_seq_data[index], byte1);
			return FALSE;
		}
	}
#ifdef DEBUG_PRINT
	printk("done\n");
#endif

#ifdef DEBUG_PRINT
	printk("Attempting CRTC Register Inits...Mode 7\n");

	printk("Disabling CRTC protection...\n");
#endif

	OUTB(VGARAW_CRTC_INDEX_WRITE, VGARAW_CRTC_VRETRACE_END_INDEX);
	byte1 = INB(VGARAW_CRTC_INDEX_READ);
	byte2 = INB(VGARAW_CRTC_DATA_READ);

#ifdef DEBUG_PRINT
	printk("Now: @%02X:%02X...", byte1, byte2);
#endif

	OUTB(VGARAW_CRTC_INDEX_WRITE, VGARAW_CRTC_VRETRACE_END_INDEX);
	OUTB(VGARAW_CRTC_DATA_WRITE, 0x60);

	byte1 = INB(VGARAW_CRTC_INDEX_READ);
	byte2 = INB(VGARAW_CRTC_DATA_READ);

#ifdef DEBUG_PRINT
	printk("Written: @%02X:%02X...", byte1, byte2);
#endif

	if ((byte2 & 0x80) == 0x00) {
#ifdef DEBUG_PRINT
		printk("\nCRTC Write Protection Disabled\n");
#endif
	} else {
		printk("\nCAUTION: CRTC STILL WRITE PROTECTED.");
	}

#ifdef DEBUG_PRINT
	printk("Starting CRTC Mode 7 Init.\n");
#endif
	for (index = 0; index < 25; index++) {
#ifdef DEBUG_PRINT
		printk("%02X:%02X...", index, vgaraw_crtc_data[index]);
#endif
		if (vgaraw_crtc_data[index] != -1) {
			OUTB(VGARAW_CRTC_INDEX_WRITE, index);
			OUTB(VGARAW_CRTC_DATA_WRITE,
			     vgaraw_crtc_data[index]);
		}

		byte1 = INB(VGARAW_CRTC_DATA_READ);

		if (byte1 != vgaraw_crtc_data[index]) {
			printk("CRTC Comparison Fail: W:A %02X:%02X \n",
			       vgaraw_crtc_data[index], byte1);
			return FALSE;
		}
#ifdef DEBUG_PRINT
		if (index % 8 == 7)
			printk("\n");
#endif
	}
#ifdef DEBUG_PRINT
	printk("done\n");
	printk("CRTC Write Protection Reenabled.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Setting Graphics Controller Registers\n");
#endif
	for (index = 0; index < 9; index++) {
		OUTB(VGARAW_GCTL_INDEX_WRITE, index);

		if (vgaraw_gctl_data[index] != -1) {
			OUTB(VGARAW_GCTL_DATA_WRITE,
			     vgaraw_gctl_data[index]);
		}

		byte1 = INB(VGARAW_GCTL_INDEX_READ);
		byte2 = INB(VGARAW_GCTL_DATA_READ);

#ifdef DEBUG_PRINT
		printk("%02X:%02X...", byte1, byte2);
#endif

		if (byte2 != vgaraw_gctl_data[index]) {
			printk("GCTL Comparison Fail: W:A %02X:%02X \n",
			       vgaraw_gctl_data[index], byte1);
			return FALSE;
		}

		if (index % 8 == 7)
			printk("\n");
	}
#ifdef DEBUG_PRINT
	printk("done\n");
#endif

#ifdef DEBUG_PRINT
	printk("Writing Attribute Control Registers\n");
#endif
	for (index = 0; index < 0x15; index++) {
		// Clear the attribute control register FF
		// This sets VGARAW_ACTL_READ/WRITE to address mode
		byte1 = INB(VGARAW_INPUT_STAT1_READ);

		// This drops the appropriate address into
		// the address register
		currdata = 0x20 | (index & 0x1F);
		OUTB(VGARAW_ACTL_WRITE, currdata);

		// The index has been loaded and the FF toggled state
		// Now the data can be pushed to the register
		if (vgaraw_actl_data[index] != -1) {
			OUTB(VGARAW_ACTL_WRITE, vgaraw_actl_data[index]);
		}

		currdata = INB(VGARAW_ACTL_READ);

#ifdef DEBUG_PRINT
		printk("%02X:%02X...", index, currdata);
#endif

		if (currdata != vgaraw_actl_data[index]) {
			printk("ACTL Comparison Fail: W:A %02X:%02X \n",
			       vgaraw_actl_data[index], currdata);
			return FALSE;
		}

		if (index % 8 == 7)
			printk("\n");
	}
#ifdef DEBUG_PRINT
	printk("done\n");
#endif

	return TRUE;
}

void vga_font_init(void)
{
	unsigned int j;
	unsigned int *ip;

#ifdef DEBUG_PRINT
	printk("Attempting to get at font information...\n");
#endif

#ifdef DEBUG_PRINT
	printk("Shutting down odd/even mode...");
#endif
	outb(0x05, VGARAW_GCTL_INDEX_WRITE);
	outb(0x00, VGARAW_GCTL_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("GCTL Shutdown Done...");
#endif

	outb(0x04, VGARAW_SEQ_INDEX_WRITE);
	outb(0x06, VGARAW_SEQ_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("SEQ Shutdown Done.\n");
	printk
	    ("This value also just set extended memory.  Be Forewarned.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Setting byte access mode...");
#endif
	outb(0x17, VGARAW_CRTC_INDEX_WRITE);
	outb(0xE3, VGARAW_CRTC_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("Done.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Attempting Memory Map Change...");
#endif
	outb(0x06, VGARAW_GCTL_INDEX_WRITE);
	outb(0x00, VGARAW_GCTL_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("Done.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Attempting plane 2 read mode access change...");
#endif
	outb(0x04, VGARAW_GCTL_INDEX_WRITE);
	outb(0x02, VGARAW_GCTL_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("Done.\n");
#endif

#ifdef DEBUG_PRINT
	printk("Access enabled to bit plane 2 (I hope).\n");

	printk("Attempting write access...\n");

	printk("Setting write access to bit plane 2...");
#endif
	outb(0x02, VGARAW_SEQ_INDEX_WRITE);
	outb(0x04, VGARAW_SEQ_DATA_WRITE);
#ifdef DEBUG_PRINT
	printk("Done.\n");
#endif


#ifdef DEBUG_PRINT
	printk("Writing font data...");
#endif

	ip = (unsigned int *) console_font;
	for (j = 0xa0000; j < 0xa2000; ip += 4, j += 32) {
		__writel(ip[0], j);
		__writel(ip[1], j + 4);
		__writel(ip[2], j + 8);
		__writel(ip[3], j + 12);
	}

#if 0
	addr = 0xA0000;
	while (addr < 0xA2000) {
		writeb(bitplane2[addr - 0xA0000], addr);
		mb();

#ifdef DEBUG_PRINT
		if ((addr % 16) == 0) {
			printk("\n%08lX:", addr);
		}

		byte1 = readb(addr);

		printk(" %02X", byte1);
#endif

		addr++;
	}
#endif

#ifdef DEBUG_PRINT
	printk("Done.\n");
#endif

}

/* *************************************************************
   Subroutine dump_vga_regs()
   
   Does exactly what you expect.  Dumps all VGA registers out.
   Useful for debugging.
   *************************************************************
*/

#if 0
void dump_vga_regs(void)
{
	unsigned char byte1, byte2, currdata, index;

	printk("Attempting vga register reads\n");
	byte1 = INB(VGARAW_MISC_OUTPUT_READ);
	printk("Misc output = %04x: %04X\n",
	       VGARAW_MISC_OUTPUT_READ, byte1);
	byte1 = INB(VGARAW_FEATURE_CTL_READ);
	printk("Feature ctl = %04X: %04X\n",
	       VGARAW_FEATURE_CTL_READ, byte1);

	printk("Reading Sequence Registers\n");
	for (index = 0; index < 5; index++) {
		OUTB(VGARAW_SEQ_INDEX_WRITE, index);

		byte1 = INB(VGARAW_SEQ_INDEX_READ);
		byte2 = INB(VGARAW_SEQ_DATA_READ);
		printk("%02X:%02X...", byte1, byte2);
	}
	printk("done\n");

	printk("Reading CRTC Registers\n");
	for (index = 0; index < 25; index++) {
		OUTB(VGARAW_CRTC_INDEX_WRITE, index);

		byte1 = INB(VGARAW_CRTC_INDEX_READ);
		byte2 = INB(VGARAW_CRTC_DATA_READ);

		printk("%02X:%02X...", byte1, byte2);
		if (index % 8 == 7)
			printk("\n");
	}
	printk("done\n");

	printk("Reading Graphics Controller Registers\n");
	for (index = 0; index < 9; index++) {
		OUTB(VGARAW_GCTL_INDEX_WRITE, index);

		byte1 = INB(VGARAW_GCTL_INDEX_READ);
		byte2 = INB(VGARAW_GCTL_DATA_READ);

		printk("%02X:%02X...", byte1, byte2);

		if (index % 8 == 7)
			printk("\n");
	}
	printk("done\n");

	printk("Reading Attribute Control Registers\n");
	for (index = 0; index < 0x15; index++) {
		// Clear the attribute control register FF
		// This sets VGARAW_ACTL_READ/WRITE to address mode
		byte1 = INB(VGARAW_INPUT_STAT1_READ);

		/* This is an explicit memory barrier in order to wait */
		/* for the FF to reset itself */
		mb();

		/* A second memory barrier to stop Pyxis problems? */
		mb();

		// This drops the appropriate address into
		// the address register
		currdata = 0x20 | (index & 0x1F);

		OUTB(VGARAW_ACTL_WRITE, currdata);

		// The index has been loaded and the FF toggled state
		// Now the data can be sucked from the register

		currdata = INB(VGARAW_ACTL_READ);

		printk("%02X:%02X...", index, currdata);
		if (index % 8 == 7)
			printk("\n");
	}
	printk("done\n");

}

#endif

/*
 *-----------------------------------------------------------------------
 * vgaraw_init --
 *      Handle the initialization of VGA register structure.
 *      Does NOT access the BIOS structure.  Thus it has to go
 *      after the registers and memory directly via the chipset
 *
 * Input:
 *      None
 *
 * Results:
 *      VGA inititalised.
 *
 * Side Effects: 
 *
 *      None.
 *
 *-----------------------------------------------------------------------
 */
static int vgaraw_init(void)
{
	unsigned char bus, devfn;
	unsigned int i;

	printk("Entering vgaraw_init\n");

	/* Check if a VGA is installed */
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

	/* Put registers in semi-consistent state */
	if (!reset_vga_regs())
		return FALSE;

	vga_font_init();

	/* Don't take the chance of something wierd getting smashed */
	if (!reset_vga_regs())
		return FALSE;

	/* Clear and reset the screen */
	for (i = 0; i < 25 * 80; i++)
		writew(' ' | (7 << 8), 0xb8000U + i * 2);
	vga.row = vga.col = 0;
	vga.nrows = 25;
	vga.ncols = 80;
	vga.video_base = 0xb8000;

	return TRUE;
}
