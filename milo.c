#define LOAD_RAMDISK 1

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
 *  MIniLOader command interface - MILO
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
#include <linux/compile.h>
#include <linux/malloc.h>

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

/* From hwrpb.c - it ought not to be here, but just for now... */
extern struct hwrpb_struct *hwrpb;

typedef struct {
	int dev;
	char *path;
} path_t;

/*
 *  External things - all from boot.c
 */
extern U64 *pkernel_args;
extern U64 kernel_at;
extern U64 kernel_entry;
extern U64 kernel_pages;

extern struct pcb_struct boot_pcb;

extern unsigned int ptbr;
extern U64 vptbr;

extern unsigned char default_PCI_master_latency;
extern void dev_config(void);
extern void calibrate_delay(void);

extern int printk_rows;

char *scratch = NULL;

static char *__PULL(void);

static void ls_parse(void);
static void run_parse(void);
static void read_parse(void);
#ifdef MINI_NVRAM
static void resetenv_parse(void);
#endif
static void setenv_parse(void);
static void unsetenv_parse(void);
static void show_parse(void);
static void sleep_parse(void);
static void pci_parse(void);
static void cd_parse(void);
static path_t path_parse(int);

static void parse_command(void);

static void boot_parse(void);
static void help_parse(void);

#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
static void bootopt_parse(void);
#endif

#ifdef DEBUG_MILO
static void reboot_parse(void);
#endif

static void milo_whatami(void);
static int do_timeout(int timeout);

static void parse_command_string(char *string);

struct cmdoption {
	char *command;
	struct cmdoption *next;
};

struct config_image {
	char *label;
	char *device;
	char *image;
	char *root;
	char *append;
	struct cmdoption *cmdlist;
};

struct command {
	char *name;
	char *help_args;
	char *help_text;
	void (*action_rtn) (void);
};

static struct command commands[] = {
/*
 *  Device specific stuff, showing, booting, running, listing.
 */
	{"ls",
	 "[<path>]",
	 "List files in directory on device",
	 &ls_parse},
	{"cd",
	 "<path>",
	 "Change current working directory",
	 &cd_parse},
	{"boot",
	 "[<path> {<arg>}]",
	 "Boot the specified kernel image",
	 &boot_parse},
	{"read",
	 "[<dev>:] <block>",
	 "Read <dev> starting from given <block>",
	 &read_parse},
	{"run",
	 "<path>",
	 "Run a standalone program",
	 &run_parse},
	{"show",
	 "",
	 "Display all known devices and file systems",
	 &show_parse},
	{"sleep",
	 "<secs>",
	 "Sleep for <secs> seconds",
	 &sleep_parse},
	{"pci",
	 "",
	 "Display PCI configuration",
	 &pci_parse},
/*
 *  Environment variable stuff.
 */
#if defined(MINI_NVRAM)
	{"setenv",
	 "VAR VALUE",
	 "Set the variable VAR to the specified VALUE",
	 &setenv_parse},
	{"unsetenv",
	 "VAR",
	 "Delete the specified environment variable",
	 &unsetenv_parse},
	{"resetenv",
	 "",
	 "Delete all environment variables",
	 &resetenv_parse},
#else
	{"unset",
	 "<var>",
	 "Delete the specified variable",
	 &unsetenv_parse},
	{"set",
	 "[<var> <value>]",
	 "Set <var> to <value>, or display variables",
	 &setenv_parse},
#if 0
	{"reset",
	 "",
	 "Delete all variables",
	 &resetenv_parse},
#endif
#endif
/*
 *  System specific commands
 */
#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
	{"bootopt",
	 "num",
	 "Select firmware type to use on next power up",
	 &bootopt_parse},
#endif
#ifdef DEBUG_MILO
/*
 *  Debug commands.
 */
	{"reboot",
	 "",
	 "Reset milo (reboot)",
	 &reboot_parse},
#endif
/*
 *  Help
 */
	{"help",
#if defined(MINI_NVRAM)
	 "[env]",
#else
	 "",
#endif
	 "Print this help text",
	 &help_parse},
};


#define NUM_MILO_COMMANDS (sizeof(commands) / sizeof(struct command))
#define MAX_TOKENS 10

char *tokens[MAX_TOKENS];
int ntokens = 0;

int curdev = -1;
#define curpath getwd (curdev)->path

static char *__PULL()
{
	char *value = NULL;

	if (ntokens > 0) {
		int i;

		value = tokens[0];
		for (i = 1; i < ntokens; i++)
			tokens[i - 1] = tokens[i];
		ntokens--;
	}
	return value;
}

#define __ADD(value) tokens[ntokens++] = value

/*****************************************************************************
 *  Action routines                                                          *
 *****************************************************************************/
/*
 *  The linux kernel is either on one of the known devices or it is linked
 *  into this image at a known location.
 */

