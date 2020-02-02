/*
 *  milo/proc.c based on linux/fs/proc/array.c
 *
 *  This file is subject to the terms and conditions of the GNU General
 *  Public License.  See the file "COPYING" in the main directory of this
 *  archive for more details.
 *
 *  Stefan Reinauer, <stepan@suse.de>
 *
 *  Copyright (C) 1992  by Linus Torvalds
 *  based on ideas by Darren Senn
 */

#include <linux/config.h>   
#include <linux/proc_fs.h>
#include "milo.h"

struct proc_dir_entry *proc_scsi;
struct inode_operations proc_scsi_inode_operations;
struct inode_operations proc_array_inode_operations;

/*
 * This is the root "inode" in the /proc tree..
 */

struct proc_dir_entry proc_root = {
	PROC_ROOT_INO, 5, "/proc",
	S_IFDIR | S_IRUGO | S_IXUGO, 2, 0, 0,
	0, &proc_array_inode_operations,
	NULL, NULL,
	NULL,
	&proc_root, NULL
};


/* Dummies for fs/proc/generic.c */

void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
#ifdef DEBUG_PROC
	printk ("Warning: remove_proc_entry() called.\n");
#endif
}

struct proc_dir_entry *create_proc_entry(const char *name, mode_t mode,
                                         struct proc_dir_entry *parent)
{
#ifdef DEBUG_PROC
	printk ("Warning: create_proc_entry() called\n");
#endif
	return NULL;
}

/* other dummies */

int proc_register(struct proc_dir_entry *driver, struct proc_dir_entry *x)
{
#ifdef DEBUG_PROC
	printk ("Warning: proc_register() called\n");
#endif
	return 0;
}

int proc_unregister(struct proc_dir_entry *driver, int x)
{
#ifdef DEBUG_PROC
	printk ("Warning: proc_uregister() called\n");
#endif
	return 0;
}

void pci_proc_init(void)
{
}
