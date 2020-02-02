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
/* 1999/04/30 Modified by Loic Prylli:
   - correct 16bit FAT handling, 12bit FAT was also wrong=>buffer overflows
   - rewrite directory handling to handle those larger than a block (16 entries)
   - bread should return 0 on success!!!
   - remove some pseudo-inode and memory leaks (but not all)
   - fix attribute parsing, long names are now ignored, and files without archive bit are not
   - fix sizing of the fat table
   - some sanity checks
*/

/* This is a set of functions that provides minimal msdos filesystem
 * functionality to the Linux bootstrapper.  All we can do is open
 * and read files... but that's all we need 8-)
 */

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>
#include <asm/unaligned.h>
#include "milo.h"
#include "fs.h"
#include "dos.h"

/* Milo file system support routines. */
static int msdos_mount(long device, int quiet);
static int msdos_bread(int fd, long blkno, char *buffer);
static int msdos_open(char *filename);
static long msdos_fdsize(int fd);
static void msdos_ls(char *dirname, char *devname);
static void msdos_close(int fd);

struct bootfs msdosfs = {
	"msdos",
	0,
	0,

	msdos_mount,
	msdos_open,
	msdos_fdsize,
	msdos_bread,
	msdos_ls,
	msdos_close,
};

/* fd is really an index into the inode table */

int fd;

/* the inode table contains local copies of directories on the last
   MSDOS file system that we read */
#define MAX_OPEN_FILES 5
static struct inode_table_entry {
	struct msdos_dir_entry entry;
	int offset;
	int free;
	short *block_list;
	short blocks;
} inode_table[MAX_OPEN_FILES];


/* keep useful information about the open filesystem */
static struct msdos_sb *sb = NULL;
static unsigned short int *fat = NULL;

/* ------------------------------------------------------------ */
/* INODE routines                                               */
/* ------------------------------------------------------------ */
static int allocate_inode(struct msdos_dir_entry *entry)
{
	int i;

	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (inode_table[i].free) {
			inode_table[i].free = FALSE;
			inode_table[i].blocks = 0;
			inode_table[i].block_list = NULL;
			memcpy(&inode_table[i].entry, entry,
			       sizeof(struct msdos_dir_entry));
			return i;
			break;
		}
	}
	printk("allocate_inode: no free inodes\n");
	return -1;
}

static void free_inode(int inode)
{
	inode_table[inode].free = TRUE;
	if (inode_table[inode].block_list != NULL) {
		free(inode_table[inode].block_list);
	}
}

static inline int msdos_build_root_inode(void)
{
	struct msdos_dir_entry root;
	int ino, i, nbblock;

	root.name[0] = 0;
	root.attr = ATTR_DIR;
	root.start = sb->dir_start;
	root.size = sb->dir_entries * sizeof(struct msdos_dir_entry);
	ino = allocate_inode(&root);
	nbblock = (root.size + sb->blocksize - 1) / sb->blocksize;
	inode_table[ino].block_list =
	    malloc(nbblock * sizeof(*inode_table[ino].block_list));
	if (!inode_table[ino].block_list)
		return -1;
	for (i = 0; i < nbblock; i++) {
		inode_table[ino].block_list[i] = sb->dir_start + i;
	}
	return ino;
}

/* ------------------------------------------------------------ */
/* FAT routines                                                 */
/* ------------------------------------------------------------ */
static inline unsigned short int read_fat(unsigned char *tfat, int offset)
{
	int index;
	int mask;
	unsigned short int value;

	/* use the File Allocation Table (FAT) to get at the appropriate
	   block of the file */
	if (sb->fat_bits == 16) {
		index = offset * 2;
		mask = 0xffff;
	} else {
		index = offset * 3 / 2;
		mask = 0xfff;
	}

#if 0
	printk("read_fat: offset = %d, index = %d\n", offset, index);
#endif

	value = get_unaligned((unsigned short *) &tfat[index]);
	if (sb->fat_bits == 12 && (offset & 0x1))
		value = value >> 4;
	value &= mask;
#if 0
	printk("read_fat: value = 0x%x (%d)\n", value, value);
#endif

	return value;
}