static void boot_cmd(char *fs_type,
		     char *devname, char *filename, char *boot_string)
{
	U64 entry, milo_rd_start=0, milo_rd_size=0;
	U64 *kernel_args;
	char *where;

#if 0
	int milo_rd_num_pages;
#endif

	entry = load_image_from_device(fs_type, devname, filename);

	if (entry == 0) {
		printk("MILO: Failed to load the kernel\n");
		return;
	}

	/* now we've loaded the kernel, mark the memory that it occupies as in use
	 */
#ifdef DEBUG_BOOT
	printk("...freeing original marked kernel pages: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(KERNEL_START),
		      _PHYSICAL_TO_PFN(MAX_KERNEL_SIZE), FREE_PAGE);
#ifdef DEBUG_BOOT
	printk("...marking real kernel pages as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(KERNEL_START),
		      kernel_pages, ALLOCATED_PAGE);

#if LOAD_RAMDISK
	load_ramdisk_from_device(fs_type, devname, "initdisk.gz");
	milo_rd_start = *(unsigned long *) (ZERO_PGE + 0x100);
	milo_rd_size = *(unsigned long *) (ZERO_PGE + 0x108);
#endif
/*****************************************************************************
 *  Setup the reboot block for when we reboot.                               *
 *****************************************************************************/
	if (milo_reboot_block != NULL) {
		if (milo_verbose)
			printk
			    ("Setting up Milo reboot parameter block at 0x%p\n",
			     milo_reboot_block);
		milo_reboot_block->boot_filesystem =
		    fs_name_to_type(fs_type);
		milo_reboot_block->boot_device =
		    device_name_to_number(devname);
		strncpy(milo_reboot_block->boot_filename, filename,
			sizeof(milo_reboot_block->boot_filename));
		if (boot_string == NULL)
			milo_reboot_block->boot_string[0] = '\0';
		else
			strncpy(milo_reboot_block->boot_string,
				boot_string,
				sizeof(milo_reboot_block->boot_string));

		/* now set the reboot flag so that we do the right thing next time */
		milo_reboot_block->flags |= REBOOT_REBOOT;
	}

	/* Set hwrpb->cycle_freq according to CPU_CLOCK, if any */
	if ((where = getenv("CPU_CLOCK")))
	hwrpb->cycle_freq = atoi(where) * 1000000L;

/*****************************************************************************
 *  Build the final memory map                                               *
 *****************************************************************************/

	/* This must be done when all required memory has been allocated */
	make_HWRPB_virtual();
	build_HWRPB_mem_map();

/*****************************************************************************
 *  Set up the area where we pass any boot arguments.                        *
 *****************************************************************************/

	/* for now, just make sure that it is zero */
	memset((void *) ZERO_PGE, 0, PAGE_SIZE);

	strcat((char *) ZERO_PGE, "bootdevice=");
	strcat((char *) ZERO_PGE, devname);
	strcat((char *) ZERO_PGE, " bootfile=");
	strcat((char *) ZERO_PGE, filename);
	if (boot_string != NULL) {
		strcat((char *) ZERO_PGE, " ");
		strcat((char *) ZERO_PGE, boot_string);
	}
#if LOAD_RAMDISK
	*(unsigned long *) (ZERO_PGE + 0x100) =
	    (unsigned long) milo_rd_start;
	*(unsigned long *) (ZERO_PGE + 0x108) =
	    (unsigned long) milo_rd_size;
#endif
	if (milo_verbose)
		printk("Boot line is %s\n", (char *) ZERO_PGE);

/*****************************************************************************
 *  Set up the Kernel calling information block                              *
 *****************************************************************************/
/*
 *  This is located below this image in memory.  It is 7*8 bytes long and
 *  contains the following:
 * 
 *  Offset      Contents
 *    0*8       Linux kernel entry point
 */
	pkernel_args = kernel_args = (U64 *) LOADER_AT + 8;
	kernel_args[0] = entry;

	if (milo_verbose)
		printk("Bootstrap complete, kernel @ 0x%lx, sp @ 0x%lx\n",
		       entry, (U64) (INIT_STACK + PAGE_SIZE));

/*****************************************************************************
 *  Swap to the new page tables and jump to the Linux Kernel                 *
 *****************************************************************************/

	/* change context */
	if (milo_verbose)
		printk
		    ("...turning on virtual addressing and jumping to the Linux Kernel\n");

	/* build the pcb */
	boot_pcb.ksp = INIT_STACK + PAGE_SIZE;
	boot_pcb.usp = 0;
	boot_pcb.ptbr = ptbr;
	boot_pcb.pcc = 0;
	boot_pcb.asn = 0;
	boot_pcb.unique = 0;
	boot_pcb.flags = 1;

	/* bump up the ipl */
	cli();

	/* now switch to the new scheme of things. */
	printk
	    ("\n--------------------------------------------------------------------------------\n");
	swap_to_palcode((void *) PALCODE_AT, (void *) entry,
			(void *) INIT_STACK + PAGE_SIZE, ptbr,
			(void *) vptbr);
/*
 *  If we ever get to this point, something has gone seriously wrong!!!
 */
#ifdef NEVER
	error("If you can read this, something went VERY wrong!");
#endif
}

static path_t *getwd(unsigned int devnum)
{
	typedef struct WD_T {
		path_t wd;
		struct WD_T *next;
	} wd_t;

	static wd_t head = { {0, NULL}, NULL };
	wd_t *p = &head;

	/* Search for working directory associated with the specified device */
	while (p->wd.dev != devnum && p->next)
		p = p->next;

	/* Either we've found the path now, or we're at the end of the list. */
	if (p->wd.dev != devnum) {
		p->next = malloc(sizeof(wd_t));
		p = p->next;

		p->wd.dev = devnum;
		p->wd.path = copy("");
		p->next = NULL;
	}

	/* Return path of working directory */
	return &(p->wd);
}

static int chdev(int devnum)
{
	int channel = device_mount(dev_name(devnum), NULL);

	if (channel < 0)
		return -1;

	curdev = devnum;
	__close(channel);
	return channel;
}

static int chdir(path_t dir)
{
	path_t *tmp = getwd(dir.dev);
	int channel = device_mount(dev_name(dir.dev), NULL);

	if (dir.path == NULL)
		return chdev(dir.dev);

	/* Otherwise first check if supplied path really exists */
	if (channel < 0 || __open(dir.path) < 0) {
		printk("Invalid argument: %s\n", dir.path);
		if (channel >= 0)
			__close(channel);
		return -1;
	}

	/* FIXME: We should really check if the supplied path is a directory */

	/* If the supplied path exists change working directory */
	free(tmp->path);
	tmp->path = copy(dir.path);

	__close(channel);
	return 0;
}

