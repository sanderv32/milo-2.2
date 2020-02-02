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
 * paradis@amt.tay1.dec.com
 */

#include <linux/kernel.h>

#include "milo.h"
#include "video/video.h"
#include "uart.h"

extern volatile unsigned long jiffies;
extern int uart_enabled;

#ifdef MINI_SERIAL_IO

int kbd_getc_with_timeout(int nsecs)
{
	unsigned long endjifs;

	if (nsecs) {
		endjifs = jiffies + ((unsigned long) nsecs * 1000L);
		sti();
	}

	while (1) {

		/* Input from both uart */
		if (uart_charav(SERIAL_PORT))
			return (uart_getchar(SERIAL_PORT));

		if (nsecs) {
			if (jiffies > endjifs) {
				return -1;
			}
		}
	}
}

#else
/* This is a *very* simple polled keyboard driver.  The scancode
 * processing is primitive, and it only understands CTRL and SHIFT
 * (and not both at once, either!  CTRL overrides SHIFT...)
 */

/* These are the only "special" keyboard functions we'll support */
#define KB_CTRL		0xf0
#define KB_SHIFT	0xf1
#define KB_CAPSLOCK	0xf2

/* These are just placeholders... we don't assign them any real
 * value or function...
 */
#define KB_NUMLOCK	0
#define KB_SCRLOCK	0
#define KB_ALT		0

/* This is the scancode mapping table for "plain" characters
 * (no ctrl or shift)
 */
static unsigned char plain_charmap[] = {
	0, '\033', '1', '2', '3', '4', '5', '6',
	/* 0x00 */
	'7', '8', '9', '0', '-', '=', '\010', '\011',
	/* 0x08 */
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
	/* 0x10 */
	'o', 'p', '[', ']', '\015', KB_CTRL, 'a', 's',
	/* 0x18 */
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
	/* 0x20 */
	'\047', '`', KB_SHIFT, '\\', 'z', 'x', 'c', 'v',
	/* 0x28 */
	'b', 'n', 'm', ',', '.', '/', KB_SHIFT, '*',
	/* 0x30 */
	KB_ALT, '\040', KB_CAPSLOCK, 0, 0, 0, 0, 0,
	/* 0x38 */
	0, 0, 0, 0, 0, KB_NUMLOCK, KB_SCRLOCK, '7',
	/* 0x40 */
	'8', '9', '-', '4', '5', '6', '+', '1',
	/* 0x48 */
	'2', '3', '0', '.', 0, 0, 0, 0,	/* 0x50 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x58 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x60 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x68 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x70 */
	0, 0, 0, 0, 0, 0, 0, 0	/* 0x78 */
};


/* This is the scancode mapping table for shifted characters */
static unsigned char shift_charmap[] = {
	0, '\033', '!', '@', '#', '$', '%', '^',
	/* 0x00 */
	'&', '*', '(', ')', '_', '+', '\010', '\011',
	/* 0x08 */
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	/* 0x10 */
	'O', 'P', '{', '}', '\015', KB_CTRL, 'A', 'S',
	/* 0x18 */
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
	/* 0x20 */
	'"', '~', KB_SHIFT, '|', 'Z', 'X', 'C', 'V',
	/* 0x28 */
	'B', 'N', 'M', '<', '>', '?', KB_SHIFT, '*',
	/* 0x30 */
	KB_ALT, '\040', KB_CAPSLOCK, 0, 0, 0, 0, 0,
	/* 0x38 */
	0, 0, 0, 0, 0, KB_NUMLOCK, KB_SCRLOCK, '7',
	/* 0x40 */
	'8', '9', '-', '4', '5', '6', '+', '1',
	/* 0x48 */
	'2', '3', '0', '.', 0, 0, 0, 0,	/* 0x50 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x58 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x60 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x68 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 0x70 */
	0, 0, 0, 0, 0, 0, 0, 0	/* 0x78 */
};

