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
 * This is a set of functions that provides minimal iso9660
 * filesystem functionality to the Linux bootstrapper.
 */

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>

#include "milo.h"
#include "fs.h"
#include "isofs.h"

/* Milo file system support routines. */
static int iso9660_mount(long device, int quiet);
static int iso9660_bread(int fd, long blkno, char *buffer);
static int iso9660_open(char *filename);
static long iso9660_fdsize(int fd);
static void iso9660_ls(char *dirname, char *devname);
static void iso9660_close(int fd);

struct bootfs iso9660fs = {
	"iso9660",
	0,
	0,
	iso9660_mount,
	iso9660_open,
	iso9660_fdsize,
	iso9660_bread,
	iso9660_ls,
	iso9660_close,
};

#define MAX_OPEN_FILES 5

static struct inode_table_entry {
	struct iso_inode inode;
	int inumber;
	int free;
	unsigned short old_mode;
	int size;
	int nlink;
	int mode;
	void *start;
} inode_table[MAX_OPEN_FILES];

unsigned long root_inode = 0;

/* iso9660 support code */

static long cd_device;
static int fd = -1;
static struct isofs_super_block *sb = NULL;
static char *data_block;
static char *big_data_block;

int isonum_711(char *p)
{
	return (*p & 0xff);
}

int isonum_712(char *p)
{
	int val;

	val = *p;
	if (val & 0x80)
		val |= 0xffffff00;
	return (val);
}

int isonum_721(char *p)
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

int isonum_722(char *p)
{
	return (((p[0] & 0xff) << 8) | (p[1] & 0xff));
}

int isonum_723(char *p)
{
	return (isonum_721(p));
}

int isonum_731(char *p)
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

int isonum_732(char *p)
{
	return (((p[0] & 0xff) << 24)
		| ((p[1] & 0xff) << 16)
		| ((p[2] & 0xff) << 8)
		| (p[3] & 0xff));
}

int isonum_733(char *p)
{
	return (isonum_731(p));
}

static long iso9660_fdsize(int fd)
{
	return inode_table[fd].size;
}

static int isofs_bread(long blkno, char *buffer)
{
#ifdef DEBUG_ISOFS
	printk("ISOFS: isofs_bread: reading block %d\n", blkno);
#endif
	if (device_read
	    (fd, buffer, sb->s_blocksize,
	     (blkno * sb->s_blocksize)) != sb->s_blocksize) {
		printk("iso9660 (bread): read error on block %ld\n",
		       blkno);
		return -1;
	} else
		return 0;
}

/* Release our hold on an inode.  Since this is a read-only application,
 * don't worry about putting back any changes...
 */
static void isofs_iput(struct iso_inode *ip)
{
	struct inode_table_entry *itp;

	/* Find and free the inode table slot we used... */
	itp = (struct inode_table_entry *) ip;

	itp->inumber = 0;
	itp->free = 1;
}

/* Read the specified inode from the disk and return it to the user.
 * Returns NULL if the inode can't be read...
 */
static struct iso_inode *isofs_iget(int ino)
{
	int i;
	struct iso_inode *inode;
	struct inode_table_entry *itp;
	struct iso_directory_record *raw_inode;
	unsigned char *pnt = NULL;
	void *cpnt = NULL;
	int high_sierra;
	int block;

#ifdef DEBUG_ISOFS
	printk("isofs_iget(): called for ino = %d\n", ino);
#endif

	/* find a free inode to play with */
	inode = NULL;
	itp = NULL;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (inode_table[i].free) {
			itp = &(inode_table[i]);
			inode = &(itp->inode);
			break;
		}
	}
	if ((inode == NULL) || (itp == NULL)) {
		printk("iso9660 (iget): no free inodes\n");
		return (NULL);
	}

	block = ino >> sb->s_blocksize_bits;
#ifdef DEBUG_ISOFS
	printk("isofs_inode, reading ino %d, block %d at offset %d\n",
	       ino, block, sb->s_blocksize * ino);