static int parse_config_line(char *line, char **option, char **value)
{
	char *chptr;
	int has_quote = 0;

	chptr = line;
	while (*chptr && (*chptr == ' ' || *chptr == '\t'))
		chptr++;
	if (*chptr == '#' || !*chptr) {
		*option = NULL;
		return 0;
	}

	*option = chptr;

	while (*chptr && *chptr != ' ' && *chptr != '\t' && *chptr != '=')
		chptr++;
	if (!*chptr)
		return 1;
	line = chptr;

	while (*chptr && *chptr != '=')
		chptr++;
	if (!*chptr)
		return 1;
	*line = '\0';

	chptr++;
	while (*chptr == ' ' || *chptr == '\t')
		chptr++;
	if (!*chptr) {
		*value = NULL;
		return 1;
	}

	if (*chptr == '"') {
		has_quote = 1;
		chptr++;
	}

	*value = chptr;

	chptr = *value + strlen(*value) - 1;
	while (*chptr == ' ' || *chptr == '\t')
		chptr--;

	if (has_quote && (*chptr != '"')) {
		*value = NULL;
		return 1;
	} else if (has_quote)
		chptr--;

	*(chptr + 1) = '\0';

	return 0;
}

void boot_config(struct config_image *image, char *fs_type, char *defdev,
		 char *optionstr)
{
	char **options;
	char *chptr, *start;
	int num_options;
	char *full_options, *options_buf;
	struct cmdoption *cmdp;
	int i = 0;
	int j;

	for (cmdp = image->cmdlist; cmdp; cmdp = cmdp->next)
		parse_command_string(cmdp->command);

	if (image->root)
		i += strlen(image->root) + 10;
	if (image->append)
		i += strlen(image->append);
	if (optionstr)
		i += strlen(optionstr);

	if (i) {
		/* build up the argument string, eliminating duplicated arguments
		   as necessary */
		full_options = kmalloc(i + 5, 0);
		*full_options = '\0';

		if (image->root) {
			strcpy(full_options, "root=");
			strcat(full_options, image->root);
			strcat(full_options, " ");
		}
		if (image->append) {
			strcat(full_options, image->append);
			strcat(full_options, " ");
		}
		if (optionstr)
			strcat(full_options, optionstr);

		/* guess at an upper estimate for the number of options we should worry
		   about */

		i = 0;
		for (chptr = full_options; *chptr; chptr++)
			if (*chptr == ' ')
				i++;

		options = kmalloc(sizeof(*options) * i, 0);

		start = full_options;
		num_options = 0;
		while (*start) {
			while (*start && (*start == ' ' || *start == '\t'))
				start++;
			if (!*start)
				break;

			chptr = start;
			while (*chptr && *chptr != '=' && *chptr != ' ')
				chptr++;

			for (i = 0; i < num_options; i++) {
				if (options[i]
				    && !strncmp(start, options[i],
						chptr - start)) break;
			}

			if (i < num_options) {
				options[i] = start;
			} else {
				options[num_options++] = start;
			}

			while (*chptr && *chptr != ' ')
				chptr++;
			*chptr = '\0';
			start = chptr + 1;
		}

		j = 0;
		for (i = 0; i < num_options; i++)
			j += strlen(options[i]);

		options_buf = kmalloc(j + num_options, 0);
		*options_buf = '\0';

		for (i = 0; i < num_options; i++) {
			strcat(options_buf, options[i]);
			strcat(options_buf, " ");
		}

		options_buf[j + num_options] = '\0';
		kfree(full_options);
		kfree(options);
	} else {
		options_buf = NULL;
		options = NULL;
	}

	start = image->image;
	if ((chptr = strchr(start, ':'))) {
		defdev = start;
		*chptr = '\0';
		start = chptr + 1;
	}
	boot_cmd(fs_type, defdev, start, options_buf);

	if (options_buf)
		kfree(options_buf);
}