/* This is the scancode mapping table for control characters */
static unsigned char ctrl_charmap[] = {
	'\000', '\033', '1', '\000', '\033', '\034', '\035', '\036',
	/* 0x00 */
	'\037', '\177', '9', '0', '-', '=', '\030', '\011',
	/* 0x08 */
	'\021', '\027', '\005', '\022', '\024', '\031', '\025', '\011',
	/* 0x10 */
	'\017', '\020', '\033', '\035', '\015', KB_CTRL, '\001', '\023',
	/* 0x18 */
	'\004', '\006', '\007', '\010', '\012', '\013', '\014', ';',
	/* 0x20 */
	'\047', '\036', KB_SHIFT, '\034', '\032', '\030', '\003', '\026',
	/* 0x28 */
	'\002', '\016', '\015', ',', '.', '/', KB_SHIFT, '*',
	/* 0x30 */
	KB_ALT, '\000', KB_CAPSLOCK, 0, 0, 0, 0, 0,
	/* 0x38 */
	0, 0, 0, 0, 0, KB_NUMLOCK, KB_SCRLOCK, '7',
	/* 0x40 */
	'8', '9', '-', '4', '5', '6', '+', '1',
	/* 0x48 */
	'2', '3', '0', '.', '\000', '\000', '\000', 0,
	/* 0x50 */
	0, '\000', '\000', '\000', '\000', '\000', '\000', '\000',
	/* 0x58 */
	'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
	/* 0x60 */
	'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
	/* 0x68 */
	'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
	/* 0x70 */
	'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000'
	    /* 0x78 */
};

/* These are the keyboard states we can support */
enum kbdstates {
	KBD_NORMAL,
	KBD_SHIFTED,
	KBD_CTRL,
};

enum kbdstates keyboard_state = KBD_NORMAL;

#define SCANCODE_KEYUP	0x80
#define SCANCODE_LEFTSHIFT	0x2a
#define SCANCODE_RIGHTSHIFT	0x36
#define SCANCODE_CTRL		0x1d
#define SCANCODE_CAPSLOCK	0x3a
#define SCANCODE_EXTENDED	0xe0

/*
 * keyboard controller registers
 */
#define KBD_STATUS_REG      (unsigned int) 0x64
#define KBD_CNTL_REG        (unsigned int) 0x64
#define KBD_DATA_REG	    (unsigned int) 0x60
/*
 * controller commands
 */
#define KBD_READ_MODE	    (unsigned int) 0x20
#define KBD_WRITE_MODE	    (unsigned int) 0x60
#define KBD_SELF_TEST	    (unsigned int) 0xAA
#define KBD_SELF_TEST2	    (unsigned int) 0xAB
#define KBD_CNTL_ENABLE	    (unsigned int) 0xAE
/*
 * keyboard commands
 */
#define KBD_ENABLE	    (unsigned int) 0xF4
#define KBD_DISABLE	    (unsigned int) 0xF5
#define KBD_RESET	    (unsigned int) 0xFF
/*
 * keyboard replies
 */
#define KBD_ACK		    (unsigned int) 0xFA
#define KBD_POR		    (unsigned int) 0xAA
/*
 * status register bits
 */
#define KBD_OBF		    (unsigned int) 0x01
#define KBD_IBF		    (unsigned int) 0x02
#define KBD_GTO		    (unsigned int) 0x40
#define KBD_PERR	    (unsigned int) 0x80
/*
 * keyboard controller mode register bits
 */
#define KBD_EKI		    (unsigned int) 0x01
#define KBD_SYS		    (unsigned int) 0x04
#define KBD_DMS		    (unsigned int) 0x20
#define KBD_KCC		    (unsigned int) 0x40

#define TIMEOUT_CONST	500000

static int kbd_wait_for_input(void)
{
	int n;
	int status, data;

	n = TIMEOUT_CONST;
	do {
		status = inb(KBD_STATUS_REG);

		/* Wait for input data to become available.  This bit will then be
		 * cleared by the following read of the DATA register. */

		if (!(status & KBD_OBF))
			continue;

		data = inb(KBD_DATA_REG);

		/* Check to see if a timeout error has occured.  This means that
		 * transmission was started but did not complete in the normal time
		 * cycle.  PERR is set when a parity error occured in the last
		 * transmission. */
		if (status & (KBD_GTO | KBD_PERR)) {
			continue;
		}
		return (data & 0xff);
	} while (--n);
	return (-1);		/* timed-out if fell through to here...
				 */
}

