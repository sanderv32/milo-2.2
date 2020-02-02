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
 *  general support code.
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

#include "milo.h"
#include "impure.h"
#include "uart.h"
#include "fs.h"

void error(const char *x)
{
	printk("%s\nPress any key to reboot", x);
	kbd_getc();
	halt();
}

void __panic(char *where, const char *fmt, ...)
{
	va_list ap;
	char buf[132];

	printk("panic called (@ 0x%p)\n", where - 4);

	va_start(ap, fmt);

	vsprintf(buf, fmt, ap);

	va_end(ap);		/* clean up */

	error(buf);
}

extern void init_machvec(void);

void doinitdata(void)
{
	extern unsigned long long _edata, _end;

	memset(&_edata, 0, (unsigned int) &_end - (unsigned int) &_edata);

        init_machvec();
}

unsigned long cserve_rd_icsr(void)
{
	return cServe(0x11, 0, 0);
}

void cserve_wr_icsr(unsigned long icsr)
{
	cServe(0x10, icsr, 0);
}
