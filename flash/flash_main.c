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
 *  Main routine for the flash update code.
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

extern void video_init(void);

#define DEBUG_FLASH 1
/*****************************************************************************
 *  Macros.                                                                  *
 *****************************************************************************/
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

extern void FMU(void);

kernel_cap_t cap_bset = CAP_FULL_SET;

volatile struct timeval xtime;	/* The current time */

unsigned long volatile jiffies = 0;
extern unsigned long loops_per_sec;
static unsigned long memory_start = 0;
static unsigned long memory_end = 0;

void *high_memory;
U64 milo_memory_size = 0;

extern unsigned long long _end;
char *free_memory = NULL;

#ifndef DEBUG_MALLOC
void *vmalloc(const unsigned long size)
{
	void *p;

	p = kmalloc(size, 1);
	return p;
}

#endif

#ifdef DEBUG_MALLOC
void *__kmalloc(unsigned int size, int priority, const char *file,
		const int line)
#else
void *kmalloc(size_t size, int priority)
#endif
{
	char *result;

	size += 8 - (size & 0x7);	/* align to an 8 byte boundary */
	result = free_memory;
	free_memory += size;

	return (void *) (result);
}

void free(void *where)
{
}

void boot_main_cont(void)
{
}

#if 0
void *malloc(int size)
{
	return vmalloc(size);
}

#endif
#ifdef CONFIG_TGA_CONSOLE
void tga_console_init(void)
{
}

#endif				/* CONFIG_TGA_CONSOLE */

/*****************************************************************************
 *  HWRPB routines                                                           *
 *****************************************************************************/
struct hwrpb_struct *hwrpb;
void init_HWRPB(void)
{
#if defined(DEBUG_FLASH)
	printk("Building a HWRPB (part I)\n");
#endif

	hwrpb = (struct hwrpb_struct *) kmalloc(PAGE_SIZE, 1);
#if defined(DEBUG_FLASH)
	printk("...Hwrpb at physical address 0x%p\n", hwrpb);
#endif

	/* fill it out the basic bits */

	hwrpb->phys_addr = (U64) hwrpb & 0xffffffff;
	hwrpb->size = sizeof(struct hwrpb_struct);

/*
 *  Set up the system type.  This is based on configure-time #defines 
 */

#ifdef MINI_ALPHA_EV5
	hwrpb->max_asn = 127;
#else
	hwrpb->max_asn = 63;
#endif

	/* weird i/o subsystems */
#ifdef MINI_ALPHA_EB64
	hwrpb->sys_type = 18;
#endif

	/* apecs/21064 i/o */
#ifdef MINI_ALPHA_APECS
#ifdef MINI_ALPHA_EB64P
	hwrpb->sys_type = ST_DEC_EB64P;
#endif
#ifdef MINI_ALPHA_CABRIOLET
	hwrpb->sys_type = ST_DEC_EB64P;
#endif
#ifdef MINI_ALPHA_AVANTI
	hwrpb->sys_type = ST_DEC_2100_A50;
#endif
#endif

	/* 21066/68 style i/o */
#ifdef MINI_ALPHA_LCA
#ifdef MINI_ALPHA_EB66
	hwrpb->sys_type = ST_DEC_EB66;
#endif
#ifdef MINI_ALPHA_EB66P
	hwrpb->sys_type = ST_DEC_EB66;
#endif
#ifdef MINI_ALPHA_NONAME
	hwrpb->sys_type = ST_DEC_AXPPCI_33;
#endif
#endif

#if defined(DEBUG_FLASH)
	printk("...sys_type = %ld\n", hwrpb->sys_type);
#endif
	hwrpb->intr_freq = HZ * 4096;
}

/*****************************************************************************
 *   Main entry point for system primary bootstrap code                      *
 *****************************************************************************/
/* 
 *
 * At this point we have been loaded into memory at LOADER_AT and we are running in Kernel
 * mode.  The Kernel stack base is at the top of memory (wherever that is).  Assume that it
 * takes up two 8K pages.  We are running 1-to-1 memory mapping (ie physical).
 */
void __main(void)
{
	unsigned long start_mem, end_mem;

/*****************************************************************************
 *  Initialize.                                                              *
 *****************************************************************************/
/*
 *  First initialize the Evaulation Board environment.
 */

	/* let's get explicit about some variables we care about. */
	free_memory = (char *) &_end;
	set_hae(alpha_mv.hae_cache);

	/* fix memory size at minimum. */
	milo_memory_size = 8 * 1024 * 1024;
#if defined(DEBUG_FLASH)
	printk("...memory size is 0x%lx\n", milo_memory_size);
#endif

	/* initialise the hwrpb */
	init_HWRPB();

	/* Get some temporary memory. */
	start_mem = (unsigned long) kmalloc(8 * PAGE_SIZE, 0);
	end_mem = start_mem + (8 * PAGE_SIZE);

#ifdef DEBUG_FLASH
	printk("...start_mem @ 0x%lx, end_mem @ 0x%lx\n", start_mem,
	       end_mem);
#endif

	/* Initialize the machine. Usually has to do with
	   setting DMA windows and the like.  */
	if (alpha_mv.init_arch)
		alpha_mv.init_arch(&memory_start, &memory_end);

/*
 *  If we are setting up PCI, then do so.   
 */
#ifdef CONFIG_PCI
	printk("Configuring PCI\n");
	pci_init();
	alpha_mv.pci_fixup();
#endif

	/* init the video */
	video_init();

	/* initialise the keyboard */
#ifdef MINI_SERIAL_ECHO
	printk("Initializing keyboard\n");
#endif
	kbd_init();

	FMU();
}