/* Read and convert the File Allocation Table */
static inline int translate_fat(void)
{

	void *buffer;
	int size_in_words;
	int bytes_read;
	int i;

	/* allocate a buffer to hold the real FAT */
	buffer = malloc(sb->blocksize * sb->fat_length);
	if (buffer == NULL) {
		printk("MSDOS, translate_fat failed to allocate buffer\n");
		return -1;
	}
	/* ...and read it */
	bytes_read = device_read(0, buffer,
				 sb->blocksize * sb->fat_length,
				 sb->fat_start * sb->blocksize);
	if (bytes_read != sb->blocksize * sb->fat_length) {
		printk("MSDOS: failed to read FAT\n");
		free(buffer);
		return -1;
	}

	/* now work out how big that fat table is in words (16 bits) */
	if (sb->fat_bits == 16)
		size_in_words = bytes_read / 2;
	else
		size_in_words = bytes_read * 2 / 3;	/* div by 1,5 */
	if (fat != NULL)
		free(fat);
	fat = malloc(size_in_words * sizeof(short));
	if (fat == NULL) {
		printk("MSDOS: error cannot allocate memory for FAT\n");
		free(buffer);
		return -1;
	}

	/* ...and translate it */
	for (i = 0; i < size_in_words; i++)
		fat[i] = read_fat((unsigned char *) buffer, i);

	/* free off the temporary buffer and return successfully */
	free(buffer);
	return 0;
}

/* ------------------------------------------------------------ */
/* DOS routines                                                 */
/* ------------------------------------------------------------ */
/* is this a valid directory entry? */
static inline int valid_entry(struct msdos_dir_entry *entry)
{
	if ((unsigned char) (entry->name[0]) == (unsigned char) 0x05)
		return 0;
	if (IS_FREE(entry->name))
		return 0;
	if (entry->attr & ATTR_VOLUME)
		return 0;
	return 1;
}

/* convert a dir number from FAT into a real block number on the device */
static inline int __convert_dir(int dir)
{
	return ((dir - 2) * sb->cluster_size) + sb->data_start;
}

/* convert block numbers from FAT into real block numbers on the device */
static inline int __convert_block_list(int inode)
{
	int i;

	if (sb->cluster_size > 1) {
		unsigned short *new_list =
		    (unsigned short *) malloc(sizeof(short) *
					      inode_table[inode].blocks *
					      sb->cluster_size);
		int j;

		if (new_list == NULL) {
			printk
			    ("ERROR, convert_block_list failed to allocate memory\n");
			return -1;
		}
		j = 0;
		for (i = 0; i < inode_table[inode].blocks; i++) {
			new_list[i * sb->cluster_size] =
			    ((inode_table[inode].block_list[i] - 2) *
			     sb->cluster_size) + sb->data_start;
			for (j = 1; j < sb->cluster_size; j++) {
				new_list[(i * sb->cluster_size) + j] =
				    new_list[(i * sb->cluster_size) +
					     (j - 1)] + 1;
			}
		}
		free(inode_table[inode].block_list);
		inode_table[inode].block_list = new_list;
		inode_table[inode].blocks =
		    (inode_table[inode].entry.size +
		     sb->blocksize) / sb->blocksize;
	} else {
		for (i = 0; i < inode_table[inode].blocks; i++)
			inode_table[inode].block_list[i] =
			    inode_table[inode].block_list[i] - 2 +
			    sb->data_start;
	}
	return 0;
}

/* Generate the list of blocks that make up this file if alloc == 1*/
/* just count them if alloc == 0 */
static int fill_block_list(int inode, int alloc)
{
	int index, block;
	int size_in_blocks;
	unsigned int eof_mask;

#ifdef DEBUG_MSDOS
	printk("fill_block_list called for inode %d\n", inode);
#endif

	/* free off the old block list and allocate a new one */
	if (inode_table[inode].block_list != NULL) {
		free(inode_table[inode].block_list);
	}

	if (alloc) {
		size_in_blocks =
		    (inode_table[inode].entry.size + sb->blocksize -
		     1) / sb->blocksize;
		inode_table[inode].block_list =
		    malloc(sizeof(short) * size_in_blocks);

		if (inode_table[inode].block_list == NULL) {
			printk
			    ("MSDOS, fill_block_list error, failed to allocate block list\n");
			return -1;
		}

		inode_table[inode].blocks = size_in_blocks;
	}

	/* read the blocks into the list (checking for the end of the file) */
	if (sb->fat_bits == 12)
		eof_mask = 0xff8;
	else
		eof_mask = 0xfff8;

	block = 0;
	index = inode_table[inode].entry.start;
	do {
		if (index - 2 >= sb->clusters || block >= sb->clusters) {
			printk
			    ("found invalid cluster num: invalid FAT??\n");
			return -1;
		}
		if (alloc)
			inode_table[inode].block_list[block] = index;
		block += 1;
		index = fat[index];
	}
	while ((index & eof_mask) != eof_mask);

	if (alloc) {
		inode_table[inode].blocks = block;
		/* now convert the blocks into real block numbers in the device. */
		return __convert_block_list(inode);
	} else
		return block;

}

