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
/* Very simpleminded environment-variable routines.  In order to avoid
 * having to come up with a whole memory allocator scheme, fraught with
 * fragmentation and whatnot, I just divide the available space into
 * fixed-size chunks.  For now, this just uses a volatile RAM buffer.
 * Depending on platform, I'll use NVRAM or flash as appropriate to
 * hold the environment variables from one invocation to another...
 */

/* Environment area size.  This may vary from platform to platform.
 * Currently only defined for NoName; we'll take a default for the
 * others.
 */

#include <linux/config.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <linux/kernel.h>
#include <string.h>

#include "milo.h"

#ifdef MINI_ALPHA_NONAME
#define ENVIRON_SIZE    2*1024
#else
#define ENVIRON_SIZE	8192
#endif

int milo_verbose = 0;

/* This is the "working" area for the environment.  We use this because
 * copying to/from NVRAM or flash *may* be painful...
 */

static unsigned char environ[ENVIRON_SIZE];

static int environ_initialized = 0;

/* This is the size of the environment chunks */
#define ENVIRON_CHUNK_SIZE	128

#define ENVIRON_NVARS		(ENVIRON_SIZE/ENVIRON_CHUNK_SIZE)


#if defined(MINI_NVRAM)

/* NVRAM functions */
static void fill_env_from_nvram(unsigned char *area, int size)
{
	nvram_copyin(0, area, size);
}

static void save_env_to_nvram(unsigned char *area, int size)
{
	nvram_copyout(0, area, size);
}

void test_nvram()
{
	int i;
	int miscompares = 0;

	printk("Testing NVRAM...\n");
	for (i = 0; i < ENVIRON_SIZE; i++) {
		environ[i] = ((i >> 8) & 0xff) ^ (i & 0xff);
	}

	save_env_to_nvram(environ, ENVIRON_SIZE);

	for (i = 0; i < ENVIRON_SIZE; i++) {
		environ[i] = 0;
	}

	fill_env_from_nvram(environ, ENVIRON_SIZE);

	for (i = 0; i < ENVIRON_SIZE; i++) {
		if (environ[i] != (((i >> 8) & 0xff) ^ (i & 0xff))) {
			miscompares++;
		}
	}

	if (miscompares > 0) {
		printk("NVRAM test failed: %d miscompares\n", miscompares);
	} else {
		printk("NVRAM test passes!\n");
	}
}

void resetenv(void)
{
	int i;

	test_nvram();
	strcpy(environ, ENVIRON_ID);
	for (i = 1; i < ENVIRON_NVARS; i++) {

		/* Mark chunk as free... */
		environ[i * ENVIRON_CHUNK_SIZE] = '\0';
	}
	save_env_to_nvram(environ, ENVIRON_SIZE);
}
#endif


/* Initialize the environment area.  Copy it out of NVRAM/flash
 * if necessary and initialize it.
 */
void env_init(void)
{
#if !defined(MINI_NVRAM)
	memset(environ, 0, ENVIRON_SIZE);
	environ_initialized = 1;
	if (milo_verbose)
		printk("NVRAM is not supported on this system\n");
#else
	char *p;

	fill_env_from_nvram(environ, ENVIRON_SIZE);

	/* If it is not valid, then give up. */
	if (strcmp(environ, ENVIRON_ID) != 0) {
		printk("No environment present\n");
	}
	environ_initialized = 1;
	if ((p = getenv("VERBOSE")))
		milo_verbose = atoi(p);
#endif
}

void setenv(char *var, char *value)
{
	int len;
	int i;
	char *cp;
	char *firstfree = NULL;

#if defined(MINI_NVRAM)
	if (!environ_initialized) {
		env_init();
	}
#endif

	/* Go through the existing chunks.  If we find this variable already,
	 * replace it.  Otherwise, allocate a new chunk. */

	len = strlen(var);

	for (i = 1; i < ENVIRON_NVARS; i++) {
		cp = &(environ[i * ENVIRON_CHUNK_SIZE]);
		if ((firstfree == NULL) && (*cp == '\0')) {
			firstfree = cp;
		}
		if ((strncmp(cp, var, len) == 0) && (*(cp + len) == '=')) {

			/* We found it!  Replace the whole thing... */
			strcpy(cp, var);
			strcat(cp, "=");
			strcat(cp, value);
#if defined(MINI_NVRAM)
			save_env_to_nvram(environ, ENVIRON_SIZE);
#endif
			return;
		}
	}

	/* Not found.  Need to build a new one... */
	if (firstfree) {
		strcpy(firstfree, var);
		strcat(firstfree, "=");
		strcat(firstfree, value);
#if defined(MINI_NVRAM)
		save_env_to_nvram(environ, ENVIRON_SIZE);
#endif
		return;
	} else {
		printk
		    ("MILO: Cannot set environment variable %s: No space left\n",
		     var);
		return;
	}
}

void unsetenv(char *var)
{
	int len;
	int i;
	char *cp;

#if defined(MINI_NVRAM)
	if (!environ_initialized) {
		env_init();
	}
#endif

	/* Go through the existing chunks.  If we find this variable, delete it. */

	len = strlen(var);

	for (i = 1; i < ENVIRON_NVARS; i++) {
		cp = &(environ[i * ENVIRON_CHUNK_SIZE]);
		if ((strncmp(cp, var, len) == 0) && (*(cp + len) == '=')) {

			/* We found it!  Zap it... */
			*cp = '\0';
#if defined(MINI_NVRAM)
			save_env_to_nvram(environ, ENVIRON_SIZE);
#endif
			return;
		}
	}

	/* Not found... don't do anything... */
	return;
}


char *getenv(char *var)
{
	int len;
	int i;
	char *cp;

#if defined(MINI_NVRAM)
	if (!environ_initialized) {
		env_init();
	}
#endif

	/* Go through the existing chunks.  If we find this variable, return its
	 * value. */

	len = strlen(var);

	for (i = 1; i < ENVIRON_NVARS; i++) {
		cp = &(environ[i * ENVIRON_CHUNK_SIZE]);
		if ((strncmp(cp, var, len) == 0) && (*(cp + len) == '=')) {

			/* We found it!  Return its value. */
			return (cp + len + 1);
		}
	}

	/* Not found... don't do anything... */
	return ((char *) 0);
}

void printenv()
{
	int i;
	char *cp;

	if (!environ_initialized) {
		env_init();
	}

	for (i = 1; i < ENVIRON_NVARS; i++) {
		cp = &(environ[i * ENVIRON_CHUNK_SIZE]);
		if (*cp) {
			printk("%s\n", cp);
		}
	}
}
