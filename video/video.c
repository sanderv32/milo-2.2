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
 *  Milo: video support
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
#include "video.h"

#if defined(MINI_ALPHA_P2K) || defined(MINI_ALPHA_AVANTI) || defined(MINI_ALPHA_NONAME) || defined (MINI_ALPHA_XL)
#include <linux/tty.h>
struct screen_info screen_info;
#endif


extern struct bootvideo decbiosvideo;
extern struct bootvideo freebiosvideo;
extern struct bootvideo vgavideo;
extern struct bootvideo vgavideo2;
extern struct bootvideo tgavideo;

/*
 *  Note, the order here is important...
 */
static struct bootvideo *bootvideo[] = {
#ifdef MINI_VGA_RAW
	&vgavideo,
#endif
#ifdef MINI_VGA_RAW2
	&vgavideo2,
#endif
#ifdef MINI_DIGITAL_BIOSEMU
	&decbiosvideo,
#endif
#ifdef MINI_FREE_BIOSEMU
	&freebiosvideo,
#endif
#ifdef MINI_TGA_CONSOLE
	&tgavideo
#endif
};

#define NUM_MILO_VIDEO (sizeof(bootvideo) / sizeof(struct bootvideo *))

struct bootvideo *milo_video = NULL;

void video_init(void)
{
	int i;
	int status;

	milo_video = NULL;
	for (i = 0; i < NUM_MILO_VIDEO; i++) {
#ifdef MINI_SERIAL_ECHO
		printk("Trying %s...\n", bootvideo[i]->name);
#endif
		status = (bootvideo[i]->init) ();
		if (status) {
			milo_video = bootvideo[i];
			return;
		}
	}
}

/* This is for BIOS emulators */
#undef malloc

#if defined(MINI_DIGITAL_BIOSEMU) || defined(MINI_FREE_BIOSEMU)
void *malloc(int size)
{
	return vmalloc(size);
}

#endif