/* allocate a buffer and copy directory entries */
static void *msdos_readdir(int ino)
{
	struct msdos_dir_entry *dir = &inode_table[ino].entry;
	int i, count;
	char *buf;

	/* compute the directory size by browsing the fat */
	if (!dir->size) {
		count = fill_block_list(ino, 0);
		if (count < 0)
			return 0;
		dir->size = count * sb->blocksize * sb->cluster_size;
		if (fill_block_list(ino, 1) < 0) {
			dir->size = 0;
			return 0;
		}
	}

	/* allocate some memory to play with */
	buf = malloc(dir->size);
	if (buf == NULL) {
		printk
		    ("ERROR: msdos_readdir, failed to allocate buffer\n");
		return 0;
	}
	memset(buf, 0, dir->size);

	/* and read in the directory entry from disk */
	for (i = 0; i < dir->size; i += sb->blocksize) {
		int length =
		    dir->size - i >
		    sb->blocksize ? sb->blocksize : dir->size - i;
		if (device_read
		    (fd, buf + i, length,
		     inode_table[ino].block_list[i / sb->blocksize] *
		     sb->blocksize) != length) {
			printk
			    ("ERROR: msdos_readdir, failed to read a buffer\n");
			return 0;
		}
	}
	return buf;
}


/* convert an 8.3 DOS file name into a Unix one */
static inline void __convert_name(char *array, char *name)
{
	int i;
	int suffix = 0;
	extern int tolower(int);

	/* is there a suffix? */
	for (i = 8; i < 11; i++) {
		if (array[i] != 0x20) {
			suffix = 1;
			break;
		}
	}

	/* first the main part */
	for (i = 0; i < 8; i++) {
		if (*array != 0x20) {
			*name++ = tolower(*array);
		}
		array++;
	}

	if (suffix) {
		*name++ = '.';

		/* now the suffix */
		for (i = 0; i < 3; i++) {
			if (*array != 0x20) {
				*name++ = tolower(*array);
			}
			array++;
		}
	}

	*name++ = '\0';
}

static int msdos_search_directory(int ino, char *entryname)
{
	unsigned char *buf;
	struct msdos_dir_entry *dir = &inode_table[ino].entry;
	int i, count;
	char name[16];

	/* we cannot search a file, only directories */
	if (!(dir->attr & ATTR_DIR))
		return -1;

	buf = msdos_readdir(ino);
	if (!buf)
		return -1;

	/* now search the directory */
	count = dir->size / sizeof(struct msdos_dir_entry);
	dir = (struct msdos_dir_entry *) buf;
	for (i = 0; i < count; i++) {
		if (valid_entry(dir)) {
			__convert_name(dir->name, name);
			/* does it match? */
			if (strcmp(entryname, name) == 0) {
				ino = allocate_inode(dir);
				if (ino >= 0) {
					if (dir->attr & ATTR_DIR) {
						/* translate the directory offset into a 
						   block offset */
						inode_table[ino].entry.
						    start = dir->start;
						if (inode_table[ino].entry.
						    size) {
							printk
							    ("directory has non-zero size!!\n");
							return -1;
						}

					}
				}
				free(buf);
				return ino;
			}
		}
		dir++;
	}

	/* we failed */
	free(buf);
	return -1;
}

static int msdos_find(char *entryname)
{
	int ino, newino;
	char namebuf[256];
	char *current;

	/* this might be an "ls" of the root */
	if (strlen(entryname) == 0)
		return msdos_build_root_inode();
	if (strcmp(entryname, "/") == 0)
		return msdos_build_root_inode();

	/* copy the name so I can play with it */
	strcpy(namebuf, entryname);

	/* look in the root directory */
	current = strtok(namebuf, "/");

	ino = msdos_build_root_inode();
	if (ino < 0)
		return ino;

	newino = msdos_search_directory(ino, current);
	free_inode(ino);
	if (newino < 0)
		return newino;
	ino = newino;

	current = strtok(NULL, "/");
	while (current) {
		newino = msdos_search_directory(ino, current);
		free_inode(ino);
		if (newino < 0)
			return -1;
		ino = newino;
		current = strtok(NULL, "/");
	}

	/* found it */
	return ino;
}