/* Read a config file and use it as a menu for booting */
static void boot_via_config(char *fs_type, char *devname, char *bootstring)
{
	int channel, fd, i;
	char *buf, *iobuf, *chptr, *start, *option, *value;
	int blkno = 0;
	int rc, num_images;
	long size;
	struct config_image *images;
	char *command_string;
	int timeout = -1;
	char *fed = NULL;

	channel = device_mount(devname, fs_type);
	if (channel)
		return;

	fd = __open("/etc/milo.conf");
	if (fd < 0) {
		fd = __open("/milo.cfg");
		if (fd < 0) {
			printk
			    ("MILO: failed to open /etc/milo.conf or /milo.cfg\n");
			return;
		}
	}

	size = __fdsize(fd);

	chptr = buf = kmalloc(size + 2, 0);

	iobuf = kmalloc(IOBUF_SIZE + 512, 0) + 512;
	iobuf = (char *) ((unsigned long) iobuf & (~(512 - 1)));
	while ((rc = __bread(fd, blkno++, iobuf)) == 0) {
		memcpy((char *) chptr, iobuf, IOBUF_SIZE);
		chptr += IOBUF_SIZE;
		if ((chptr - buf) >= size)
			break;
	}

	kfree(iobuf);
	__close(channel);

	buf[size] = '\n';
	buf[size + 1] = '\0';

	/* half the number of newlines puts a nice, easy upper bound on the number
	   of images we may want to boot (each image has an image and a label) */
	for (num_images = 0, chptr = buf; *chptr; chptr++) {
		if (*chptr == '\n')
			num_images++;
	}

	num_images = num_images / 2;
	images = kmalloc(sizeof(*images) * num_images, 0);
	memset(images, 0, sizeof(*images) * num_images);
	num_images = 0;

	start = buf;
	while (*start) {
		chptr = start;
		while (*chptr != '\n')
			chptr++;

		*chptr = '\0';
		iobuf = chptr + 1;

		if (parse_config_line(start, &option, &value) ||
		    (option && !value)) {
			printk("MILO: error in config file in '%s'\n",
			       start);
			kfree(buf);
			kfree(images);
			return;
		}

		start = iobuf;

		if (!option)
			continue;

		if (!strcmp(option, "timeout")) {
			timeout = atoi(value);
			if (timeout > 10000 || timeout < 0) {
				printk("MILO: illegal timeout value %d\n",
				       timeout);
				timeout = -1;
			} else {
				timeout /= 10;	/* convert to seconds */
			}
		} else if (!strcmp(option, "image")) {
			num_images++;
			images[num_images - 1].image = value;
		} else if (num_images && !strcmp(option, "device")) {
			images[num_images - 1].device = value;
		} else if (num_images && !strcmp(option, "root")) {
			images[num_images - 1].root = value;
		} else if (num_images && !strcmp(option, "label")) {
			images[num_images - 1].label = value;
		} else if (num_images && !strcmp(option, "append")) {
			images[num_images - 1].append = value;
		} else if (!strcmp(option, "command")) {
			if (!num_images)
				parse_command_string(value);
			else {	/* Append command to the list */
				struct cmdoption **p =
				    &images[num_images - 1].cmdlist;
				while (*p)
					p = &(*p)->next;
				*p = kmalloc(sizeof(**p), 0);
				(*p)->command = value;
				(*p)->next = 0;
			}
		} else {
			printk
			    ("MILO: ignoring unknown option '%s' in config file\n",
			     option);
		}
	}

	if (!num_images) {
		printk("MILO: no images are defined in the config file\n");
		kfree(images);
		kfree(buf);
		return;
	}

	if (timeout > 0)
		fed = do_timeout(timeout) ? NULL : images[0].label;
	else if (timeout == 0)
		fed = images[0].label;

	if (!fed) {
		printk("\nAvailable configurations: ");
		for (i = 0; i < num_images; i++)
			printk("%s ", images[i].label);
		printk("\n(or `halt' to return back to MILO prompt)\n");
	}

	command_string = (char *) kmalloc(2 * 256, 0);
	while (1) {
		if (!fed) {
			printk("boot: ");
			kbd_gets(command_string, 256);
			start = command_string;
		} else {
			start = fed;
			fed = NULL;
		}

		while (*start == ' ')
			start++;
		if (!*start)
			continue;

		chptr = start;
		while (*chptr && *chptr != ' ')
			chptr++;
		if (*chptr) {
			*chptr++ = '\0';
			while (*chptr && *chptr == ' ')
				chptr++;
			/* leave chptr at the beginning of the arguments */
		}

		if (!strcmp(start, "halt"))
			break;

		for (i = 0; i < num_images; i++)
			if (!strcmp(start, images[i].label))
				break;

		if (i < num_images) {
			if (images[i].device == NULL)
				boot_config(images + i, fs_type, devname,
					    chptr);
			else
				boot_config(images + i, fs_type,
					    images[i].device, chptr);
		} else {
			printk("MILO: Unknown boot label %s\n", start);
		}
	}

	kfree(command_string);
	kfree(images);
	kfree(buf);
}

/* Load a standalone executable file from disk and run it.  This is 
 * different from boot in that we don't have to set up the system to run
 * Linux.  At the moment, updateflash is the only such executable.
 */
static int run_cmd(char *fs_type, char *devname, char *filename)
{
	U64 entry;
	unsigned long flags;

	entry = load_image_from_device(fs_type, devname, filename);
	if (entry) {
		void (*routine_address) (void);

		/* call the image that we have just loaded. */
		routine_address = (void (*)(void)) entry;

		save_flags(flags);
		cli();
		(routine_address) ();
		restore_flags(flags);

		/* If we get here, then I guess we can return to the command loop...
		 * although there's no guarantees as to what the program may have done 
		 * to our state... */
		return (0);
	}
	return (-1);
}

static void ls_cmd(char *fs_type, char *lsdev, char *lsdir)
{
	int channel;

	channel = device_mount(lsdev, fs_type);
	if (channel < 0) {
		printk("MILO: Cannot access device %s\n", lsdev);
		return;
	}
	__ls(lsdir, lsdev);
	__close(channel);
}

static void read_cmd(char *lsdev, int blknum)
{
	unsigned char buf[24 * 16];	/* One screenful at a time */
	int rc, i, n;
	long offset = blknum * 512;	/* For partitions up to 512 GB... */
	int dummyfd = -1;

	dummyfd = device_open(lsdev);
	if (dummyfd < 0) {
		printk("MILO: Cannot access device %s\n", lsdev);
		return;
	}

	do {
		rc = device_read(dummyfd, buf, sizeof(buf), offset);

		if (rc < 0) {
			printk("\nRead of block from %s:[%d] failed\n",
			       lsdev, blknum);
			return;
		}
		printk("\n");
		for (n = 0; n < rc; n += 16) {
			printk("%010lx:", offset + n);
			for (i = 0; i < 16; ++i)
				printk(" %02x", buf[n + i]);
			printk("  |");
			for (i = 0; i < 16; ++i) {
				int c = buf[n + i];

				printk("%c",
				       (c <= ' ' || c > 126) ? '.' : c);
			}
			printk("|\n");
		}
		printk
		    ("                  -- q to quit - any other key to continue --");
		offset += sizeof(buf);
	}
	while (kbd_getc() != 'q');

	printk("\n");
	device_close(dummyfd);
}

