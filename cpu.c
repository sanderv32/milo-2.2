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
 * CPU initialization code and global data for ALPHA platforms
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <asm/system.h>		/* for cli()/sti() */
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/hwrpb.h>

#include "milo.h"

extern int KEYBOARD_IRQ;	/* in drivers/char/keyboard.c */

#define vulp	volatile unsigned long *

/*
 * globals with platform-dependent values
 */
int high_page_byte;		/* for DMA address settings */

extern void write_cpureg_long(int v, U64 addr);
extern int read_cpureg_long(U64 addr);
extern void write_cpureg_quad(U64 v, U64 addr);
extern U64 read_cpureg_quad(U64 addr);


#if defined(MINI_ALPHA_EB66) || defined(MINI_ALPHA_EB66P) \
 || defined(MINI_ALPHA_NONAME) || defined(MINI_ALPHA_TAKARA)
static void alpha_pic_init(void)
{

#if 0

	/* Enable the NMI interrupt */
	outb(0x00, 0x70);

	/* Set Special Mask Mode as an EOI precaution */
	outb(0xc0, 0xa0);	/*CTLR 2 */
	outb(0xc0, 0x20);	/*CTLR 1 */

	/* Enable Non Maskable Interrupt (NMI) masks */
	outw(0x0000, 0x61);
	outw(0x0c00, 0x461);

#endif

#ifndef MINI_ALPHA_NONAME

	/*  Initialize PIC ELR to set all IRQ interrupts to Edge trigger, This
	 * should be power-up default, but let's do it any way and let EISA bus
	 * code set as needed */
	outb(0x00, 0x4d1);
	outb(0x00, 0x4d0);
#endif

	/*  Initialize all PIC interrupts to be disabled just in case. They will be
	 * enabled through EISA bus support. */
	outb(0xff, 0xa1);	/* CTRL2 */
	outb(0xff, 0x21);	/* CTRL1 */
}

#endif				/* MINI_ALPHA_EB66 ||
				 * MINI_ALPHA_EB66P ||
				 * MINI_ALPHA_NONAME */