static void kbd_write(int address, int data)
{
	int status;

	do {
		status = inb(KBD_STATUS_REG);	/* spin until input buffer empty */
	} while (status & KBD_IBF);
	outb(data, address);	/* write out the data */
}

void kbd_init(void)
{
	unsigned long flags;

	save_flags(flags);
	cli();

	/* Flush any pending input. */
	while (kbd_wait_for_input() != -1)
		continue;

	/* Test the keyboard interface. This seems to be the only way to get it
	 * going. If the test is successful a x55 is placed in the input buffer. */
	kbd_write(KBD_CNTL_REG, KBD_SELF_TEST);
	if (kbd_wait_for_input() != 0x55) {
		printk("initialize_kbd: keyboard failed self test.\n");
		restore_flags(flags);
		return;
	}

	/* Perform a keyboard interface test.  This causes the controller to test
	 * the keyboard clock and data lines.  The results of the test are placed
	 * in the input buffer. */
	kbd_write(KBD_CNTL_REG, KBD_SELF_TEST2);
	if (kbd_wait_for_input() != 0x00) {
		printk("initialize_kbd: keyboard failed self test 2.\n");
		restore_flags(flags);
		return;
	}

	/* Enable the keyboard by allowing the keyboard clock to run. */
	kbd_write(KBD_CNTL_REG, KBD_CNTL_ENABLE);

	/* Reset keyboard. If the read times out then the assumption is that no
	 * keyboard is plugged into the machine. This defaults the keyboard to
	 * scan-code set 2. */
	kbd_write(KBD_DATA_REG, KBD_RESET);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk("initialize_kbd: reset kbd failed, no ACK.\n");
		restore_flags(flags);
		return;
	}

	if (kbd_wait_for_input() != KBD_POR) {
		printk("initialize_kbd: reset kbd failed, not POR.\n");
		restore_flags(flags);
		return;
	}

	/* now do a DEFAULTS_DISABLE always */
	kbd_write(KBD_DATA_REG, KBD_DISABLE);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk("initialize_kbd: disable kbd failed, no ACK.\n");
		restore_flags(flags);
		return;
	}

	/* Enable keyboard interrupt, operate in "sys" mode,  enable keyboard (by
	 * clearing the disable keyboard bit),  disable mouse, do conversion of
	 * keycodes. */
	kbd_write(KBD_CNTL_REG, KBD_WRITE_MODE);
	kbd_write(KBD_DATA_REG, KBD_EKI | KBD_SYS | KBD_DMS | KBD_KCC);

	/* now ENABLE the keyboard to set it scanning... */
	kbd_write(KBD_DATA_REG, KBD_ENABLE);
	if (kbd_wait_for_input() != KBD_ACK) {
		printk("initialize_kbd: keyboard enable failed.\n");
		restore_flags(flags);
		return;
	}

	restore_flags(flags);

	return;
}

