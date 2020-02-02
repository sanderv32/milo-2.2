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

struct bootvideo freebiosvideo = {
	"BIOS VGA (free x86 emulator)",

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
 *  Use David Mosberger-Tang's BIOS emulation technology to set up the
 *  video device.  This has the advantage of being free and modifiable but
 *  it's not so complete as Digitals.  
 */
#include "x86/x86_decode.h"
#include "x86/x86_debug.h"
#include "x86/x86_bios.h"

#define KB		1024
#define MEM_SIZE	(64*KB)	/* should be plenty for now */

#define OFF(addr)	(((addr) >> 0) & 0xffff)
#define SEG(addr)	(((addr) >> 4) & 0xf000)

#define VGA_BIOS_BASE	0xc0000
#define VGA_BIOS_ENTRY	(VGA_BIOS_BASE + 3)


/*
 * BIOS Video Display Data Area 1:
 */
struct video_area1 {
	u8 pad;			/* to make things aligned */
	u8 mode;		/* active display mode */
	u16 ncols;		/* # of display columns */
	u16 len;		/* length of video buffer */
	u16 start;		/* starting address in video buffer */
	struct {
		u8 col;
		u8 row;
	} cup[8];		/* cursor position in page 0..7 */
	u16 cursor_type;
	u8 active_page;		/* number of currently active page */
	u8 base0, base1;	/* CRT controller base address */
	u8 p3x8;		/* current value of register 0x3x8 */
	u8 p3x9;		/* current value of register 0x3x9 */
};


static inline unsigned long
mem_read(SysEnv * m, unsigned long addr, unsigned long size)
{
	unsigned long val = 0;

	if (addr >= 0xa0000 && addr <= 0xfffff) {
		switch (size) {
		case 1:
			val = readb(addr);
			break;
		case 2:
			if (addr & 0x1) {
				val =
				    (readb(addr) | (readb(addr + 1) << 8));
			} else {
				val = readw(addr);
			}
			break;
		case 4:
			if (addr & 0x3) {
				val = (readb(addr) |
				       (readb(addr + 1) << 8) |
				       (readb(addr + 2) << 16) |
				       (readb(addr + 3) << 24));
			} else {
				val = readl(addr);
			}
			break;
		}
	} else if (addr > m->mem_size - size) {
		printk("mem_read: address %#lx (size %lu) out of range!\n",
		       addr, size);
		halt_sys(m);
	} else {
		switch (size) {
		case 1:
			val = *(u8 *) (m->mem_base + addr);
			break;
		case 2:
			if (addr & 0x1) {
				val = (*(u8 *) (m->mem_base + addr) |
				       (*(u8 *) (m->mem_base + addr + 1) <<
					8));
			} else {
				val = *(u16 *) (m->mem_base + addr);
			}
			break;
		case 4:
			if (addr & 0x3) {
				val = (*(u8 *) (m->mem_base + addr + 0) |
				       (*(u8 *) (m->mem_base + addr + 1) <<
					8) |
				       (*(u8 *) (m->mem_base + addr + 2) <<
					16) |
				       (*(u8 *) (m->mem_base + addr + 3) <<
					24));
			} else {
				val = *(u32 *) (m->mem_base + addr);
			}
			break;
		}
	}
	if (DEBUG_MEM_TRACE(m)) {
		x86_debug_printf(m, "%#08x %d -> %#x\n", addr, size, val);
	}
	return val;
}


void
mem_write(SysEnv * m, unsigned long addr, unsigned long val,
	  unsigned long size)
{
	if (DEBUG_MEM_TRACE(m)) {
		x86_debug_printf(m, "%#08x %d <- %#x\n", addr, size, val);
	}

	if (addr >= 0xa0000 && addr <= 0xfffff) {

		/* it's an adapter BIOS ROM access */
		switch (size) {
		case 1:
			writeb(val, addr);
			break;
		case 2:
			if (addr & 0x1) {
				writeb(val >> 0, addr);
				writeb(val >> 8, addr + 1);
			} else {
				writew(val, addr);
			}
			break;
		case 4:
			if (addr & 0x3) {
				writeb(val >> 0, addr);
				writeb(val >> 8, addr + 1);
				writeb(val >> 16, addr + 1);
				writeb(val >> 24, addr + 1);
			} else {
				writel(val, addr);
			}
			break;
		}
	} else if (addr >= m->mem_size) {
		printk("mem_write: address %#lx out of range!\n", addr);
		halt_sys(m);
	} else {
		switch (size) {
		case 1:
			*(u8 *) (m->mem_base + addr) = val;
			break;
		case 2:
			if (addr & 0x1) {
				*(u8 *) (m->mem_base + addr + 0) =
				    (val >> 0) & 0xff;
				*(u8 *) (m->mem_base + addr + 1) =
				    (val >> 8) & 0xff;
			} else {
				*(u16 *) (m->mem_base + addr) = val;
			}
			break;
		case 4:
			if (addr & 0x1) {
				*(u8 *) (m->mem_base + addr + 0) =
				    (val >> 0) & 0xff;
				*(u8 *) (m->mem_base + addr + 1) =
				    (val >> 8) & 0xff;
				*(u8 *) (m->mem_base + addr + 2) =
				    (val >> 16) & 0xff;
				*(u8 *) (m->mem_base + addr + 3) =
				    (val >> 24) & 0xff;
			} else {
				*(u16 *) (m->mem_base + addr) = val;
			}
			break;
		}
	}
}


u8 sys_rdb(SysEnv * m, u32 addr)
{
	return mem_read(m, addr, 1);
}


u16 sys_rdw(SysEnv * m, u32 addr)
{
	return mem_read(m, addr, 2);
}


u32 sys_rdl(SysEnv * m, u32 addr)
{
	return mem_read(m, addr, 4);
}


void sys_wrb(SysEnv * m, u32 addr, u8 val)
{
	mem_write(m, addr, val, 1);
}


void sys_wrw(SysEnv * m, u32 addr, u16 val)
{
	mem_write(m, addr, val, 2);
}


void sys_wrl(SysEnv * m, u32 addr, u32 val)
{
	mem_write(m, addr, val, 4);
}


u8 sys_inb(SysEnv * m, u32 port)
{
	unsigned long val;

	val = inb(port);
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 1 -> %#x\n", port, val);
	}
	return val;
}