void milo_cpu_init(void)
{
/* NOTE: NO PRINTING VIA "PRINTK" BEFORE THE SECOND "SWITCH"!!!!!! */

	/* set-up the address constants used for IO and memory space  accesses
	 * first */
#ifdef MINI_ALPHA_EB64

	high_page_byte = 0x00;	/* straighforward map */

#define PIC1  0x20
#define PIC2  0xA0

	outb(0x00, PIC1 + 0x1);	/* allow the PICS to interrupt */
	outb(0x00, PIC2 + 0x1);

#define IDX486 0xec
#define DATA486  0xed

	/* Enable 0x100000 to 0x7FFFFF as local bus memory (DMA) */
	outb(0x20, IDX486);
	outb(0xa0, DATA486);	/* 8MB region starting at 0x00 */
	outb(0x21, IDX486);
	outb(0xfe, DATA486);	/* Enable all subregions except 1st Meg
				 */

	/* Enable 0x800000 to 0xFFFFFF as local bus memory (DMA) */
	outb(0x22, IDX486);
	outb(0xa1, DATA486);	/* 8MB region starting at 8MB */
	outb(0x23, IDX486);
	outb(0xff, DATA486);	/* Enable all subregions */

	/* do DMA controller inits */
	outb(0xff, DMA1_RESET_REG);
	outb(0xff, DMA2_RESET_REG);

	outb(0x00, DMA1_CMD_REG);
	outb(0x00, DMA2_CMD_REG);

	outb(0x46, DMA1_MODE_REG);
	outb(0xc0, DMA2_MODE_REG);

	outb(0x02, DMA1_EXTMODE_REG);

	outb(0x0f, DMA1_MASK_ALL_REG);	/* 0,1,2,3 */
	outb(0x0e, DMA2_MASK_ALL_REG);	/* 5,6,7 */

	/* Enable the NMI interrupt */
	outb(0x00, 0x70);

#define ICW1_C1 0x20
#define ICW2_C1 0x21
#define ICW3_C1 0x21
#define ICW4_C1 0x21
#define MASK_C1 0x21
#define ELCR_C1 0x4D0
#define OCW1_C1 0x21
#define OCW2_C1 0x20
#define OCW3_C1 0x20

#define ICW1_C2 0xA0
#define ICW2_C2 0xA1
#define ICW3_C2 0xA1
#define ICW4_C2 0xA1
#define MASK_C2 0xA1
#define ELCR_C2 0x4D1
#define OCW1_C2 0xA1
#define OCW2_C2 0xA0
#define OCW3_C2 0xA0

	outb(0x19, ICW1_C2);	/* level sensitive */
	outb(0x08, ICW2_C2);
	outb(0x02, ICW3_C2);
	outb(0x01, ICW4_C2);
	outb(0x19, ELCR_C2);	/* select level sensitive */

	outb(0x11, ICW1_C1);
	outb(0x00, ICW2_C1);	/* edge sensitive */
	outb(0x04, ICW3_C1);
	outb(0x01, ICW4_C1);
	outb(0x11, ELCR_C1);	/* edge sensitive */

	for (i = 0; i < 8; i++) {	/* issue end of interrupt sequence */
		outb(0x20 | i, OCW2_C2);
		outb(0x20 | i, OCW2_C1);
	}

	/*  Initialize all PIC interrupts to be disabled just in case. They will be
	 * enabled through EISA bus support. */
	outb(0xff, 0xa1);	/* CTRL2 */
	outb(0xff, 0x21);	/* CTRL1 */

#endif

#ifdef MINI_ALPHA_EB66

	/* JAE: NOTE: this may be different for other platforms;  but this does
	 * work for EB66 at the moment */
	high_page_byte = 0x40;	/* map 4GB from PCI to MEM */

	/* init the PIC for the IRQs */
	alpha_pic_init();

	/* do DMA controller inits */

	outb(0xff, DMA1_RESET_REG);
	outb(0xff, DMA2_RESET_REG);

	outb(0x00, DMA1_CMD_REG);
	outb(0x00, DMA2_CMD_REG);

	outb(0x0f, DMA1_MASK_ALL_REG);	/* 0,1,2,3 */
	outb(0x0e, DMA2_MASK_ALL_REG);	/* 5,6,7 */

/*
 *  Put both DMA controllers into cascade mode.
 */
	outb(0xc1, DMA1_MODE_REG);
	outb(0xc1, DMA2_MODE_REG);

	/* do PCI-system memory window setup.  1Gbyte PCI memory maps into 0 system
	 * address. */

	write_cpureg_quad(0x3ff00000, 0xfffffc0180000000ULL | 0x140);
	/*  mask 0 = 1Gb */
	write_cpureg_quad(0x00000000, 0xfffffc0180000000ULL | 0x180);
	/*  translated base 0 = 0 */
	write_cpureg_quad(0x40000000 | 0x0000000200000000ULL,
			  0xfffffc0180000000ULL | 0x100);	/* base 0 = 1 Gb */

	outb(0xdf, 0x26);	/* allow ISA interrupts */
	outb(0xff, 0x27);

#endif


#ifdef MINI_ALPHA_EB66P

	/* JAE: NOTE: this may be different for other platforms;  but this does
	 * work for EB66 at the moment */
	high_page_byte = 0x40;	/* map 4GB from PCI to MEM */

	/* init the PIC for the IRQs */
	alpha_pic_init();

	/* do DMA controller inits */

	outb(0xff, DMA1_RESET_REG);
	outb(0xff, DMA2_RESET_REG);

	outb(0x00, DMA1_CMD_REG);
	outb(0x00, DMA2_CMD_REG);

	outb(0x0f, DMA1_MASK_ALL_REG);	/* 0,1,2,3 */
	outb(0x0e, DMA2_MASK_ALL_REG);	/* 5,6,7 */

/*
 *  Put both DMA controllers into cascade mode.
 */
	outb(0xc1, DMA1_MODE_REG);
	outb(0xc1, DMA2_MODE_REG);

	/* do PCI-system memory window setup.  1Gbyte PCI memory maps into 0 system
	 * address. */

	write_cpureg_quad(0x3ff00000, 0xfffffc0180000000ULL | 0x140);
	/*  mask 0 = 1Gb */
	write_cpureg_quad(0x00000000, 0xfffffc0180000000ULL | 0x180);
	/*  translated base 0 = 0 */
	write_cpureg_quad(0x40000000 | 0x0000000200000000ULL,
			  0xfffffc0180000000ULL | 0x100);	/* base 0 = 1 Gb */

	outb(0xfb, 0x21);
	outb(0xff, 0xa1);

	outb(0xef, 0x804);
	outb(0xff, 0x806);
	outb(0xff, 0x806);

#endif

#ifdef MINI_ALPHA_NONAME

	/* JAE: NOTE: this may be different for other platforms;  but this does
	 * work for EB66 at the moment */
	high_page_byte = 0x40;	/* map 4GB from PCI to MEM */

	/* init the PIC for the IRQs */
	alpha_pic_init();

	/* do DMA controller inits */

	outb(0xff, DMA1_RESET_REG);
	outb(0xff, DMA2_RESET_REG);

	outb(0x00, DMA1_CMD_REG);
	outb(0x00, DMA2_CMD_REG);

	outb(0x0f, DMA1_MASK_ALL_REG);	/* 0,1,2,3 */
	outb(0x0e, DMA2_MASK_ALL_REG);	/* 5,6,7 */

/*
 *  Put both DMA controllers into cascade mode.
 */
	outb(0xc1, DMA1_MODE_REG);
	outb(0xc1, DMA2_MODE_REG);

	/* do PCI-system memory window setup.  1Gbyte PCI memory maps into 0 system
	 * address. */

	write_cpureg_quad(0x3ff00000, 0xfffffc0180000000ULL | 0x140);
	/*  mask 0 = 1Gb */
	write_cpureg_quad(0x00000000, 0xfffffc0180000000ULL | 0x180);
	/*  translated base 0 = 0 */
	write_cpureg_quad(0x40000000 | 0x0000000200000000ULL,
			  0xfffffc0180000000ULL | 0x100);	/* base 0 = 1 Gb */

	outb(0xfb, 0x21);
	outb(0xff, 0xa1);

#endif

#if 0
#ifdef MINI_ALPHA_EB64P

	/* JAE: NOTE: this may be different for other platforms;  but this does
	 * work for EB66 at the moment */
	high_page_byte = 0x40;	/* map 4GB from PCI to MEM */

	/* init the PIC for the IRQs */
	alpha_pic_init();

	/* do DMA controller inits */

	outb(0xff, DMA1_RESET_REG);
	outb(0xff, DMA2_RESET_REG);

	outb(0x00, DMA1_CMD_REG);
	outb(0x00, DMA2_CMD_REG);

	outb(0x0f, DMA1_MASK_ALL_REG);	/* 0,1,2,3 */
	outb(0x0e, DMA2_MASK_ALL_REG);	/* 5,6,7 */

/*
 *  Put both DMA controllers into cascade mode.
 */
	outb(0xc1, DMA1_MODE_REG);
	outb(0xc1, DMA2_MODE_REG);

	/* do PCI-system memory window setup.  1Gbyte PCI memory maps into 0 system
	 * address. */

	write_cpureg_quad(0x3ff00000, 0xfffffc0180000000ULL | 0x140);
	/*  mask 0 = 1Gb */
	write_cpureg_quad(0x00000000, 0xfffffc0180000000ULL | 0x180);
	/*  translated base 0 = 0 */
	write_cpureg_quad(0x40000000 | 0x0000000200000000ULL,
			  0xfffffc0180000000ULL | 0x100);	/* base 0 = 1 Gb */

	outb(0xdf, 0x26);	/* allow ISA interrupts */
	outb(0xff, 0x27);

#endif
#endif

}



/* JAE: this needs to be done *ACCURATELY* for a given platform... */