static void sleep_parse(void)
{
	unsigned int opt = 0;
	char *token = __PULL();

	if (token == NULL || __PULL()) {
		printk("Syntax error: wrong argument count\n");
		return;
	}

	opt = atoi(token);

	if (opt < 0 || opt > 255)
		printk("Invalid argument: %s\n", token);
	else {
		extern unsigned long volatile jiffies;
		unsigned long until = jiffies + ((unsigned long) opt * HZ);

		while (jiffies < until)
			continue;
	}
}

static void pci_parse(void)
{
#ifdef CONFIG_PCI_OLD_PROC
	char *buffer;

	buffer = (char *) kmalloc(PAGE_SIZE, 0);

	if (buffer == NULL) {
		printk("pci_parse: kmalloc failed.\n");
		return;
	}

	printk("pci_parse: get_pci_list();\n");
	get_pci_list(buffer);
	printk(buffer);
	kfree(buffer);
#else
	printk("Sorry, PCI device list needs CONFIG_PCI_OLD_PROC.\n");
#endif
}

/*****************************************************************************
 *  Parsing action routines                                                  *
 *****************************************************************************/

#define PATH_COMPLETE_DEV 1
#define PATH_COMPLETE_PATH 2

static path_t path_parse(int flags)
{
	char *token = __PULL();
	path_t path = { 0, NULL };

	/* path ::= [ <dev> ':' ] [ '/' ] <name> | <dev> ':' */
	if (token != NULL) {
		char *tmp = strchr(token, ':');

		/* Parse device if given */
		if (tmp) {
			*tmp++ = '\0';
			path.dev = device_name_to_number(token);
			if (path.dev < 0)
				printk("Invalid argument: %s\n", token);
			token = tmp;

			if (*token == '\0'
			    && !(flags & PATH_COMPLETE_PATH)) return path;
		} else
			path.dev = curdev;

		/* Does <name> start with a `/' ? */
		if (*token == '/') {
			path.path = copy(token + 1);
			return path;
		} else {
			path.path = copy(getwd(path.dev)->path);

			/* Process leading '.' or '..' */
			while (token[0] == '.') {
				if (token[1] == '\0')
					return path;
				else if (token[1] == '/')
					token += 2;
				else if (token[1] == '.'
					 && (token[2] == '\0'
					     || token[2] == '/')) {
					/* Go to subdir */
					tmp =
					    path.path + strlen(path.path);
					while (tmp != path.path
					       && *tmp != '/') tmp--;
					*tmp = '\0';
					if (token[2] == '\0')
						return path;
					else
						token += 3;
				} else
					break;	/* just a plain dot-file */
			}

			if (*token) {
				tmp =
				    malloc(strlen(path.path) +
					   strlen(token) + 2);
				if (path.path[0] != '\0')
					stpcpy(stpcpy
					       (stpcpy(tmp, path.path),
						"/"), token);
				else
					strcpy(tmp, token);
				free(path.path);
				path.path = tmp;
			}
		}
	} else if (flags & PATH_COMPLETE_DEV) {
		path.dev = curdev;
		if (flags & PATH_COMPLETE_PATH)
			path.path = copy(curpath);
		else
			path.path = NULL;
	}

	return path;
}

static void cd_parse()
{
	path_t dir = path_parse(0);

	if (dir.dev == 0 || __PULL())
		printk("Syntax error: wrong argument count\n");
	else if (dir.path == NULL)
		printk("%s:/%s\n", dev_name(dir.dev),
		       getwd(dir.dev)->path);
	else
		chdir(dir);

	if (dir.path)
		free(dir.path);
}

static void ls_parse()
{
	path_t dir = path_parse(PATH_COMPLETE_DEV | PATH_COMPLETE_PATH);

	if (__PULL())
		printk("Syntax error: wrong argument count\n");
	else
		ls_cmd(NULL, dev_name(dir.dev), dir.path);

	free(dir.path);
}

static void boot_parse()
{
	path_t image = path_parse(PATH_COMPLETE_DEV);
	char *token, *bootstring, *devname;

	/* Determine bootstring */
	if (!(token = __PULL()))
		bootstring = getenv("BOOT_STRING");
	else {
		char *tmp = bootstring = scratch;
		memset(bootstring, 0, 256);

		do
			tmp = stpcpy(stpcpy(tmp, token), " ");
		while ((token = __PULL()));
	}

	/* Strip leading and trailing quotes from the boot string */
	if (bootstring != NULL) {
		if (strlen(bootstring) != 0) {
			if (bootstring[0] == '"')
				bootstring[0] = ' ';
			if (bootstring[strlen(bootstring) - 1] == '"')
				bootstring[strlen(bootstring) - 1] = ' ';
		}
	}

	/* If path is empty, boot via config */
	devname = dev_name(image.dev);

	if (image.path == NULL)
		boot_via_config(NULL, devname, bootstring);
	else {
		boot_cmd(NULL, devname, image.path, bootstring);
		free(image.path);
	}
}


static void run_parse()
{
	path_t prog = path_parse(0);

	if (prog.dev == 0 || prog.path == NULL || __PULL())
		printk("Syntax error: wrong argument count\n");
	else
		run_cmd(NULL, dev_name(prog.dev), prog.path);

	if (prog.path)
		free(prog.path);
}

static void read_parse(void)
{
	char *dev = __PULL(), *block = __PULL();
	int blknum = 0;

	if (dev == NULL || __PULL()) {
		printk("Syntax error: wrong argument count\n");
		return;
	}

	if (block != NULL)
		dev[strlen(dev) - 1] = '\0';
	else {
		block = dev;
		dev = dev_name(curdev);
	}

	blknum = atoi(block);

	if (blknum < 0)
		printk("Invalid argument: %s\n", block);
	else
		read_cmd(dev, blknum);
}