int kbd_getc_with_timeout(int nsecs)
{
	int scancode;
	int character = 0;
	unsigned long endjifs = jiffies + ((unsigned long) nsecs * 1000L);

	sti();

	for (;;) {

		for (;;) {

			/* Read a scancode from the keyboard... */
			if ((inb_p(0x64) & 0x21) == 1) {
#ifdef MINI_SERIAL_ECHO
				uart_enabled = 0;
#endif
				scancode = inb(0x60);
				break;
			}
#ifdef MINI_SERIAL_ECHO

			/* Allow input from both uart & kbd */
			if (uart_charav(SERIAL_PORT)) {
				uart_enabled = 1;
				return (uart_getchar(SERIAL_PORT));
			}
#endif

			if (nsecs) {
				if (jiffies > endjifs) {
					return -1;
				}
			}
		}

		/* If it's an extended function, ignore it... */
		if (scancode == SCANCODE_EXTENDED) {
			continue;
		}

		/* Process certain key releases first...  */
		if (scancode & SCANCODE_KEYUP) {
			scancode &= ~SCANCODE_KEYUP;

			/* Did we release a SHIFT key? */
			if (((scancode == SCANCODE_LEFTSHIFT) ||
			     (scancode == SCANCODE_RIGHTSHIFT)) &&
			    (keyboard_state == KBD_SHIFTED)) {
				keyboard_state = KBD_NORMAL;
				continue;
			}

			/* Did we release a CTRL key? */
			if ((scancode == SCANCODE_CTRL)
			    && (keyboard_state == KBD_CTRL)) {
				keyboard_state = KBD_NORMAL;
				continue;
			}

			/* Other releases aren't important to us... */
			continue;
		}

		/* It's a keypress.  Is it a ctrl? */
		if (scancode == SCANCODE_CTRL) {
			keyboard_state = KBD_CTRL;
			continue;
		}

		/* Is it a shift? */
		if (((scancode == SCANCODE_LEFTSHIFT) ||
		     (scancode == SCANCODE_RIGHTSHIFT)) &&
		    (keyboard_state != KBD_CTRL)) {
			keyboard_state = KBD_SHIFTED;
			continue;
		}

		/* Is it a capslock? */
		if ((scancode == SCANCODE_CAPSLOCK)
		    && (keyboard_state != KBD_CTRL)) {
			if (keyboard_state == KBD_NORMAL) {
				keyboard_state = KBD_SHIFTED;
			} else {
				keyboard_state = KBD_NORMAL;
			}
			continue;
		}

		/* It's none of these.  Find the right table and translate... */
		switch (keyboard_state) {
		case KBD_NORMAL:
			character = plain_charmap[scancode];
			break;
		case KBD_SHIFTED:
			character = shift_charmap[scancode];
			break;
		case KBD_CTRL:
			character = ctrl_charmap[scancode];
			break;
		}

		if (character) {
			return character;
		}
	}
}

#endif

/* Wait for keyboard, and return whatever data the it keyboard has for us... */
int kbd_getc(void)
{
	return (kbd_getc_with_timeout(0));
}

void wait_for_keypress(void)
{
	kbd_getc();
}

static void backspace(int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		__video_putchar('\b');
		__video_putchar(' ');
		__video_putchar('\b');
#ifdef MINI_SERIAL_ECHO
		uart_putchar(SERIAL_PORT, '\b');
		uart_putchar(SERIAL_PORT, ' ');
		uart_putchar(SERIAL_PORT, '\b');
#endif
	}
}


/* Very simple-minded get-string function.  Understands rubout and CR, 
 * and character echo, but not much else...  Cuts off any trailing
 * newline, and null-terminates the return string.  Returns the number
 * of characters read.
 */
int kbd_gets(char *charbuf, int maxlen)
{
	int bufpos;
	char c;

	bufpos = 0;
	for (;;) {
		while ((c = kbd_getc()) == '\000');
		switch (c) {
		case 0x08:
		case 0x7f:

			/* Backspace or rubout means to get rid of the previous
			 * character, if any. */
			if (bufpos > 0) {
				--bufpos;
				backspace(1);
			}
			break;

		case 0x15:

			/* ctrl-U means erase to beginning of line */
			backspace(bufpos);
			bufpos = 0;
			break;

		case 0x0d:
		case 0x0a:

			/* CR or LF means to null-terminate the string and return the
			 * number of characters read. */
			__video_putchar('\r');
			__video_putchar('\n');
#ifdef MINI_SERIAL_ECHO
			uart_putchar(SERIAL_PORT, '\r');
			uart_putchar(SERIAL_PORT, '\n');
#endif
			charbuf[bufpos] = '\0';
			return (bufpos);

		default:

			/* Stuff this character into the buffer (if we have room and
			 * its valid) then advance the character pointer... */
			if (c < ' ' && c != '\t')
				break;
			if ((c == ' ' || c == '\t') && bufpos == 0)
				break;

			if ((bufpos + 1) < maxlen) {
				charbuf[bufpos++] = c;
				__video_putchar(c);
#ifdef MINI_SERIAL_ECHO
				uart_putchar(SERIAL_PORT, c);
#endif
			}
			break;
		}
	}
}
