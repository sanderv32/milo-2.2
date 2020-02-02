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

/*
 *  If you turn DEBUG_RELOCATE on, you'll have 
 *  to include uart.o, printf.o and so on.
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

#include "milo.h"
#include "impure.h"
#include "uart.h"

#define RELOCATE_VERSION "1.1"

/*****************************************************************************
 *  Macros.                                                                  *
 *****************************************************************************/

struct bootvideo *milo_video = NULL;
unsigned long milo_global_flags;

extern char flash_image[];
extern unsigned int flash_image_size;

/* Dummy functions used by head.S, but are only needed when
 * running the real Milo[tm], not the relocate stub 
 */

void free(void *where)
{
}

void video_putchar(unsigned char c)
{

}

void boot_main_cont(void)
{
}

extern void init_machvec(void);
void doinitdata(void)
{
	init_machvec();
}

void __panic(void)
{
}

int kbd_getc(void)
{
	return 0;
}

/*****************************************************************************
 *   Main entry point for system primary bootstrap code                      *
 *****************************************************************************/
unsigned long __main(void)
{
/*****************************************************************************
 *  Initialize.                                                              *
 *****************************************************************************/
/*
 *  First initialize the Evaulation Board environment.
 */
	milo_global_flags = *(unsigned long *) (DECOMP_PARAMS + 768 + 8);

#ifdef DEBUG_RELOCATE
#if defined(MINI_ALPHA_PC164) || defined(MINI_ALPHA_LX164)
/*
 *  The PC164 employs the SMC FDC37C93X Ultra I/O Controller.
 *  After reset all of the devices are disabled. SMC93X_Init() will
 *  autodetect the FDC37C93X and enable each of the devices.
 */
	SMC93x_Init();
#endif

	/* we can only output to the serial port *after* we initialise it */
	uart_init();
#endif

#ifdef DEBUG_RELOCATE
	/* announce ourselves */
	printk("Milo Relocate: V" RELOCATE_VERSION "\n");
#endif

#define PARAMS_AT	(PALCODE_AT - 1024)
#define MEMORY_AT	(PALCODE_AT - 256)
#define FLAGS_AT	(MEMORY_AT + 8)
	/* copy the parameters block (not overwriting ARC-provided stuff) */
#ifdef DEBUG_RELOCATE
	printk("Copying parameters into position at 0x%p\n",
	       (void *) PARAMS_AT);
#endif
	if (*(unsigned int *) (DECOMP_PARAMS + 768 + 4))
#ifdef DEBUG_RELOCATE
		printk("  memory = %u Mb\n",
		       *(unsigned int *) (DECOMP_PARAMS + 768 + 4)),
#endif
		    *(long *) MEMORY_AT = *(long *) (DECOMP_PARAMS + 768);

	memcpy((void *) FLAGS_AT, (void *) (DECOMP_PARAMS + 768 + 8), 248);

	if (strncmp((char *) PARAMS_AT, "MILOOSLOADOPTIONS=", 18) != 0	/* Not from ARC */
	    || *(char *) (PARAMS_AT + 18) == '\0')
#ifdef DEBUG_RELOCATE
		printk("  %s\n", (char *) DECOMP_PARAMS),
#endif
		    memcpy((void *) PARAMS_AT, (void *) DECOMP_PARAMS,
			   768);

	/* copy milo into the appropriate place in memory */
#ifdef DEBUG_RELOCATE
	printk("Copying Milo into position at 0x%p\n",
	       (void *) PALCODE_AT);
#endif
	memcpy((void *) PALCODE_AT, (void *) flash_image,
	       flash_image_size);

	*(unsigned long *) (STUB_AT - 8) = milo_global_flags;

	/* now jump to it */
#ifdef DEBUG_RELOCATE
	printk("Jumping to Milo Stub...\n");
#endif
	return (unsigned long) PALCODE_AT;
}