#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
static void bootopt_parse(void)
{
	unsigned int opt = 0;
	char *token;

	token = __PULL();
	if (token != NULL)
		opt = atoi(token);

	if (opt < 0 || opt > 255)
		printk("Invalid argument: %s\n", token);
	else {
		int ch;

		outb(0x3f, 0x70);
		printk
		    ("\nWarning: Changing the bootoption value may render your system\n"
		     "       unusable.  If you boot the miniloader, be sure to connect\n"
		     "       a terminal to /dev/ttyS0 at 9600bps.\n\n"
		     "Current setting: %d\n" "Changing to %d, ok (y/n)? ",
		     inb(0x71), opt);
		ch = kbd_getc();
		printk("%c\n", ch);
		if (ch == 'y') {
			outb(0x3f, 0x70);
			outb(opt, 0x71);
			outb(0x3f, 0x70);
			printk("New setting: %d\n", opt);
		}
	}
}
#endif

#if defined (MINI_NVRAM)
static void resetenv_parse(void)
{
	if (__PULL())
		printk("Syntax error: wrong argument count\n");
	else
		resetenv();
}
#endif

static void setenv_parse(void)
{
	extern void dev_config(void);
	char *name, *value;
	char buf[20];
	int i;

	name = __PULL();
	value = __PULL();

	if (name == NULL && value == NULL)
		printenv();
	else if (value == NULL)
		printk("Syntax error: wrong argument count\n");
	else {
/*
 *  Check for the special variable PCI_LATENCY.
 */
		if (strcmp(name, "PCI_LATENCY") == 0) {
			int latency = atoi(value);

			if (latency < 0 || latency > 255) {
				printk("Invalid argument: %s\n", value);
				return;
			}
			printk("Setting PCI bus master latency to %d.\n"
			       "Re-initializing PCI configuration...\n",
			       latency);
			default_PCI_master_latency = latency;
			dev_config();
		}

/*
 *  Check for the special variable MEMORY_SIZE.
 */
		if (strcmp(name, "MEMORY_SIZE") == 0) {
			int megs = atoi(value);
			int size;

			size = megs * 1024 * 1024;
			if (size < MILO_MIN_MEMORY_SIZE) {
				printk("Invalid argument: %s\n", value);
				return;
			}
			printk
			    ("Setting system memory size to %d megabytes.\n"
			     "Re-initializing memory configuration...\n",
			     megs);
			reset_mem(size);
		}

/*
 *  Check for the special variable BOOT_STRING.
 */
		if (strcmp(name, "BOOT_STRING") == 0) {
			char *bootstring = scratch;
			char *token;

			memset(scratch, 0, 256);
			token = value;
			while (token != NULL) {
				strcat(bootstring, token);
				strcat(bootstring, " ");
				token = __PULL();
			}
			value = bootstring;
		}

		/* now set the environment variable. */
		setenv(name, value);

/*
 *  Check for the special variables SCSIn_HOSTID.
 *  This has to be done *after* the variable has been set.
 */
		for (i = 0; i < 8; i++) {
			sprintf(buf, "SCSI%d_HOSTID", i);
			if (strcmp(buf, name) == 0) {
				dev_config();
				break;
			}
		}

/*
 *  Check for the special variable VERBOSE.
 */
		if (strcmp(name, "VERBOSE") == 0)
			milo_verbose = atoi(value);
	}
}

static void unsetenv_parse(void)
{
	char *token = __PULL();

	if (token == NULL || __PULL())
		printk("Syntax error: wrong argument count\n");
	else
		unsetenv(token);
}

#ifdef DEBUG_MILO
static void reboot_parse(void)
{
	halt();
}
#endif

static void show_parse(void)
{
	milo_whatami();
	if (milo_verbose)
#ifdef MINI_NVRAM
		printk("NVRAM is supported on this system\n");
#else
		printk("NVRAM is not supported on this system\n");
#endif
	show_devices();
	show_known_fs();
}

static void help_parse(void)
{
#define COMMAND_SIZE 22
	int i;
	int count;
	char *token;

	token = __PULL();
	if (token == NULL) {

		/* information about commands */

		printk("MILO command summary:\n\n");

		for (i = 0; i < NUM_MILO_COMMANDS; i++) {
			printk("%s %s ", commands[i].name,
			       commands[i].help_args);
			count =
			    strlen(commands[i].name) +
			    strlen(commands[i].help_args) + 2;
			if (count <= COMMAND_SIZE) {
				count = COMMAND_SIZE - count;
			} else {
				printk("\n");
				count = COMMAND_SIZE;
			}
			while (count--)
				printk(" ");
			printk("- %s\n", commands[i].help_text);
		}

		/* information about variables */

		printk("\n"
#if !defined(MINI_NVRAM)
		       "Variables that MILO cares about:\n"
		       "MEMORY_SIZE           - System memory size in megabytes\n"
#if 0
		       "BOOT_DEV              - Specifies the default boot device\n"
		       "BOOT_FILE             - Specifies the default boot file\n"
#endif
		       "BOOT_STRING           - Specifies the boot string to pass to the kernel\n"
		       "SCSIn_HOSTID          - Specifies the host id of the n-th SCSI controller\n"
		       "PCI_LATENCY           - Specifies the PCI master device latency\n"
		       "CPU_CLOCK             - Specifies the CPU clock speed in MHz\n"
		       "VERBOSE               - Level of verbosity (0-1)\n\n"
#endif
		       "Paths are specified as [<dev>:]<path> with <dev> being e.g. sda1, sr0, ...\n"
		       "Add `| more' to page through output from commands.\n\n");

#if defined(MINI_NVRAM)
		printk
		    ("Type 'help env' for a list of environment variables.\n\n");
#endif
	} else {
#if defined(MINI_NVRAM)
		if (strcmp(token, "env")) {
			printk("MILO: unrecognised help subject\n");
			return;
		}

		printk("Environment variables that MILO cares about:\n");
		printk
		    ("  MEMORY_SIZE      - System memory size in megabytes\n");
		printk
		    ("  AUTOBOOT         - If set, MILO attempts to boot on powerup\n");
		printk
		    ("                     and enters command loop only on failure\n");
		printk
		    ("  AUTOBOOT_TIMEOUT - Seconds to wait before auto-booting on powerup\n");
		printk
		    ("  BOOT_DEV         - Specifies the default boot device\n");
		printk
		    ("  BOOT_FILE        - Specifies the default boot file\n");
		printk
		    ("  BOOT_STRING      - Specifies the boot string to pass to the kernel\n");
		printk
		    ("  SCSIn_HOSTID     - Specifies the host id of the n-th SCSI controller\n");
		printk
		    ("  PCI_LATENCY      - Specifies the PCI master device latency\n");
		printk
		    ("  CPU_CLOCK        - Specifies the CPU clock speed in MHz\n");
		printk
		    ("  VERBOSE          - Level of verbosity (0-1)\n\n");
#else
		printk("Syntax error: wrong argument count\n");
#endif				/* MINI_NVRAM */
	}
}