static inline void msdos_list_entry(struct msdos_dir_entry *entry)
{
	unsigned char name[16];

	if (valid_entry(entry)) {
#ifdef DEBUG_MSDOS
		printk("(0x%x/%d) ", entry->attr, entry->start);
#endif
		__convert_name(entry->name, name);
		if (!(entry->attr & ATTR_DIR))
			printk("%s: %d bytes", name, entry->size);
		else
			printk("%s/", name);
		printk("\n");
	}
}

static inline void msdos_list_dir(int ino)
{
	struct msdos_dir_entry *dir = &inode_table[ino].entry;
	struct msdos_dir_entry *entry;
	void *cluster;
	int i;

	/* dir->start = first cluster, dir->size = size in bytes */
	cluster = msdos_readdir(ino);
	if (!cluster)
		return;


	entry = (struct msdos_dir_entry *) cluster;
	for (i = 0; i < dir->size / sizeof(struct msdos_dir_entry); i++) {
		msdos_list_entry(entry);
		entry++;
	}

	free(cluster);
}

/* Read the super block of an MS-DOS FS. */
static int msdos_read_super(unsigned long dev, int quiet)
{
	struct msdos_boot_sector *b;
	int data_sectors, logical_sector_size, sector_mult, fat_clusters =
	    0;
	int bytes_read;
	int blksize = 512;
	int error;

	/* allocate the super block */
	if (sb == NULL) {
		sb = (struct msdos_sb *) malloc(sizeof(struct msdos_sb));
		if (sb == NULL) {
			printk("MSDOS: failed to allocate super block\n");
			return -1;
		}
	}

	/* allocate someplace to hold the boot sector */
	b = malloc(sizeof(struct msdos_boot_sector));
	if (b == NULL) {
		printk("MSDOS: failed to allocate boot sector buffer\n");
		return -1;
	}

	blksize = 512;

	/* read the boot sector */
	bytes_read = device_read(0, (void *) b,
				 sizeof(struct msdos_boot_sector), 0);
	if (bytes_read != sizeof(struct msdos_boot_sector)) {
		printk("MSDOS: failed to read msdos boot sector\n");
		free(b);
		return -1;
	}
#ifdef DEBUG_MSDOS
	printk("MSDOS: boot sector read into memory at 0x%p\n",
	       (void *) b);
#endif

/*
 * The DOS3 partition size limit is *not* 32M as many people think.  
 * Instead, it is 64K sectors (with the usual sector size being
 * 512 bytes, leading to a 32M limit).
 * 
 * DOS 3 partition managers got around this problem by faking a 
 * larger sector size, ie treating multiple physical sectors as 
 * a single logical sector.
 * 
 * We can accommodate this scheme by adjusting our cluster size,
 * fat_start, and data_start by an appropriate value.
 *
 * (by Drew Eckhardt)
 */

#define ROUND_TO_MULTIPLE(n,m) ((n) && (m) ? (n)+(m)-1-((n)-1)%(m) : 0)
	/* don't divide by zero */

	logical_sector_size =
	    CF_LE_W(get_unaligned((unsigned short *) &b->sector_size));
	sector_mult = logical_sector_size >> SECTOR_BITS;
	sb->cluster_size = b->cluster_size * sector_mult;
	sb->fats = b->fats;
	sb->fat_start = CF_LE_W(b->reserved) * sector_mult;
	sb->fat_length = CF_LE_W(b->fat_length) * sector_mult;
	sb->dir_start =
	    (CF_LE_W(b->reserved) +
	     b->fats * CF_LE_W(b->fat_length)) * sector_mult;
	sb->dir_entries =
	    CF_LE_W(get_unaligned(((unsigned short *) &b->dir_entries)));
	sb->data_start =
	    sb->dir_start +
	    ROUND_TO_MULTIPLE((sb->dir_entries << MSDOS_DIR_BITS) >>
			      SECTOR_BITS, sector_mult);
	data_sectors =
	    (CF_LE_W(get_unaligned(((unsigned short *) &b->sectors))) ?
	     CF_LE_W(get_unaligned(((unsigned short *) &b->sectors))) :
	     CF_LE_L(b->total_sect)) * sector_mult - sb->data_start;
	error = !b->cluster_size || !sector_mult;
	if (!error) {
		sb->clusters = b->cluster_size ? data_sectors /
		    b->cluster_size / sector_mult : 0;
		sb->fat_bits = sb->clusters > MSDOS_FAT12 ? 16 : 12;
		fat_clusters =
		    sb->fat_length * SECTOR_SIZE * 8 / sb->fat_bits;
		error = !sb->fats || (sb->dir_entries & (MSDOS_DPS - 1))
		    || sb->clusters + 2 > fat_clusters + MSDOS_MAX_EXTRA
		    || (logical_sector_size & (SECTOR_SIZE - 1))
		    || !b->secs_track || !b->heads;
	}
	/*
	   This must be done after the brelse because the bh is a dummy
	   allocated by msdos_bread (see buffer.c)
	 */
	sb->blocksize = blksize;	/* Using this small block size solve the */
	/* the misfit with buffer cache and cluster */
	/* because cluster (DOS) are often aligned */
	/* on odd sector */
	sb->blocksize_bits = blksize == 512 ? 9 : 10;
	if (1) {		/* always print */
		/* The MSDOS_CAN_BMAP is obsolete, but left just to remember */
		printk("[MS-DOS FS Rel. 12,FAT]\n");
#ifdef DEBUG_MSDOS
		printk
		    ("[me=0x%x,cs=%d,#f=%d,fs=%d,fl=%d,ds=%d,de=%d,data=%d,"
		     "se=%d,ts=%ld,ls=%d]\n", b->media, sb->cluster_size,
		     sb->fats, sb->fat_start, sb->fat_length,
		     sb->dir_start, sb->dir_entries, sb->data_start,
		     CF_LE_W(get_unaligned
			     ((unsigned short *) &b->sectors)),
		     (unsigned long) b->total_sect, logical_sector_size);
		printk("Transaction block size = %d\n", blksize);
#endif
	}
	if (sb->clusters + 2 > fat_clusters)
		sb->clusters = fat_clusters - 2;

	if (error) {
		if (!quiet) {
			printk
			    ("VFS: Can't find a valid MSDOS filesystem on dev "
			     "%s.\n", kdevname(dev));
		}
		free(b);
		return -1;
	}

	/* read in the FAT */
	if (translate_fat() < 0) {
		free(b);
		free(sb);
		sb = NULL;
		return -1;
	}

	free(b);
	return 0;
}