#endif
	if (isofs_bread(block, data_block) < 0) {
		printk("iso9660: unable to read i-node block");
		return NULL;
	}

	pnt = ((unsigned char *) data_block
	       + (ino & (sb->s_blocksize - 1)));
	raw_inode = ((struct iso_directory_record *) pnt);
	high_sierra = sb->s_high_sierra;

	if ((ino & (sb->s_blocksize - 1)) + *pnt > sb->s_blocksize) {
		int frag1, offset;

		offset = (ino & (sb->s_blocksize - 1));
		frag1 = sb->s_blocksize - offset;
		cpnt = big_data_block;
		memcpy(cpnt, data_block + offset, frag1);
		if (isofs_bread(++block, data_block) < 0) {
			printk("unable to read i-node block");
			return NULL;
		}
		offset += *pnt - sb->s_blocksize;
		memcpy((char *) cpnt + frag1, data_block, offset);
		pnt = ((unsigned char *) cpnt);
		raw_inode = ((struct iso_directory_record *) pnt);
	}

	if (raw_inode->flags[-high_sierra] & 2) {
		itp->mode = S_IRUGO | S_IXUGO | S_IFDIR;
		itp->nlink = 1;	/* Set to 1.  We know there are 2, but
				   the find utility tries to optimize
				   if it is 2, and it screws up.  It is
				   easier to give 1 which tells find to
				   do it the hard way. */
	} else {
		itp->mode = sb->s_mode;	/* Everybody gets to read the file. */
		itp->nlink = 1;
		itp->mode |= S_IFREG;
		/* If there are no periods in the name, then set the execute permission bit */
		for (i = 0; i < raw_inode->name_len[0]; i++)
			if (raw_inode->name[i] == '.'
			    || raw_inode->name[i] == ';')
				break;
		if (i == raw_inode->name_len[0]
		    || raw_inode->name[i] == ';') itp->mode |= S_IXUGO;	/* execute permission */
	}

	itp->size = isonum_733(raw_inode->size);

	/* There are defective discs out there - we do this to protect
	   ourselves.  A cdrom will never contain more than 700Mb */
	if ((itp->size < 0 || itp->size > 700000000) && sb->s_cruft == 'n') {
		printk
		    ("Warning: defective cdrom.  Enabling \"cruft\" mount option.\n");
		sb->s_cruft = 'y';
	}

/* Some dipshit decided to store some other bit of information in the high
   byte of the file length.  Catch this and holler.  WARNING: this will make
   it impossible for a file to be > 16Mb on the CDROM!!!*/

	if (sb->s_cruft == 'y' && itp->size & 0xff000000) {
		/*          printk("Illegal format on cdrom.  Pester manufacturer.\n"); */
		itp->size &= 0x00ffffff;
	}

	if (raw_inode->interleave[0]) {
		printk("Interleaved files not (yet) supported.\n");
		itp->size = 0;
	}

	/* I have no idea what file_unit_size is used for, so
	   we will flag it for now */
	if (raw_inode->file_unit_size[0] != 0) {
		printk("File unit size != 0 for ISO file (%d).\n", ino);
	}

	/* I have no idea what other flag bits are used for, so
	   we will flag it for now */
#ifdef DEBUG_ISOFS
	if ((raw_inode->flags[-high_sierra] & ~2) != 0) {
		printk("Unusual flag settings for ISO file (%d %x).\n",
		       ino, raw_inode->flags[-high_sierra]);
	}
#endif

	inode->i_first_extent = (isonum_733(raw_inode->extent) +
				 isonum_711(raw_inode->ext_attr_length))
	    << sb->s_log_zone_size;

	inode->i_backlink = 0xffffffff;	/* Will be used for previous directory */
	switch (sb->s_conversion) {
	case 'a':
		inode->i_file_format = ISOFS_FILE_UNKNOWN;	/* File type */
		break;
	case 'b':
		inode->i_file_format = ISOFS_FILE_BINARY;	/* File type */
		break;
	case 't':
		inode->i_file_format = ISOFS_FILE_TEXT;	/* File type */
		break;
	case 'm':
		inode->i_file_format = ISOFS_FILE_TEXT_M;	/* File type */
		break;
	}

#if 0
/* Now test for possible Rock Ridge extensions which will override some of
   these numbers in the inode structure. */

	if (!high_sierra)
		parse_rock_ridge_inode(raw_inode, inode);
#endif

	/* keep our inode table correct */
	itp->free = 0;
	itp->inumber = ino;

	/* return a pointer to it */
	return inode;
}

/*
 * ok, we cannot use strncmp, as the name is not in our data space.
 * Thus we'll have to use isofs_match. No big problem. Match also makes
 * some sanity tests.
 *
 * NOTE! unlike strncmp, isofs_match returns 1 for success, 0 for failure.
 */
