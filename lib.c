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

#include <linux/string.h>
#include "milo.h"

/*
 * NOTE: This version of atoi () conforms even less to the C standard than the
 *       previous one.  But at least now we've got a way of telling when an
 *       error occurs during parsing: just return -1.
 */
int atoi(const char *s)
{
	int retval = 0;

	while (*s) {
		if ((*s < '0') || (*s > '9')) {
			return -1;
		}
		retval *= 10;
		retval += (*s - '0');
		s++;
	}

	return (retval);
}

char *stpcpy(char *dest, const char *src)
{
	while ((*dest = *src++))
		dest++;

	return dest;
}

int tolower(int c)
{
	if ((c >= 'A') && (c <= 'Z'))
		c = c - 'A' + 'a';
	return c;
}

/* Make a copy of a string in dynamically allocated memory */
char *copy(const char *src)
{
	char *dest = malloc(strlen(src) + 1);

	strcpy(dest, src);
	return dest;
}