u16 sys_inw(SysEnv * m, u32 port)
{
	unsigned long val;

	val = inw(port);
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 1 -> %#x\n", port, val);
	}
	return val;
}


u32 sys_inl(SysEnv * m, u32 port)
{
	unsigned long val;

	val = inl(port);
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 1 -> %#x\n", port, val);
	}
	return val;
}


void sys_outb(SysEnv * m, u32 port, u8 val)
{
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 1 <- %#x\n", port, val);
	}
	outb(val, port);
}


void sys_outw(SysEnv * m, u32 port, u16 val)
{
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 2 <- %#x\n", port, val);
	}
	outw(val, port);
}


void sys_outl(SysEnv * m, u32 port, u32 val)
{
	if (DEBUG_IO_TRACE(m)) {
		x86_debug_printf(m, "\t\t\t\t%#04x 4 <- %#x\n", port, val);
	}
	outl(val, port);
}


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
	int ret=TRUE;
	SysEnv sys;
	u8 *vid2;
	struct video_area1 *vid1;
	struct pci_dev *pdev=NULL;

#ifdef DEBUG_PRINT
	printk("vgabios_init (free x86 BIOS emulator): entry\n");
#endif	/* DEBUG_PRINT */

	while ((pdev = pci_find_class(PCI_CLASS_DISPLAY_VGA << 8,pdev)))
	{
		memset(&sys, 0, sizeof(sys));

		printk ("vendor=0x%04x, device=0x%04x\n",
				pdev->vendor,pdev->device);

		pci_write_config_dword(pdev, PCI_ROM_ADDRESS,
				VGA_BIOS_BASE | PCI_ROM_ADDRESS_ENABLE);

		if (sys_rdw(&sys, VGA_BIOS_BASE) != 0xaa55) {
			printk("vga_bios_init: sorry, there seems to be no BIOS\n");
			pci_write_config_dword(pdev, PCI_ROM_ADDRESS,0);
			ret=FALSE;
			break;
		}

		sys.mem_base = (unsigned long) kmalloc(MEM_SIZE, 0);
		if (!sys.mem_base) {
			printk("vgabios_init: can't alloc memory");
			pci_write_config_dword(pdev, PCI_ROM_ADDRESS,0);
			ret=FALSE;
			break;
		}

		sys.mem_size = MEM_SIZE;

		/* Initialize Interval Timers: */
		outb(0x54, 0x43);	/* counter 1: refresh timer */
		outb(0x18, 0x41);

#if !defined(MINI_ALPHA_RUFFIAN)
		outb(0x36, 0x43);	/* counter 0: system timer */
		outb(0x00, 0x40);
		outb(0x00, 0x40);
#endif

		outb(0xb6, 0x43);	/* counter 2: speaker */
		outb(0x31, 0x42);
		outb(0x13, 0x42);

		/* now get started: */
		x86_bios_init(&sys);
		sys.x86.R_CS = SEG(VGA_BIOS_ENTRY);
		sys.x86.R_IP = OFF(VGA_BIOS_ENTRY);
		sys.x86.R_SS = SEG(sys.mem_size - 8);
		sys.x86.R_SP = OFF(sys.mem_size - 8);

		printk("vgabios_init: starting emulation...\n");
		x86_exec(&sys);

		printk("vgabios_init: emulation done...\n");

		/* Notice that the first data byte is at 0x449 but we need to make it 0x448
		 * because of the padding at the beginning of the structure. */
		vid1 = (struct video_area1 *) (sys.mem_base + 0x448);
		vid2 = (u8 *) (sys.mem_base + 0x484);
		vga.nrows = *vid2 + 1;
		vga.ncols = vid1->ncols;
		vga.row = vid1->cup[vid1->active_page].row;
		vga.col = vid1->cup[vid1->active_page].col;

#if 1

		/* Explicitly set video mode 3.  This is a workaround until I figured why
		 * #9 Level 12 doesn't initialize properly without this... */
		((char *) sys.mem_base)[0x4000] = 0xcd;
		/* INT 10 */
		((char *) sys.mem_base)[0x4001] = 0x10;
		((char *) sys.mem_base)[0x4002] = 0xc3;
		/* RET */
		sys.x86.R_AH = 0x00;	/* set video mode 3 */
		sys.x86.R_AL = 0x03 | (1 << 7);	/* but don't clear screen */
		sys.x86.R_CS = SEG(0x04000);
		sys.x86.R_IP = OFF(0x04000);
		sys.x86.R_SS = SEG(sys.mem_size - 8);
		sys.x86.R_SP = OFF(sys.mem_size - 8);
		x86_exec(&sys);
#endif
		vga.video_base = 0xb8000UL + vid1->start;

		pci_write_config_dword(pdev, PCI_ROM_ADDRESS,0);

	}

	return (ret);
}