static int isofs_match(int len, const char *name, const char *compare,
		       int dlen)
{
	if (!compare)
		return 0;

#ifdef DEBUG_ISOFS
	printk("isofs_match: comparing %s with %s\n", name, compare);
#endif

	/* check special "." and ".." files */
	if (dlen == 1) {
		/* "." */
		if (compare[0] == 0) {
			if (!len)
				return 1;
			compare = ".";
		} else if (compare[0] == 1) {
			compare = "..";
			dlen = 2;
		}
	}
	if (dlen != len)
		return 0;
	return !memcmp(name, compare, len);
}

int isofs_bmap(struct iso_inode *inode, int block)
{

	if (block < 0) {
		printk("_isofs_bmap: block<0");
		return 0;
	}
	return (inode->i_first_extent >> sb->s_blocksize_bits) + block;
}

/*
 *	isofs_list_entry()
 *
 * lists an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as an inode number). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 */
static void isofs_list_entry(struct iso_inode *dir)
{
	unsigned long bufsize = sb->s_blocksize;
	unsigned char bufbits = sb->s_blocksize_bits;
	unsigned int block, i, f_pos, offset, inode_number;
	void *cpnt = NULL;
	unsigned int old_offset;
	unsigned int backlink;
	int dlen, rrflag;
	char *dpnt;
	struct iso_directory_record *de;
	char c;
	struct inode_table_entry *itp = (struct inode_table_entry *) dir;

	if (!dir)
		return;

	if (!(block = dir->i_first_extent))
		return;

	f_pos = 0;
	offset = f_pos & (bufsize - 1);
	block = isofs_bmap(dir, f_pos >> bufbits);
	if (!block)
		return;

	if (isofs_bread(block, data_block) < 0)
		return;

	while (f_pos < itp->size) {
		de = (struct iso_directory_record *) (data_block + offset);
		backlink = itp->inumber;
		inode_number =
		    (block << bufbits) + (offset & (bufsize - 1));

		/* If byte is zero, this is the end of file, or time to move to
		   the next sector. Usually 2048 byte boundaries. */

		if (*((unsigned char *) de) == 0) {
			offset = 0;
			f_pos = ((f_pos & ~(ISOFS_BLOCK_SIZE - 1))
				 + ISOFS_BLOCK_SIZE);
			block = isofs_bmap(dir, f_pos >> bufbits);
			if (!block)
				return;
			if (isofs_bread(block, data_block) < 0)
				return;
			continue;	/* Will kick out if past end of directory */
		}

		old_offset = offset;
		offset += *((unsigned char *) de);
		f_pos += *((unsigned char *) de);

		/* Handle case where the directory entry spans two blocks.
		   Usually 1024 byte boundaries */
		if (offset >= bufsize) {
			unsigned int frag1;
			frag1 = bufsize - old_offset;
			cpnt = big_data_block;
			memcpy(cpnt, data_block + old_offset, frag1);

			de = (struct iso_directory_record *) cpnt;
			offset = f_pos & (bufsize - 1);
			block = isofs_bmap(dir, f_pos >> bufbits);
			if (!block)
				return;
			if (isofs_bread(block, data_block) < 0);
			memcpy((char *) cpnt + frag1, data_block, offset);
		}

		/* Handle the '.' case */

		if (de->name[0] == 0 && de->name_len[0] == 1) {
			inode_number = itp->inumber;
			backlink = 0;
		}

		/* Handle the '..' case */

		if (de->name[0] == 1 && de->name_len[0] == 1) {

#ifdef DEBUG_ISOFS
			printk("Doing .. (%d %d)",
			       sb->s_firstdatazone, itp->i_inumber);
#endif
			if ((sb->s_firstdatazone) != itp->inumber)
				inode_number = dir->i_backlink;
			else
				inode_number = itp->inumber;
			backlink = 0;
		}

		dlen = de->name_len[0];
		dpnt = de->name;
		/* Now convert the filename in the buffer to lower case */
#if 0
		rrflag = get_rock_ridge_filename(de, &dpnt, &dlen, dir);
#else
		rrflag = 0;
#endif
		if (rrflag) {
			if (rrflag == -1)
				return;	/* Relocated deep directory */
		} else {
			if (sb->s_mapping == 'n') {
				for (i = 0; i < dlen; i++) {
					c = dpnt[i];
					if (c >= 'A' && c <= 'Z')
						c |= 0x20;	/* lower case */
					if (c == ';' && i == dlen - 2
					    && dpnt[i + 1] == '1') {
						dlen -= 2;
						break;
					}
					if (c == ';')
						c = '.';
					de->name[i] = c;
				}
			}
		}
		/*
		 * Skip hidden or associated files unless unhide is set 
		 */
		if (!(de->flags[-sb->s_high_sierra] & 5)
		    || sb->s_unhide == 'y') {
			if (dlen == 1) {
				if (*dpnt == 0) {
					printk(".");
					dlen = 0;
				}
				if (*dpnt == 1) {
					printk("..");
					dlen = 0;
				}
			}
			while (dlen--)
				printk("%c", *dpnt++);
			/* is it a directory? */
			if (de->flags[-sb->s_high_sierra] & 2)
				printk("/");
			printk("\n");
		}
		if (cpnt)
			cpnt = NULL;

	}
}

