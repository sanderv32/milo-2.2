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
 *  Flash update utility
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
#include "impure.h"
#include "uart.h"
#include "flash.h"

extern flash_t Flash;
flash_t *flashdriver = &Flash;

#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
static void env_parse(void);
static void delete_parse(void);
static void bootopt_parse(void);
#endif
static void list_parse(void);
static void program_parse(void);
static void help_parse(void);
static void quit_parse(void);

unsigned int quit = FALSE;

static int tolower(int c)
{
	if ((c >= 'A') && (c <= 'Z'))
		return c - 'A' + 'a';
	return c;
}

int yes_no(char *string)
{
	char c;

	printk("%s", string);
	c = kbd_getc();
	printk("%c\n", c);
	if (tolower(c) == 'y')
		return TRUE;
	else
		return FALSE;
}

#if 0
static void stop(void)
{
	printk("Swapping back to Milo\n");
	halt();
}

#endif

static void parse_command_string(char *string);

struct command {
	char *name;
	char *help_args;
	char *help_text;
	void (*action_rtn) (void);
};

static struct command commands[] = {
	{"list",
	 "",
	 "List the contents of flash",
	 &list_parse},
	{"program",
	 "",
	 "program an image into flash",
	 &program_parse},
	{"quit",
	 "",
	 "Quit",
	 &quit_parse},
/*
 *  System specific commands
 */
#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
	{"delete",
	 "image wnt|milo | block block-number",
	 "Delete a flash image or a flash block",
	 &delete_parse},
	{"environment",
	 "block-number",
	 "Set which block should contain environment variables",
	 &env_parse},
	{"bootopt",
	 "num",
	 "Select firmware type to use on next power up",
	 &bootopt_parse},
#endif
/*
 *  Help
 */
	{"help",
	 "",
	 "Print this help text",
	 &help_parse},
};

#define NUM_FMU_COMMANDS (sizeof(commands) / sizeof(struct command))

#define MAX_TOKENS 10
char *tokens[10];
int ntokens = 0;

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
/*****************************************************************************
 *  Parsing action routines                                                  *
 *****************************************************************************/

static void list_parse(void)
{
	__flash_list();
}

static void quit_parse(void)
{
	halt();
}

static void program_parse(void)
{
	__flash_program();
}

#if defined(MINI_ALPHA_CABRIOLET) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB164) || defined(MINI_ALPHA_PC164)
static void delete_parse(void)
{
	char *token;
	int no;

	token = __PULL();
	if (token != NULL) {
		if (strcmp(token, "image") == 0) {
			token = __PULL();
			if (token == NULL) {
				printk
				    ("ERROR: supply an image to delete\n");
				return;
			}
			__flash_delete_image(token);
			return;
		}

		if (strcmp(token, "block") == 0) {
			token = __PULL();
			if (token == NULL) {
				printk
				    ("ERROR: supply a block number to delete\n");
				return;
			}
			no = atoi(token);
			__flash_delete_block(no);
			return;
		}
	}
	printk("ERROR: supply either block or image keyword\n");
}

static void env_parse(void)
{
	char *token;
	int blockno;

	token = __PULL();
	if (token == NULL) {
		printk("ERROR: supply a block number to use\n");
		return;
	}
	blockno = atoi(token);
	__flash_env(blockno);
}

static void bootopt_parse(void)
{
	unsigned int opt = 0;
	char *token;

	token = __PULL();
	if (token != NULL)
		opt = atoi(tokens[1]);

	if (opt > 255) {
		printk
		    ("%u is not a valid value for the bootopt argument %s\n",
		     opt, token);
	} else {
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

static void help_parse(void)
{
#define COMMAND_SIZE 20
	int i;
	int count;

	printk("FMU command summary:\n\n");

	for (i = 0; i < NUM_FMU_COMMANDS; i++) {
		printk("%s ", commands[i].name);
		count = strlen(commands[i].name);
		count++;
		printk("%s ", commands[i].help_args);
		count += strlen(commands[i].help_args);
		count++;
		if (count < COMMAND_SIZE) {
			count = COMMAND_SIZE - count;
		} else {
			printk("\n");
			count = COMMAND_SIZE;
		}
		while (count--)
			printk(" ");
		printk("- %s\n", commands[i].help_text);
	}
}


/*****************************************************************************
 *  FMU Command processing.                                                 *
 *****************************************************************************/

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

	/* now work out what the command was from the first token */
	token = __PULL();
	if (token != NULL) {

		/* first, look for an absolute match */
		for (i = 0; i < NUM_FMU_COMMANDS; i++) {
			if (compare(token, commands[i].name) == TRUE) {
				(commands[i].action_rtn) ();
				return;
			}
		}

		/* now, look for a partial absolute match */
		for (i = 0; i < NUM_FMU_COMMANDS; i++) {
			if (partial_compare(token, commands[i].name) ==
			    TRUE) {
				found++;
				index = i;
			}
		}
		if (found == 0)
			printk("FMU: unknown command, try typing Help\n");
		else {
			if (found > 1)
				printk
				    ("FMU: more than one command matches, try typing Help\n");
			else
				(commands[index].action_rtn) ();
		}
	}
}

static void parse_command_string(char *string)
{
	char *token;

	/* Grab all of the command tokens */
	ntokens = 0;
	token = strtok(string, " \t");
	while (token != NULL) {

		/* treat ';' as end of line, new command */
		if (strcmp(token, ";") == 0) {
			parse_command();
			ntokens = 0;
		} else {
			__ADD(token);
		}
		token = strtok(NULL, " \t");
	}

	/* now that we've found it, parse it. */
	parse_command();

}

void FMU(void)
{
	char command_string[256];

	printk("\nLinux Milo Flash Management Utility V1.0\n\n");
	if (__flash_init()) {
		while (1) {
			printk("FMU> ");
			kbd_gets(command_string, 256);
			parse_command_string(command_string);
			if (quit == TRUE)
				return;
		}
	} else {
		printk("ERROR: failed to initialise flash\n");
	}
}