/*****************************************************************************
 *  MILO Command processing.                                                 *
 *****************************************************************************/
/*
 *  And what are little Milo's made of?
 */
static void milo_whatami(void)
{
	extern U64 milo_memory_size;

	printk("MILO (%s):\n", alpha_mv.vector_name);
	printk("    Built against Linux " UTS_RELEASE "\n");
	printk("    Using compiler " LINUX_COMPILER "\n");

#if 0
	/* what sort of video do we have? */
	printk("    Video is "
#if defined(MINI_DIGITAL_BIOSEMU)
	       "Digital BIOS Emulation (VGA)"
#elif defined(MINI_FREE_BIOSEMU)
	       "VGA (x86emu)"
#endif
#if (defined(MINI_DIGITAL_BIOSEMU)||defined(MINI_FREE_BIOSEMU)) && defined(MINI_TGA_CONSOLE)
	       ", "
#endif
#if defined (MINI_TGA_CONSOLE)
	       "Zlxp (TGA)"
#endif
	       "\n");
#endif

	/* what disk and floppy controllers do we know about? */
	printk("    Device drivers:\n"
#ifdef CONFIG_SCSI
	       "        SCSI ("
#ifdef CONFIG_SCSI_NCR53C7xx
	       "NCR (53c7,8xx), "
#endif
#ifdef CONFIG_SCSI_NCR53C8XX
	       "NCR (ncr53c8xx), "
#endif
#ifdef CONFIG_SCSI_SYM53C8XX
	       "Symbios Logic (sym53c8xx), "
#endif
#ifdef CONFIG_SCSI_QLOGIC_ISP
	       "Qlogic ISP, "
#endif
#ifdef CONFIG_SCSI_BUSLOGIC
	       "Buslogic, "
#endif
#ifdef CONFIG_SCSI_AIC7XXX
	       "AIC7XXX"
#endif
	       ")\n"
#endif
#ifdef CONFIG_BLK_DEV_IDE
	       "        IDE\n"
#endif
#ifdef CONFIG_BLK_DEV_FD
	       "        Floppy\n"
#endif
	    );
	/* how much memory do we think there is? */
	printk("    Memory size is %ld MBytes\n",
	       milo_memory_size / 0x100000);
}

static int partial_compare(char *token, char *command)
{
	int len;
	int i;

	if (strlen(command) < strlen(token))
		return FALSE;
	len = strlen(token);

	for (i = 0; i < len; i++)
		if (tolower(*token++) != tolower(*command++))
			return FALSE;

	return TRUE;
}

static int compare(char *token, char *command)
{
	int len;
	int i;

	if (strlen(command) != strlen(token))
		return FALSE;
	len = strlen(token);

	for (i = 0; i < len; i++)
		if (tolower(*token++) != tolower(*command++))
			return FALSE;

	return TRUE;
}

static void parse_command()
{
	char *token;
	int i;
	int found = 0;
	int index = -1;

	/* if last two tokens are "|" and "more", enable paging the output */
	if (ntokens > 2 && !strcmp(tokens[ntokens - 2], "|") &&
	    !strcmp(tokens[ntokens - 1], "more")) {
		ntokens -= 2;
		printk_rows = 1;
	} else
		printk_rows = 0;

	/* now work out what the command was from the first token */
	token = __PULL();
	if (token != NULL) {
		char *tmp = strchr(token, ':');

		/* first check if the user wants to change the current device */
		if (tmp != NULL && tmp[1] == '\0') {
			*tmp = '\0';
			chdev(device_name_to_number(token));
			return;
		}

		/* first, look for an absolute match */
		for (i = 0; i < NUM_MILO_COMMANDS; i++) {
			if (compare(token, commands[i].name) == TRUE) {
				(commands[i].action_rtn) ();
				return;
			}
		}

		/* now, look for a partial absolute match */
		for (i = 0; i < NUM_MILO_COMMANDS; i++) {
			if (partial_compare(token, commands[i].name) ==
			    TRUE) {
				found++;
				index = i;
			}
		}

		if (found == 0)
			printk("MILO: unknown command, try typing Help\n");
		else {
			if (found > 1)
				printk
				    ("MILO: more than one command matches, try typing Help\n");
			else
				(commands[index].action_rtn) ();
		}
	}
}