/*
 *	isofs_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as an inode number). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 */
static int isofs_find_entry
    (
     struct iso_inode *dir,
     const char *name,
     int namelen, unsigned long *ino, unsigned long *ino_back) {
	unsigned long bufsize = sb->s_blocksize;
	unsigned char bufbits = sb->s_blocksize_bits;
	unsigned int block, i, f_pos, offset, inode_number;
	void *cpnt = NULL;
	unsigned int old_offset;
	unsigned int backlink;
	int dlen, rrflag, match;
	char *dpnt;
	struct iso_directory_record *de;
	char c;
	struct inode_table_entry *itp = (struct inode_table_entry *) dir;

	*ino = 0;
	if (!dir)
		return -1;

	if (!(block = dir->i_first_extent))
		return -1;

	f_pos = 0;
	offset = f_pos & (bufsize - 1);
	block = isofs_bmap(dir, f_pos >> bufbits);
	if (!block)
		return -1;

	if (isofs_bread(block, data_block) < 0)
		return -1;

	while (f_pos < itp->size) {
		de = (struct iso_directory_record *) (data_block + offset);
		backlink = itp->inumber;
		inode_number =
		    (block << bufbits) + (offset & (bufsize - 1));

		/* If byte is zero, this is the end of file, or time to move to
		   the next sector. Usually 2048 byte boundaries. */

		if (*((unsigned char *) de) == 0) {
			offset = 0;
			f_pos = ((f_pos & ~(ISOFS_BLOCK_SIZE - 1))
				 + ISOFS_BLOCK_SIZE);
			block = isofs_bmap(dir, f_pos >> bufbits);
			if (!block)
				return -1;
			if (isofs_bread(block, data_block) < 0)
				return -1;
			continue;	/* Will kick out if past end of directory */
		}

		old_offset = offset;
		offset += *((unsigned char *) de);
		f_pos += *((unsigned char *) de);

		/* Handle case where the directory entry spans two blocks.
		   Usually 1024 byte boundaries */
		if (offset >= bufsize) {
			unsigned int frag1;
			frag1 = bufsize - old_offset;
			cpnt = big_data_block;
			memcpy(cpnt, data_block + old_offset, frag1);

			de = (struct iso_directory_record *) cpnt;
			offset = f_pos & (bufsize - 1);
			block = isofs_bmap(dir, f_pos >> bufbits);
			if (!block)
				return -1;
			if (isofs_bread(block, data_block) < 0)
				return 0;
			memcpy((char *) cpnt + frag1, data_block, offset);
		}

		/* Handle the '.' case */

		if (de->name[0] == 0 && de->name_len[0] == 1) {
			inode_number = itp->inumber;
			backlink = 0;
		}

		/* Handle the '..' case */

		if (de->name[0] == 1 && de->name_len[0] == 1) {

#ifdef DEBUG_ISOFS
			printk("Doing .. (%d %d)",
			       sb->s_firstdatazone, itp->i_inumber);
#endif
			if ((sb->s_firstdatazone) != itp->inumber)
				inode_number = dir->i_backlink;
			else
				inode_number = itp->inumber;
			backlink = 0;
		}

		dlen = de->name_len[0];
		dpnt = de->name;
		/* Now convert the filename in the buffer to lower case */
#if 0
		rrflag = get_rock_ridge_filename(de, &dpnt, &dlen, dir);
#else
		rrflag = 0;
#endif
		if (rrflag) {
			if (rrflag == -1)
				return -1;	/* Relocated deep directory */
		} else {
			if (sb->s_mapping == 'n') {
				for (i = 0; i < dlen; i++) {
					c = dpnt[i];
					if (c >= 'A' && c <= 'Z')
						c |= 0x20;	/* lower case */
					if (c == ';' && i == dlen - 2
					    && dpnt[i + 1] == '1') {
						dlen -= 2;
						break;
					}
					if (c == ';')
						c = '.';
					de->name[i] = c;
				}
				/* This allows us to match with and without a trailing
				   period.  */
				if (dpnt[dlen - 1] == '.'
				    && namelen == dlen - 1)
					dlen--;
			}
		}
		/*
		 * Skip hidden or associated files unless unhide is set 
		 */
		match = 0;
		if (!(de->flags[-sb->s_high_sierra] & 5)
		    || sb->s_unhide == 'y') {
			match = isofs_match(namelen, name, dpnt, dlen);
		}

		if (cpnt)
			cpnt = NULL;

#if 0
		if (rrflag)
			kfree(dpnt);
#endif
		if (match) {
			if (inode_number == -1) {
#if 0
				/* Should only happen for the '..' entry */
				inode_number =
				    isofs_lookup_grandparent(dir,
							     find_rock_ridge_relocation
							     (de, dir));
				if (inode_number == -1) {
					/* Should never happen */
					printk
					    ("Backlink not properly set.\n");
					return -1;
				}
#else
				printk
				    ("iso9660: error inode_number = -1\n");
				return -1;
			}
#endif
			*ino = inode_number;
			*ino_back = backlink;
#ifdef DEBUG_ISOFS
			printk
			    ("isofs: find_entry returning successfully (ino = %d)\n",
			     inode_number);
#endif
			return 0;
		}
	}
	return -1;
}

