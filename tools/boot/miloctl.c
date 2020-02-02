/*
 *  MILOctl - MILO parameters control utility.
 *  Copyright (C) 1998 Nikita Schmidt  <cetus@snowball.ucd.ie>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../../flags.h"

union mblk {
    u_int64_t boot[64];
    struct {
	u_int32_t cmdsig;
	char cmdvar[14];	/* must be equal to sizeof ("OSLOADOPTIONS") */
	char cmd[750];
	u_int32_t memsig;
	u_int32_t mem;
	u_int64_t flags;
	char reserved[240];
    } param;
};

static struct flag_d {
    unsigned long val;
    char *name;
    char *nick;
} flags[] = {
    { SERIAL_OUTPUT_FLAG,	"SERIAL_OUTPUT",	"S" },
    { VERBOSE_FLAG,		"VERBOSE",		"V" }
};

/* This is valid only on little endian machines */
#define	MILO_SIGNATURE	0x4f4c494d		/* "MILO" */
#define MILO_START	0x47ff041f47ff041fUL	/* nop; nop */

static int usage (char *progname)
{
    int i;

    printf ("Usage: %s [OPTION]... MILO [cmd=CMD] [mem=MEM] [{+|-}FLAG]...\n"
	    "Read or set MILO parameters in the image file given by the MILO argument.\n\n"
	    "  CMD\t  startup command\n"
	    "  MEM\t  memory size in MB\n"
	    "  FLAG\t  one of the following:\n", progname);

    for (i = 0; i < sizeof flags / sizeof (*flags); i++)
	printf ("      %-20s(`%s' for short)\n", flags[i].name, flags[i].nick);

    printf ("      (long flag names are case insensitive)\n\n"
	    "  -v, --verbose     describe the changes being made\n"
	    "  -h, --help        display this help and exit (should be alone)\n"
	    "      --version     output version information and exit (should be alone)\n\n"
	    "The order of arguments is not significant.\n"
	    "If none of the `cmd=', `mem=', or `FLAG' arguments are given,\n"
	    "current settings will be displayed.\n");

    return 0;
}

static char *cmdinfo[] = {
    "Startup command remains unset.\n",
    "Removing old startup command `%s'.\n",
    "Installing new startup command `%s%s'.\n",
    "Replacing old command `%s' with `%s'.\n"
}, *meminfo[] = {
    "Memory size remains unset.\n",
    "Removing old memory size setting.\n",
    "Setting memory size to %u Mb.\n",
    "Changing memory size setting to %u Mb.\n"
};

/*
 * Return codes:
 *   0 - OK
 *   1 - bad usage
 *   2 - error in arguments (command line too long or bad memory size)
 *   3 - system error (file not found, llseek beyond the end, etc.)
 *   4 - not a MILO image
 *   5 - old MILO image (no parameter block)
 */