static void parse_command_string(char *string)
{
	char *token, *tmp;

	while (string) {
		/* ';' marks the end of a command */
		if ((tmp = strchr(string, ';')))
			*tmp++ = '\0';

		/* Parse a command into tokens */
		token = strtok(string, " \t");
		ntokens = 0;

		while (token) {
			__ADD(token);
			token = strtok(NULL, " \t");
		}

		/* Now that we've found it, parse it */
		parse_command();

		/* Go to next command */
		string = tmp;
	}
}

static int do_timeout(int timeout)
{
#define ASCII_ESCAPE	0x1b	/* ASCII code of ESC key */
	int key;

	if (timeout <= 0)
		return 0;

	printk
	    ("Hit any key for interactive mode, ESC to boot immediately.\n"
	     "Seconds remaining: ");
	while (timeout--) {
		printk("%4d\b\b\b\b", timeout);
		key = kbd_getc_with_timeout(1);
		if (key >= 0) {
			if (key == ASCII_ESCAPE) {
				break;
			} else {
				printk
				    ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
				     "Entering interactive mode.\n");
				return -1;	/* argh, user whimped out on us... */
			}
		}
	}
	printk("   0\n");
	return 0;		/* OK, go ahead and boot */
}

static inline void autoboot(void)
{
	char *default_bootdev;
	char *default_bootfile;
	char *default_bootstring;
	int autoboot_timeout = 0;

	if (!getenv("AUTOBOOT"))
		return;

	default_bootdev = getenv("BOOT_DEV");
	default_bootfile = getenv("BOOT_FILE");
	default_bootstring = getenv("BOOT_STRING");

	if (getenv("AUTOBOOT_TIMEOUT")) {
		autoboot_timeout = atoi(getenv("AUTOBOOT_TIMEOUT"));
	}

	if (default_bootdev && default_bootfile) {

		/* We've got something to try.  Do the timeout if necessary... */
		printk("MILO: About to boot %s:%s.\n", default_bootdev,
		       default_bootfile);
		if (autoboot_timeout && do_timeout(autoboot_timeout)) {
			return;
		}

		/* If boot_cmd returns, then the boot failed and we fall through to the
		 * command loop. */
		boot_cmd(NULL, default_bootdev, default_bootfile,
			 default_bootstring);
	}
}

static inline void reboot(void)
{
	char *bootdev;
	int reboot_timeout = 30;

	/* figure out the device name from the number */
	bootdev = dev_name(milo_reboot_block->boot_device);

	/* We've got something to try.  Do the timeout if necessary... */
	if (do_timeout(reboot_timeout) < 0) {
		return;
	}

	/* If boot_cmd returns, then the boot failed and we fall through to the
	 * command loop. */
	boot_cmd(fs_type_to_name(milo_reboot_block->boot_filesystem),
		 bootdev, milo_reboot_block->boot_filename,
		 milo_reboot_block->boot_string);
}

void milo(void)
{
	char *command_string;
/*
 * If AUTOBOOT is set and we have something to boot from, then do so here. 
 * If AUTOBOOT_TIMEOUT is set, give the user a chance to bail out first...
 */

	printk("%s\n", banner);
#ifdef AUTO_DEVICE_SETUP
	/* Set active device.  Devices are setup as a side effect. */
	chdev(device_name_to_number("disk"));
#else
	printk
	    ("Type <dev>: to set active device, with <dev> e.g. sda1.\n\n");
#endif
	if (milo_verbose)
		milo_whatami();

	/* allocate some memory for command parsing */
	command_string = (char *) kmalloc(2 * 256, 0);
	if (command_string == NULL) {
		error("Not enough memory");
	}

	scratch = command_string + 256;

#ifdef MINI_NVRAM
	/* if the master PCI latency has been set then retrieve its value and
	 *  reset our builtin default of 32 */
	if (getenv("PCI_LATENCY")) {
		int latency;

		latency = atoi(getenv("PCI_LATENCY"));
		if (latency < 0 || latency > 255) {
			printk("MILO: invalid PCI latency envvar\n");
		} else {
			default_PCI_master_latency = latency;
			dev_config();
		}
	}
#endif

	sti();

	calibrate_delay();

	/* Handle boot/reboot/autoboot */
	if (milo_reboot_block == NULL)
		autoboot();
	else {

		/* Are we rebooting? */
		if (milo_reboot_block->flags & REBOOT_REBOOT)
			reboot();
		else {

			/* We didn't reboot, are there any WNT ARC arguments that we should
			 * know about? */
			if (milo_reboot_block->flags & REBOOT_CONSOLE) {

				/* OSLOADOPTIONS has been saved in ZERO_PGE, go and parse it to
				 * see if it makes sense. */
				if (strncmp((char *) ZERO_PGE, "MILO", 4)
				    == 0) {
					char *ptr =
					    (char *) ZERO_PGE + 4, *cmd;
					static char osloadoptions[] =
					    "OSLOADOPTIONS";

					cmd = ptr + sizeof(osloadoptions);
					if (*cmd) {
						if (strncmp
						    (ptr, osloadoptions,
						     sizeof osloadoptions -
						     1)) {
							printk
							    ("%s (press any key to abort)\n",
							     ptr);
							if
							    (kbd_getc_with_timeout
							     (1) < 0)
								parse_command_string
								    (cmd);
						} else {
							if (milo_verbose)
								printk
								    ("%s\n",
								     ptr);
							parse_command_string
							    (cmd);
						}
					}
				}
			}
			autoboot();
		}
	}

	/* parse commands forever */
	while (1) {

		printk("MILO> ");

		kbd_gets(command_string, 256);
		printk_rows = 0;
		parse_command_string(command_string);
	}
}