/*
 *  Look up name in the current directory and return its corresponding
 *  inode if it can be found.
 */
struct iso_inode *isofs_lookup(struct iso_inode *dir, const char *name)
{
	struct inode_table_entry *itp = (struct inode_table_entry *) dir;
	unsigned long ino, ino_back;
	struct iso_inode *result = NULL;
	int first, last;

#ifdef DEBUG_ISOFS
	printk("isofs_lookup: %s\n", name);
#endif

	/* is the current inode a directory? */
	if (!S_ISDIR(itp->mode)) {
#ifdef DEBUG_ISOFS
		printk("isofs_lookup: inode %d not a directory\n",
		       itp->inumber);
#endif
		isofs_iput(dir);
		return NULL;
	}

	/* work through the name finding each directory in turn */
	ino = 0;
	first = last = 0;
	while (last < strlen(name)) {
		if (name[last] == '/') {
			if (isofs_find_entry
			    (dir, &name[first], last - first, &ino,
			     &ino_back)) return NULL;
			/* throw away the old directory inode, we don't need it anymore */
			isofs_iput(dir);
			if (!(dir = isofs_iget(ino)))
				return NULL;
			first = last + 1;
			last = first;
		} else
			last++;
	}
	if (isofs_find_entry
	    (dir, &name[first], last - first, &ino, &ino_back)) {
		isofs_iput(dir);
		return NULL;
	}
	if (!(result = isofs_iget(ino))) {
		isofs_iput(dir);
		return NULL;
	}

	/* We need this backlink for the ".." entry unless the name that we
	   are looking up traversed a mount point (in which case the inode
	   may not even be on an iso9660 filesystem, and writing to
	   u.isofs_i would only cause memory corruption).
	 */

	result->i_backlink = ino_back;

	isofs_iput(dir);
	return result;
}

/*
 * look if the driver can tell the multi session redirection value
 */
static unsigned int isofs_get_last_session(void)
{
	unsigned int vol_desc_start;

#if DEBUG_ISOFS
	printk("iso9660: isofs_get_last_session() called\n");
#endif

	vol_desc_start = 0;
	return vol_desc_start;
}