int main (int argc, char *argv[])
{
    char *filename, *cmd, *mem, *p;
    int i, j, f;
    int verbose, tryhelp, display;
    u_int32_t memory;
    unsigned long offset;
    union mblk b;
    unsigned long set, clear, cflg;

    verbose = tryhelp = display = 0;
    filename = cmd = mem = 0;
    memory = 0;
    set = clear = 0;

    for (i = 1; i < argc; i++)
    {
	p = argv[i];

	if (*p == '-' || *p == '+')
	    for (j = 0; j < sizeof flags / sizeof (*flags); j++)
		if (strcasecmp (p + 1, flags[j].name) == 0
		 || strcmp (p + 1, flags[j].nick) == 0)
		{
		    if (*p == '-')
			clear |= flags[j].val;
		    else
			set |= flags[j].val;
		    goto nextarg;
		}

	if (*p == '-')
	{
	    if (strcmp (p, "--verbose") == 0)
		verbose = 1;
	    else if (strcmp (p, "--help") == 0)
		return usage (argv[0]);
	    else if (strcmp (p, "--version") == 0)
		return puts ("MILO parameters control utility"
			     " version 0.2."), 0;
	    else if (p[1] == '-')
	    {
		fprintf (stderr, "%s: unrecognised option `%s'\n", argv[0], p);
		tryhelp = 1;
		break;
	    } else
		while (*++p)
		    switch (*p)
		    {
			case 'h':
			    return usage (argv[0]);
			case 'v':
			    verbose = 1;
			    break;
			default:
			    fprintf (stderr, "%s: unrecognised option `-%c'\n",
				    argv[0], *p);
			    tryhelp = 1;
		    }
	} else if (strncmp (p, "cmd=", 4) == 0)
	{
	    if (cmd)
		return fprintf (stderr,
			"%s: multiple `cmd' arguments are not allowed\n",
			argv[0]), 1;
	    cmd = p + 4;
	} else if (strncmp (p, "mem=", 4) == 0)
	{
	    if (mem)
		return fprintf (stderr,
			"%s: multiple `mem' arguments are not allowed\n",
			argv[0]), 1;
	    mem = p + 4;
	} else
	{
	    if (filename)
		return fprintf (stderr,
			"%s: too many file names given\n",
			argv[0]), 1;
	    filename = p;
	}
nextarg:
    }

    if (!filename)
    {
	fprintf (stderr, "%s: no MILO file name given\n", argv[0]);
	tryhelp = 1;
    }
    if (tryhelp)
    {
	fprintf (stderr, "Try %s --help for more information.\n", argv[0]);
	return 1;
    }
    if (!cmd && !mem && !set && !clear)
	verbose = display = 1;

    if (cmd && strlen (cmd) >= sizeof (b.param.cmd))
	return fprintf (stderr, "%s: command is too long\n", argv[0]), 2;
    if (mem && *mem && ((memory = atoi (mem)) < 4 || memory > 32767))
	return fprintf (stderr, "%s: illegal memory size specified\n",
		argv[0]), 2;

    /* Open the file and read the first sector. */

    if ((f = open (filename, display ? O_RDONLY : O_RDWR)) == -1)
	return perror (filename), 3;
    if (read (f, b.boot, sizeof (b.boot)) != sizeof (b.boot))
	return fprintf (stderr, "%s: cannot read the first block\n", filename),
	    3;

    /* Check if it's an SRM-style boot block. */

    if (b.boot[0] != MILO_START)
    {
	u_int64_t cksum = 0;

	for (i = 0; i < 63; i++)
	    cksum += b.boot[i];

	if (cksum != b.boot[i])
	    return fprintf (stderr,
		    "%s: file %s does not look like a valid MILO image\n",
		    argv[0], filename), 4;

	if (verbose)
	    printf ("Boot sector found; image starts at %ld, size %ld sectors.\n",
		    b.boot[61], b.boot[60]);

	if (lseek (f, b.boot[61] * 512, SEEK_SET) == -1)
	    return perror (filename), 3;
	if (read (f, b.boot, sizeof (b.boot)) != sizeof (b.boot))
	    return fprintf (stderr, "%s: cannot read the image\n", filename),
		3;

	if (b.boot[0] != MILO_START)
	    return fprintf (stderr,
		    "%s: file %s is a bootable image of something other than MILO\n",
		    argv[0], filename), 4;
    }

    /* Search for the parameter block. */

    offset = sizeof (b.boot) - 0x100;	/* We've read that much so far. */
    do {
	offset += 0x100;
	if (offset == 0x1000)	/* Normally the parblock should be at 0xB00. */
	    return fprintf (stderr, "%s: parameter block not found in %s.\n"
		    "If it really is a MILO image, then it's not recent enough.\n",
		    argv[0], filename), 5;
	if (read (f, &b.param, 0x100) != 0x100)
	    return fprintf (stderr, "%s: cannot read the image\n", filename),
		3;
    } while (b.param.cmdsig != MILO_SIGNATURE);

    if (verbose)
	printf ("MILO parameter block is at offset 0x%lX.\n", offset);

    if (read (f, (char *)&b + 0x100, sizeof b - 0x100) != sizeof b - 0x100)
	return fprintf (stderr, "%s: cannot read the parameter block\n",
		filename), 3;

    if (display)
    {
	if (b.param.cmd[0])
	    printf ("Startup command: `%s'.\n", b.param.cmd);
	else
	    printf ("Startup command is not set.\n");
	if (b.param.memsig == MILO_SIGNATURE && b.param.mem)
	    printf ("Memory size: %d Mb.\n", b.param.mem);
	else
	    printf ("Memory size is not set.\n");
    }

    if (cmd)
    {
	if (verbose)
	{
	    i = (b.param.cmd[0] != '\0') + ((*cmd != '\0') << 1);
	    printf (cmdinfo[i], b.param.cmd, cmd);
	}
	strncpy (b.param.cmd, cmd, sizeof (b.param.cmd));
    }
    if (mem)
    {
	if (verbose)
	{
	    i = (b.param.memsig == MILO_SIGNATURE && b.param.mem)
		+ ((memory != 0) << 1);
	    printf (meminfo[i], memory);
	}
	b.param.memsig = MILO_SIGNATURE;
	b.param.mem = memory;
    }
    b.param.flags = cflg = (b.param.flags & ~clear) | set;

    if (display || ((set | clear) && verbose))
    {
	printf ("Flags: %#lx", cflg);
	if (cflg)
	{
	    j = 0;
	    for (i = 0; i < sizeof flags / sizeof (*flags); i++)
		if (flags[i].val & cflg)
		{
		    cflg &= ~flags[i].val;
		    printf (j ? ", %s" : " (%s", flags[i].name);
		    j++;
		}
	    if (cflg)
		printf (j ? ", %s" : " (%s", "<UNKNOWN>");
	    putchar (')');
	}
	printf (".\n");
    }

    /* Write the changes */

    if (!display)
    {
	if (lseek (f, -sizeof (b.param), SEEK_CUR) == -1)
	    return perror (filename), 3;
	if (write (f, &b.param, sizeof (b.param)) != sizeof (b.param))
	    return fprintf (stderr, "%s: cannot write the parameter block\n",
		    filename), 3;
    }

    close (f);

    return 0;
}
