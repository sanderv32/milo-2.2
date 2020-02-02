#/*****************************************************************************

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
 * Jacket routine for printk() in the miniloader.  It now calls vsprintf() in
 * lib.a.
 *
 * david.rusling@reo.mts.dec.com
 */

#include <asm/system.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <stdarg.h>

#include "milo.h"
#include "video/video.h"
#include "uart.h"

int cons_puts(int unit, char *buf);
void cons_gets(char *buf);

int milo_suppress_printk = 0, printk_rows = 0;

/* make this 4K and static, so it won't go on the stack, or overflow easily */
static char print_buf[4096];

int printk(const char *fmt, ...)
{
	int count;
	va_list ap;

	va_start(ap, fmt);

	if (fmt[0] == '<' && fmt[1] >= '0' && fmt[1] <= '9' && fmt[2] == '>') {

		/* skip annoying log-priority code: */
		fmt += 3;
	}

	count = vsprintf(print_buf, fmt, ap);

	if (!milo_suppress_printk)
#ifndef MILO_PAGER
		cons_puts(0, print_buf);
#else
	{
		if (printk_rows == 0)
			cons_puts (0, print_buf);
		else {
			char *cur = print_buf, *tail;

            /*
             * If the pager is active (i.e. printk_rows != 0), print line per
             * line.  If we're at the bottom of the page, wait for user input.
             */
			while (cur != NULL && cur[0] != '\0') {
				if ((printk_rows % 25) == 0) {
					cons_puts (0, "-- More --");
					kbd_getc ();
					cons_puts (0, "\r          \r");  /* Erase "-- More --" */
					printk_rows++;
				}

				if ((tail = strchr (cur, '\n')) != NULL) {
					*tail = '\0';
					cons_puts (0, cur); cons_puts (0, "\n"); printk_rows++;
					cur = tail + 1;
				} else {
					cons_puts (0, cur);
					cur = NULL;
				}
			}
		}
	}
#endif

	va_end(ap);	/* clean up */
	return count;
}

int printf(const char *fmt, ...)
{
	int count;
	va_list ap;

	va_start(ap, fmt);

	count = vsprintf(print_buf, fmt, ap);
	cons_puts(0, print_buf);

	va_end(ap);	/* clean up */

	return count;
}

int cons_puts(int unit, char *buf)
{
	int i;

	i = 0;
	while (buf[i] != '\0') {
		__video_putchar(buf[i]);
#ifdef MINI_SERIAL_ECHO
		uart_putchar(SERIAL_PORT, buf[i]);
#endif
		i++;
	}

	return i;
}

void PutChar(int c)
{
	char buf[2];

	buf[0] = c;
	buf[1] = '\0';
	cons_puts(0, buf);
}

#if 1
#ifdef MINI_SERIAL_ECHO
void cons_gets(char *buf)
{
	int i;
	int retval;

	for (i = 0;; i++) {
		retval = uart_getchar(SERIAL_PORT);
		retval &= 0xff;
		switch (retval) {
		case '\r': 
		case '\n': 
			buf[i] = '\0';
			retval = '\r';
			uart_putchar(SERIAL_PORT, '\n');
			return;

		case 0x08: 			/* BS */
		case 0x7f: 			/* Delete */
			if (i > 0) {
			    cons_puts(0, "\b \b");
			    i--;
			}
			break;
		default: 
			uart_putchar(SERIAL_PORT, retval);
			buf[i] = retval;
			break;
		}
	}
}
#endif /* MINI_SERIAL_ECHO */
#endif

#ifdef MINI_SERIAL_IO
int GetChar(void) 
{
	int c;

	c = uart_getchar(SERIAL_PORT);
	if (c == '\r')
		c = '\n';

	return c;
}
#endif