int isofs_read_super(struct isofs_super_block *s, void *data, int silent)
{
	extern void set_blocksize(kdev_t dev, int size);
	int iso_blknum;
	unsigned int blocksize_bits;
	int high_sierra;
	unsigned int vol_desc_start;
	char rock = 'y';

	struct iso_volume_descriptor *vdp;
	struct hs_volume_descriptor *hdp;

	struct iso_primary_descriptor *pri = NULL;
	struct hs_primary_descriptor *h_pri = NULL;

	struct iso_directory_record *rootp;

#if DEBUG_ISOFS
	printk("iso9660: isofs_read_super() called\n");
#endif

	/* set up the block size */
	blocksize_bits = 0;
	{
		int i = 1024;
		while (i != 1) {
			blocksize_bits++;
			i >>= 1;
		};
	};
	set_blocksize(cd_device, 1024);
	s->s_blocksize = 1024;
	s->s_blocksize_bits = blocksize_bits;

	s->s_high_sierra = high_sierra = 0;	/* default is iso9660 */

	vol_desc_start = isofs_get_last_session();

	for (iso_blknum = vol_desc_start + 16;
	     iso_blknum < vol_desc_start + 100; iso_blknum++) {
#ifdef DEBUG_ISOFS
		printk("isofs.inode: iso_blknum=%d\n", iso_blknum);
#endif
		if (isofs_bread
		    (iso_blknum << (11 - blocksize_bits), data_block) < 0) {
			printk("isofs_read_super: bread failed, dev "
			       "iso_blknum %d\n", iso_blknum);
			return -1;
		}
#ifdef DEBUG_ISOFS
		printk("iso9660: isofs_read_super(), read blocks\n");
#endif

		vdp = (struct iso_volume_descriptor *) data_block;
		hdp = (struct hs_volume_descriptor *) data_block;


		if (strncmp(hdp->id, HS_STANDARD_ID, sizeof hdp->id) == 0) {
			if (isonum_711(hdp->type) != ISO_VD_PRIMARY)
				return -1;
			if (isonum_711(hdp->type) == ISO_VD_END)
				return -1;

			s->s_high_sierra = 1;
			high_sierra = 1;
			rock = 'n';
			h_pri = (struct hs_primary_descriptor *) vdp;
			break;
		}

		if (strncmp(vdp->id, ISO_STANDARD_ID, sizeof vdp->id) == 0) {
			if (isonum_711(vdp->type) != ISO_VD_PRIMARY)
				return -1;
			if (isonum_711(vdp->type) == ISO_VD_END)
				return -1;

			pri = (struct iso_primary_descriptor *) vdp;
			break;
		}

	}			/* end of read volume descriptors */

	if (iso_blknum == vol_desc_start + 100) {
		if (!silent)
			printk
			    ("ISOFS: Unable to identify CD-ROM format.\n");
		return -1;
	}


	if (high_sierra) {
		rootp =
		    (struct iso_directory_record *) h_pri->
		    root_directory_record;
		if (isonum_723(h_pri->volume_set_size) != 1) {
			printk
			    ("Multi-volume disks not (yet) supported.\n");
			return -1;
		};
		s->s_nzones = isonum_733(h_pri->volume_space_size);
		s->s_log_zone_size = isonum_723(h_pri->logical_block_size);
		s->s_max_size = isonum_733(h_pri->volume_space_size);
	} else {
		rootp =
		    (struct iso_directory_record *) pri->
		    root_directory_record;
		if (isonum_723(pri->volume_set_size) != 1) {
			printk
			    ("Multi-volume disks not (yet) supported.\n");
			return -1;
		};
		s->s_nzones = isonum_733(pri->volume_space_size);
		s->s_log_zone_size = isonum_723(pri->logical_block_size);
		s->s_max_size = isonum_733(pri->volume_space_size);
	}

	s->s_ninodes = 0;	/* No way to figure this out easily */

	/* RDE: convert log zone size to bit shift */

	switch (s->s_log_zone_size) {
	case 512:
		s->s_log_zone_size = 9;
		break;
	case 1024:
		s->s_log_zone_size = 10;
		break;
	case 2048:
		s->s_log_zone_size = 11;
		break;

	default:
		printk("Bad logical zone size %ld\n", s->s_log_zone_size);
		return -1;
	}

	/* RDE: data zone now byte offset! */

	s->s_firstdatazone = (isonum_733(rootp->extent)
			      << s->s_log_zone_size);
	/* The CDROM is read-only, has no nodes (devices) on it, and since
	   all of the files appear to be owned by root, we really do not want
	   to allow suid.  (suid or devices will not show up unless we have
	   Rock Ridge extensions) */

	printk("Max size:%ld   Log zone size:%ld\n",
	       s->s_max_size, 1UL << s->s_log_zone_size);
	printk("First datazone:%ld   Root inode number %d\n",
	       s->s_firstdatazone >> s->s_log_zone_size,
	       isonum_733(rootp->extent) << s->s_log_zone_size);
	if (high_sierra)
		printk("Disc in High Sierra format.\n");
	/* set up enough so that it can read an inode */

	s->s_mapping = 'n';
	s->s_rock = (rock == 'y' ? 1 : 0);
	s->s_conversion = 'b';
	s->s_cruft = 'n';
	s->s_unhide = 'n';
	/*
	 * It would be incredibly stupid to allow people to mark every file 
	 * on the disk as suid, so we merely allow them to set the default 
	 * permissions.
	 */
	s->s_mode = S_IRUGO & 0777;
	s->s_blocksize = 1024;
	s->s_blocksize_bits = blocksize_bits;

	/* return successfully */
	root_inode = isonum_733(rootp->extent) << s->s_log_zone_size;
	return 0;

}