/* ------------------------------------------------------------ */
/* Externally visible MSDOS routines                            */
/* ------------------------------------------------------------ */
static int msdos_mount(long device, int quiet)
{
	int i;

#ifdef DEBUG_MSDOS
	printk("msdos: msdos_mount() called\n");
#endif

	/* Initialize the inode table */
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		inode_table[i].free = 1;
	}

	/* read the MSDOS boot block and set up the super
	   block information. */
	if (msdos_read_super(device, quiet) < 0)
		return -1;

#ifdef DEBUG_MSDOS
	printk("mount: returning successfully\n");
#endif
	return 0;
}

static long msdos_fdsize(int fd)
{
	return (inode_table[fd].entry.size);
}

static int msdos_bread(int fd, long blkno, char *buffer)
{
	long dev_blkno;

#if 0
	printk("msdos_bread: request to read block %d\n", blkno);
#endif

	/* figure out which block to read */
	blkno = blkno * 2;
	if (blkno >= inode_table[fd].blocks) {
		return -1;
	}

	dev_blkno = inode_table[fd].block_list[blkno];
	if (device_read(fd, buffer, 512, dev_blkno * 512) != 512) {
		printk("MSDOS: error reading first half\n");
		return -1;
	}

	/* try and read the second half, but is there one? */
	blkno++;
	if (blkno >= inode_table[fd].blocks) {
		return 0;
/*		return sb->blocksize; */
	}

	dev_blkno = inode_table[fd].block_list[blkno];
	if (device_read(fd, buffer + 512, 512, dev_blkno * 512) != 512) {
		printk("MSDOS: error reading second half\n");
		return 0;
/*		return -1; */
	}

	return 0;
}

static void msdos_ls(char *dirname, char *devname)
{
	int ino;

	ino = msdos_find(dirname);
	if (ino < 0)
		printk("No such file\n");
	else {
		if (!(inode_table[ino].entry.attr & ATTR_DIR))
			msdos_list_entry(&inode_table[ino].entry);
		else
			msdos_list_dir(ino);
		free_inode(ino);
	}
}

static int msdos_open(char *filename)
{
	int ino;

	ino = msdos_find(filename);
	if (ino < 0)
		printk("MSDOS: No such file (%s)\n", filename);
	else {
		/* fill in the block list for this file */
		if (fill_block_list(ino, 1) < 0) {
			free_inode(ino);
			return -1;
		}
	}
	return ino;
}

static void msdos_close(int fd)
{
	if (fd < 0 || fd >= MAX_OPEN_FILES || inode_table[fd].free) {
		printk("Oops: trying to close an non-open file!!\n");
	} else
		free_inode(fd);
}
