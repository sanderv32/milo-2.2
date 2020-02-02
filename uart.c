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
 * david.rusling@reo.mts.dec.com
 */

#include <asm/io.h>
#include "milo.h"
#include "uart.h"
#include "flags.h"

#ifndef SERIAL_SPEED
#define SERIAL_SPEED 9600
#endif

int outputport;
int inputport;

int uart_enabled = 0;

int uart_charav(int port)
{
	if (port == COM1)
		return ((inb(com1Lsr) & 1) != 0);
	if (port == COM2)
		return ((inb(com2Lsr) & 1) != 0);
	return (TRUE);
}

char uart_getchar(int port)
{
	while (!uart_charav(port));
	if (port == COM1)
		return ((char) inb(com1Rbr) & 0177);
	else if (port == COM2)
		return ((char) inb(com2Rbr) & 0177);
	else
		return (-1);
}

void uart_putchar(int port, char c)
{
	int lsr, thr;

	if (!uart_enabled)
		return;

	switch (port) {
	case COM1:
		lsr = com1Lsr;
		thr = com1Thr;
		break;
	case COM2:
		lsr = com2Lsr;
		thr = com2Thr;
		break;
	default:
		return;
	}

	if (c == '\n')
		uart_putchar(port, '\r');
	while ((inb(lsr) & 0x20) == 0);
	outb(c, thr);
}


void uart_init_line(int line, int baud)
{
	int i;
	int baudconst;

	switch (baud) {
	case 56000:
		baudconst = 2;
		break;
	case 38400:
		baudconst = 3;
		break;
	case 19200:
		baudconst = 6;
		break;
	case 9600:
		baudconst = 12;
		break;
	case 4800:
		baudconst = 24;
		break;
	case 2400:
		baudconst = 48;
		break;
	case 1200:
		baudconst = 96;
		break;
	case 300:
		baudconst = 384;
		break;
	case 150:
		baudconst = 768;
		break;
	default:
		baudconst = 12;
		break;
	}
	if (line == 1) {
		outb(0x87, com1Lcr);
		outb(0, com1Dlm);
		outb(baudconst, com1Dll);
		outb(0x07, com1Lcr);
		outb(0x0F, com1Mcr);

		for (i = 10; i > 0; i--) {
			if (inb(com1Lsr) == (unsigned int) 0)
				break;
			inb(com1Rbr);
		}
	}
	if (line == 2) {
		outb(0x87, com2Lcr);
		outb(0, com2Dlm);
		outb(baudconst, com2Dll);
		outb(0x07, com2Lcr);
		outb(0x0F, com2Mcr);

		for (i = 10; i > 0; i--) {
			if (inb(com2Lsr) == (unsigned int) 0)
				break;
			inb(com2Rbr);
		}
	}
}

int uart_init()
{
	uart_enabled = milo_global_flags & SERIAL_OUTPUT_FLAG;

#if SERIAL_PORT == 2
	inputport = outputport = COM2;
#else
	inputport = outputport = COM1;
#endif

	uart_init_line(1, SERIAL_SPEED);
	uart_init_line(2, SERIAL_SPEED);

	return 1;
}