/* ---------------------------------------------------------------------- */
/* Milo accessable iso9660 support routines.                              */
/* ---------------------------------------------------------------------- */

static int iso9660_mount(long device, int quiet)
{
	int status = 0;
	int i;

#if DEBUG_ISOFS
	printk("iso9660: iso9660_mount() called\n");
#endif

	/* Initialize the inode table */
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		inode_table[i].free = 1;
		inode_table[i].inumber = 0;
	}

	/* allocate some buffers */
	data_block = (char *) kmalloc(1024, 0);
	if (data_block == NULL) {
		printk("isofs: failed to allocate data block\n");
		return -1;
	}
#ifdef DEBUG_ISOFS
	printk("iso9660: allocated data block\n");
#endif

	big_data_block = (char *) kmalloc(1024 * 2, 2);
	if (big_data_block == NULL) {
		printk("isofs: failed to allocate big data block\n");
		return -1;
	}
#ifdef DEBUG_ISOFS
	printk("iso9660: allocated big data block\n");
#endif

	if (sb == NULL) {
		sb = (struct isofs_super_block *)
		    kmalloc(sizeof(struct isofs_super_block), 0);
		if (sb == NULL) {
			printk
			    ("iso9660: error, failed to allocate super block\n");
			return -1;
		}
	}

	cd_device = device;

	/* read the super block (this determines the file system type and
	   other important information) */
	status = isofs_read_super(sb, NULL, 1);

	return status;
}

static int iso9660_bread(int fd, long blkno, char *buffer)
{
	struct iso_inode *inode;
	unsigned int block;

	/* find the inode for this file */
	inode = &(inode_table[fd].inode);

	/* do some error checking */
	if (!inode)
		return -1;
	if (!(inode->i_first_extent))
		return -1;

	/* figure out which iso block number we're being asked for */
	block = isofs_bmap(inode, blkno);
	if (!block)
		return -1;

	/* now try and read it. */
	if (isofs_bread(block, buffer) < 0)
		return -1;

	return 0;
}

static int iso9660_open(char *filename)
{
	struct iso_inode *result;
	struct iso_inode *root;

	/* get the root directory */
	root = isofs_iget(root_inode);
	if (!root) {
		printk("iso9660: get root inode failed\n");
		return -1;
	}

	/* lookup the file */
	result = isofs_lookup(root, filename);
	if (result == NULL)
		return -1;
	else {
		struct inode_table_entry *itp =
		    (struct inode_table_entry *) result;
		return (itp - inode_table);
	}
}

static void iso9660_ls(char *dirname, char *devname)
{
	struct iso_inode *result;
	struct iso_inode *root;

	/* get the root directory */
	root = isofs_iget(root_inode);
	if (!root) {
		printk("iso9660: get root inode failed\n");
		return;
	}

	/* swallow the first character if it is '/' */
	if (strncmp(dirname, "/", 1) == 0)
		dirname++;

	/* lookup the directory */
	result = isofs_lookup(root, dirname);
	if (result == NULL)
		printk("ISOFS: error, no such file or directory\n");
	else {
		struct inode_table_entry *itp =
		    (struct inode_table_entry *) result;
#ifdef DEBUG_ISOFS
		printk("iso9660: found directory, inode @ 0x%x\n",
		       itp->inumber);
#endif
		printk("Directory of: %s %s\n", devname, dirname);
		if (S_ISDIR(itp->mode))
			isofs_list_entry(result);
		else
			printk("%s\n", dirname);
	}
}

static void iso9660_close(int fd)
{
	device_close(fd);
}

