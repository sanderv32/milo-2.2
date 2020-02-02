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
 *  Milo: file system support
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
#include "fs.h"

#define NUM_MILO_FS (sizeof(bootfs) / sizeof(struct bootfs *))

extern struct bootfs ext2fs;
extern struct bootfs iso9660fs;
extern struct bootfs msdosfs;

static struct bootfs *bootfs[] = {
	&ext2fs,
	&iso9660fs,
	&msdosfs,
};

int nr_super_blocks = 0;
int max_super_blocks = NR_SUPER;

LIST_HEAD(super_blocks);

struct file_system_type *file_systems = (struct file_system_type *) NULL;
struct bootfs *milo_open_fs = NULL;

char *fs_type_to_name(int type)
{
	if (type >= 0 && type < sizeof(bootfs) / sizeof(struct bootfs))
		return bootfs[type]->name;
	else
		return NULL;
}

int fs_name_to_type(char *name)
{
	int i;

	if (name) {
		for (i = 0; i < NUM_MILO_FS; i++)
			if (strcmp(bootfs[i]->name, name) == 0)
				return i;
	}
	return -1;
}

int try_mount(char *fs_type, char *device, int quiet)
{
	int fs;

	if (device_open(device) < 0)
		return -1;

	fs = fs_name_to_type(fs_type);
	if (fs < 0) {
		printk("MILO: file system %s not supported\n", fs_type);
		return -1;
	}

	milo_open_fs = bootfs[fs];
	return (milo_open_fs->mount) ((device_curr_inode()->i_rdev),
				      quiet);
}

int fs_mount(char *fs_type, char *device)
{
	int rc, i;

	if (fs_type) {
		return try_mount(fs_type, device, 0);
	} else {
		for (i = 0; i < NUM_MILO_FS; i++) {
			rc = try_mount(bootfs[i]->name, device, 1);
			if (rc >= 0)
				return rc;
		}

		printk("MILO: unknown filesystem on device %s\n", device);

		return rc;
	}
}

void show_known_fs(void)
{
	int i;

	printk("File systems:\n    ");
	for (i = 0; i < NUM_MILO_FS; i++)
		printk("%s ", bootfs[i]->name);
	printk("\n");

}

/*
 * Find a super_block with no device assigned.
 */
static struct super_block *get_empty_super(void)
{
	struct super_block *s;

	for (s = sb_entry(super_blocks.next);
	     s != sb_entry(&super_blocks); s = sb_entry(s->s_list.next)) {
		if (s->s_dev)
			continue;
		if (!s->s_lock)
			return s;
		printk("VFS: empty superblock %p locked!\n", s);
	}
	/* Need a new one... */
	if (nr_super_blocks >= max_super_blocks)
		return NULL;
	s = kmalloc(sizeof(struct super_block), GFP_USER);
	if (s) {
		nr_super_blocks++;
		memset(s, 0, sizeof(struct super_block));
		INIT_LIST_HEAD(&s->s_dirty);
		list_add(&s->s_list, super_blocks.prev);
	}
	return s;
}


struct file_system_type *get_fs_type(const char *name)
{
	struct file_system_type *fs = file_systems;

	if (!name)
		return fs;
	for (fs = file_systems; fs && strcmp(fs->name, name);
	     fs = fs->next);
#ifdef CONFIG_KMOD
	if (!fs && (request_module(name) == 0)) {
		for (fs = file_systems; fs && strcmp(fs->name, name);
		     fs = fs->next);
	}
#endif

	return fs;
}


void wait_on_super(struct super_block *sb)
{
	struct wait_queue wait = { current, NULL };

	add_wait_queue(&sb->s_wait, &wait);
      repeat:
	current->state = TASK_UNINTERRUPTIBLE;
	if (sb->s_lock) {
		schedule();
		goto repeat;
	}
	remove_wait_queue(&sb->s_wait, &wait);
	current->state = TASK_RUNNING;
}


struct super_block *get_super(kdev_t dev)
{
	struct super_block *s;

	if (!dev)
		return NULL;
      restart:
	s = sb_entry(super_blocks.next);
	while (s != sb_entry(&super_blocks))
		if (s->s_dev == dev) {
			wait_on_super(s);
			if (s->s_dev == dev)
				return s;
			goto restart;
		} else
			s = sb_entry(s->s_list.next);
	return NULL;
}

struct super_block *read_super(kdev_t dev, const char *name, int flags,
			       void *data, int silent)
{
	struct super_block *s;
	struct file_system_type *type;

	if (!dev)
		goto out_null;
	check_disk_change(dev);
	s = get_super(dev);
	if (s)
		goto out;

	type = get_fs_type(name);
	if (!type) {
		printk("VFS: on device %s: get_fs_type(%s) failed\n",
		       kdevname(dev), name);
		goto out;
	}
	s = get_empty_super();
	if (!s)
		goto out;
	s->s_dev = dev;
	s->s_flags = flags;
	s->s_dirt = 0;
	/* N.B. Should lock superblock now ... */
	if (!type->read_super(s, data, silent))
		goto out_fail;
	s->s_dev = dev;		/* N.B. why do this again?? */
	s->s_rd_only = 0;
	s->s_type = type;
      out:
	return s;

	/* N.B. s_dev should be cleared in type->read_super */
      out_fail:
	s->s_dev = 0;
      out_null:
	s = NULL;
	goto out;
}
