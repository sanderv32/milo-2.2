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
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/kd.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>

#include "milo.h"
#include "video.h"
#include "vgavideo.h"

static int vgabios_init(void);

struct bootvideo decbiosvideo = {
	"BIOS VGA (Digital)",

	&vgabios_init,
	&vga_putchar
};


#define VGA_FB_ADDR     0xa0000UL
#define VGA_TX_ADDR     0xb8000UL

extern void bcopy(char *dst, char *src, int count);

/*
 *  The following definition controls debug printing.
 */
/* #define DEBUG_PRINT 1   */

#define INB(port)		inb((unsigned long)(port))
#define OUTB(port, val)		{outb_p((val), (unsigned long)(port)); mb();}
#define OUTW(port, val)		{outw((val), (unsigned long)(port)); mb();}

#define MINW(port)		readw((unsigned long)(port))
#define MOUTW(port, val)	{writew((val), (unsigned long)(port)); mb();}

/*
 *  Use Digital's BIOS emulation technology to set up the video device.
 */
extern int uart_enabled;

void abort(void)
{
	uart_enabled = 1;
	printk("Aborted\n");
	for (;;);
}

extern int StartBiosEmulator(int *bios_return_data);
					/* XX: Dave ?? This was "int bios..."
					 */
extern void SetupBiosGraphics(void);

/*
 * Give it all the entrypoints/utility routines that it expects 
 */

unsigned int inportb(unsigned int p)
{
	return inb(p);
}
unsigned int inportw(unsigned int p)
{
	return inw(p);
}
unsigned int inportl(unsigned int p)
{
	return inl(p);
}
unsigned int inport(unsigned int p)
{
	return inb(p);
}
void outportb(unsigned int p, unsigned int d)
{
	outb(d, p);
}
void outportw(unsigned int p, unsigned int d)
{
	outw(d, p);
}
void outportl(unsigned int p, unsigned int d)
{
	outl(d, p);
}

void outmemb(unsigned int p, unsigned int d)
{
	writeb(d, p);
}
void outmemw(unsigned int p, unsigned int d)
{
	writew(d, p);
}
void outmeml(unsigned int p, unsigned int d)
{
	writel(d, p);
}
unsigned int inmemb(unsigned int p)
{
	return readb(p);
}
unsigned int inmemw(unsigned int p)
{
	return readw(p);
}
unsigned long inmeml(unsigned int p)
{
	return readl(p);
}

unsigned int PCIGetNumberOfBusses(void)
{
	return pci_root.subordinate;
}

#ifdef NOT_NOW
unsigned char InPciCfgB(unsigned int bus, unsigned int dev,
			unsigned int func, unsigned int reg)
{
	unsigned char value;

	pcibios_read_config_byte(bus, PCI_DEVFN(dev, func), reg, &value);

	return value;
}

unsigned short int InPciCfgW(unsigned int bus, unsigned int dev,
			     unsigned int func, unsigned int reg)
{
	unsigned short int value;

	pcibios_read_config_word(bus, PCI_DEVFN(dev, func), reg, &value);

	return value;
}

unsigned int InPciCfgL(unsigned int bus, unsigned int dev,
		       unsigned int func, unsigned int reg)
{
	unsigned int value;

	pcibios_read_config_dword(bus, PCI_DEVFN(dev, func), reg, &value);

	return value;
}

void OutPciCfgB(unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg, unsigned char data)
{
	pcibios_write_config_byte(bus, PCI_DEVFN(dev, func), reg, data);
}

void OutPciCfgW(unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg, unsigned short int data)
{
	pcibios_write_config_word(bus, PCI_DEVFN(dev, func), reg, data);
}

void OutPciCfgL(unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg, unsigned int data)
{
	pcibios_write_config_dword(bus, PCI_DEVFN(dev, func), reg, data);
}

#endif				/* NOT_NOW */

/* used only by the BIOS emulator for delaying before its banner message */

void msleep(int ms)
{
	udelay(ms * 1000);
}

extern struct timeval xtime;	/* The current time */
unsigned long gettime(void)
{
	return xtime.tv_sec;
}

void __cxx_dispatch(void)
{
	printk("__cxx_dispatch() called\n");
}

void malloc_summary(char *p)
{
	printk("malloc_summary: p @ 0x%p\n", p);
}

static int bios_return_data[2]= {0,0,0};


/*
 *-----------------------------------------------------------------------
 * vga_init --
 *      Handle the initialization of VGA register structure.
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
static int vgabios_init(void)
{
#ifdef DEBUG_PRINT
	printk("vgabios_init (Digital BIOS emulation): entry\n");
#endif				/* DEBUG_PRINT */

	/* first, try and start the emulator */
	printk("calling StartBiosEmulator()\n");

	bios_return_data[0]=0;
	bios_return_data[1]=0;
	bios_return_data[2]=0;

	if (!StartBiosEmulator(bios_return_data)) {
		printk("...returned (failed)\n");
		return FALSE;
	}
	printk("...returned (succeeded)\n");

	/* now setup the graphics portion of the emulator */
	printk("calling SetupBiosGraphics\n");
	SetupBiosGraphics();
	printk("...returned\n");

#if 0
	printk("calling clean_mem()\n");
	clean_mem();
#endif
/*
 *  Place attribute controller into normal mode.
 *  i.e. Connected to CRT controller.
 */
	outportb(0x3c0, 0x20);

	vga.row = vga.col = 0;
	vga.nrows = 25;
	vga.ncols = 80;
	vga.video_base = 0xb8000UL;

#ifdef MINI_ALPHA_RUFFIAN
	init_timers();
#endif

	return TRUE;
}
