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

/* This is a set of functions that provides minimal filesystem
 * functionality to the Linux bootstrapper.  All we can do is
 * open and read files... but that's all we need 8-)
 */

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>

#include "milo.h"
#include "fs.h"
#include "ext2.h"

#define MAX_OPEN_FILES		4

int fd = -1;
struct ext2_super_block sb;
struct ext2_group_desc *gds = NULL;
int ngroups = 0;
int blocksize;			/* Block size of this fs */
int directlim;			/* Maximum direct blkno */
int ind1lim;			/* Maximum single-indir blkno */
int ind2lim;			/* Maximum double-indir blkno */
int ptrs_per_blk;		/* ptrs/indirect block */
char filename[256];
char blkbuf[EXT2_MAX_BLOCK_SIZE];
long cached_iblkno = -1;
char iblkbuf[EXT2_MAX_BLOCK_SIZE];
long cached_diblkno = -1;
char diblkbuf[EXT2_MAX_BLOCK_SIZE];

static struct inode_table_entry {
	struct ext2_inode inode;
	int inumber;
	int free;
	unsigned short old_mode;
} inode_table[MAX_OPEN_FILES];


/* Initialize an ext2 filesystem; this is sort-of the same idea as
 * "mounting" it.  Read in the relevant control structures and 
 * make them available to the user.  Returns 0 if successful, -1 on
 * failure.
 */

/* forward declarations */
static int ext2_bread(int fd, long blkno, char *buffer);
static void ext2_breadi(struct ext2_inode *ip, long blkno, char *buffer);
static int ext2_mount(long device, int quiet);
static int ext2_open(char *filename);
static void ext2_ls(char *dirname, char *devname);
static void ext2_close(int fd);
static long ext2_fdsize(int fd);

struct bootfs ext2fs = {
	"ext2",
	0,
	0,

	ext2_mount,
	ext2_open,
	ext2_fdsize,
	ext2_bread,
	ext2_ls,
	ext2_close,
};

static int ext2_mount(long device, int quiet)
{
	extern void set_blocksize(kdev_t dev, int size);
	int i;
	long sb_offset;
	int sb_block = 1;

#ifdef DEBUG_EXT2
	printk("ext2_mount(): called, device = 0x%04lx\n", device);
#endif
	/* Initialize the inode table */
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		inode_table[i].free = 1;
		inode_table[i].inumber = 0;
	}

	/* set the device's block size */
	set_blocksize(device, EXT2_MIN_BLOCK_SIZE);

	/* Read in the first superblock */
	sb_offset = sb_block * EXT2_MIN_BLOCK_SIZE;

#ifdef DEBUG_EXT2
	printk("Reading the first superblock() at offset 0x%lx\n",
	       sb_offset);
#endif
	if (device_read(fd, (char *) &sb, sizeof(sb), sb_offset) !=
	    sizeof(sb)) {
		printk("ext2 sb read failed");
		device_close(fd);
		return (-1);
	}

	if (sb.s_magic != EXT2_SUPER_MAGIC) {
		if (!quiet) {
			printk("ext2_mount: bad magic 0x%x\n", sb.s_magic);
		}
		device_close(fd);
		return (-1);
	}
#ifdef DEBUG_EXT2
	printk("Superblock\n");
	printk("   [%d] inodes\n", sb.s_inodes_count);
	printk("   [%d] blocks\n", sb.s_blocks_count);
	printk("   [%d] blocks per group\n", sb.s_blocks_per_group);
#endif

	/* read in the EXT2 block groups */
	ngroups =
	    (sb.s_blocks_count - sb.s_first_data_block +
	     sb.s_blocks_per_group - 1)
	    / sb.s_blocks_per_group;
#ifdef DEBUG_EXT2
	printk("...ngroups is %d\n", ngroups);
#endif
	/* free the last set of groups off and allocate a new vector */
	if (gds != (void *) NULL)
		free((void *) gds);
	gds = (struct ext2_group_desc *) vmalloc(ngroups *
						 sizeof(struct
							ext2_group_desc));
	if (gds == NULL) {
		printk
		    ("ext2_mount: cannot allocate memory for %d groups\n",
		     ngroups);
		device_close(fd);
		return (-1);
	}

	/* Read in the group descriptors (immediately follows superblock) */
#ifdef DEBUG_EXT2
	printk
	    ("...reading in the group descriptors (%d) at offset 0x%lx\n",
	     ngroups, sb_offset + sizeof(sb));
	printk("   size is %lu\n",
	       ngroups * sizeof(struct ext2_group_desc));
#endif
	if (device_read
	    (fd, (char *) gds, ngroups * sizeof(struct ext2_group_desc),
	     sb_offset + sizeof(sb)) !=
	    ngroups * sizeof(struct ext2_group_desc)) {
		printk("ext2_mount: could not read groups\n");
		device_close(fd);
		return (-1);
	}

#ifdef DEBUG_EXT2
	for (i = 0; i < ngroups; i++) {
		printk("Group %d\n", i);
		printk("   [0x%X] blocks bitmap\n",
		       gds[i].bg_block_bitmap);
		printk("   [0x%X] inode bitmap\n", gds[i].bg_inode_bitmap);
		printk("   [0x%X] inode table\n", gds[i].bg_inode_table);
	}
#endif

	/* Calculate direct/indirect block limits for this file system (blocksize
	 * dependent) */
	blocksize = EXT2_BLOCK_SIZE(&sb);
	directlim = EXT2_NDIR_BLOCKS - 1;
	ptrs_per_blk = blocksize / sizeof(unsigned int);
	ind1lim = ptrs_per_blk + directlim;
	ind2lim = (ptrs_per_blk * ptrs_per_blk) + directlim;

#ifdef DEBUG_EXT2
	printk("returning successfully\n");
#endif

	return (0);
}

/* Read the specified inode from the disk and return it to the user.
 * Returns NULL if the inode can't be read...
 */
static struct ext2_inode *ext2_iget(int ino)
{
	int i;
	struct ext2_inode *ip;
	struct inode_table_entry *itp;
	int group;
	unsigned long offset;

#ifdef DEBUG_EXT2
	printk("ext2_iget(): called for ino = %d\n", ino);
	printk("...inodes per group = %d\n", sb.s_inodes_per_group);
#endif

	ip = NULL;
	itp = NULL;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (inode_table[i].free) {
			itp = &(inode_table[i]);
			ip = &(itp->inode);
			break;
		}
	}
	if ((ip == NULL) || (itp == NULL)) {
		printk("ext2_iget: no free inodes\n");
		return (NULL);
	}

	group = (ino - 1) / sb.s_inodes_per_group;
	offset = ((unsigned long) gds[group].bg_inode_table * blocksize)
	    +
	    (((ino - 1) % sb.s_inodes_per_group) *
	     sizeof(struct ext2_inode));

#ifdef DEBUG_EXT2
	printk("...group = %d, gds[group].bg_inode_table = %d\n", group,
	       gds[group].bg_inode_table);
	printk("...reading inode at offset 0x%lx\n", offset);
#endif
	if (device_read(fd, (char *) ip, sizeof(struct ext2_inode), offset)
	    != sizeof(struct ext2_inode)) {
		printk("ext2_iget: read error\n");
		return (NULL);
	}

	itp->free = 0;
	itp->inumber = ino;
	itp->old_mode = ip->i_mode;

#ifdef DEBUG_EXT2
	printk("ext2_iget(): returning inode, size is %d\n", ip->i_size);
#endif

	return (ip);
}

/* Release our hold on an inode.  Since this is a read-only application,
 * don't worry about putting back any changes...
 */
static void ext2_iput(struct ext2_inode *ip)
{
	struct inode_table_entry *itp;

	/* Find and free the inode table slot we used... */
	itp = (struct inode_table_entry *) ip;

	itp->inumber = 0;
	itp->free = 1;
}


/* Map a block offset into a file into an absolute block number.
 * (traverse the indirect blocks if necessary).  Note: Double-indirect
 * blocks allow us to map over 64Mb on a 1k file system.  Therefore, for
 * our purposes, we will NOT bother with triple indirect blocks.
 *
 * The "allocate" argument is set if we want to *allocate* a block
 * and we don't already have one allocated.
 */
static int ext2_blkno(struct ext2_inode *ip, int blkoff, int allocate)
{
	unsigned int *lp;
	unsigned int *ilp;
	unsigned int *dlp;
	int blkno;
	unsigned long iblkno;
	unsigned long diblkno;


	ilp = (unsigned int *) iblkbuf;
	dlp = (unsigned int *) diblkbuf;

	if (allocate) {
		printk
		    ("ext2_blkno: Cannot allocate on a readonly file system!\n");
		return (0);
	}

	lp = (unsigned int *) blkbuf;

	/* If it's a direct block, it's easy! */
	if (blkoff <= directlim) {
		return (ip->i_block[blkoff]);
	}

	/* Is it a single-indirect? */
	if (blkoff <= ind1lim) {
		iblkno = ip->i_block[EXT2_IND_BLOCK];

		if (iblkno == 0) {
			return (0);
		}

		/* Read the indirect block */
		if (cached_iblkno != iblkno) {
#ifdef DEBUG_EXT2
			printk("Reading indirect block at 0x%lx\n",
			       (iblkno * blocksize));
#endif
			if (device_read
			    (fd, iblkbuf, blocksize,
			     (iblkno * blocksize)) != blocksize) {
				printk("ext2_blkno: error on iblk read\n");
				return (0);
			}
			cached_iblkno = iblkno;
		}

		blkno = ilp[blkoff - (directlim + 1)];
		return (blkno);
	}

	/* Is it a double-indirect? */
	if (blkoff <= ind2lim) {

		/* Find the double-indirect block */
		diblkno = ip->i_block[EXT2_DIND_BLOCK];

		if (diblkno == 0) {
			return (0);
		}

		/* Read in the double-indirect block */
		if (cached_diblkno != diblkno) {
#ifdef DEBUG_EXT2
			printk("Reading double indirect block at 0x%lx\n",
			       (diblkno * blocksize));
#endif
			if (device_read
			    (fd, diblkbuf, blocksize,
			     (diblkno * blocksize)) != blocksize) {
				printk
				    ("ext2_blkno: err reading dindr blk\n");
				return (0);
			}
			cached_diblkno = diblkno;
		}

		/* Find the single-indirect block pointer ... */
		iblkno = dlp[(blkoff - (ind1lim + 1)) / ptrs_per_blk];

		if (iblkno == 0) {
			return (0);
		}


		/* Read the indirect block */

		if (cached_iblkno != iblkno) {
			if (device_read
			    (fd, iblkbuf, blocksize,
			     (iblkno * blocksize)) != blocksize) {
				printk("ext2_blkno: err on iblk read\n");
				return (0);
			}
			cached_iblkno = iblkno;
		}

		/* Find the block itself. */
		blkno = ilp[(blkoff - (ind1lim + 1)) % ptrs_per_blk];
		return (blkno);
	}

	if (blkoff > ind2lim) {
		printk("ext2_blkno: block number too large: %d\n", blkoff);
		return (0);
	}

	return (0);
}

/* Return the size of the open file */
static long ext2_fdsize(int fd)
{
	struct ext2_inode *ip;

	ip = &(inode_table[fd].inode);
	return ip->i_size;
}

/* Read block number "blkno" from the specified file */
static int ext2_bread(int fd, long blkno, char *buffer)
{
	struct ext2_inode *ip;

	ip = &(inode_table[fd].inode);
	ext2_breadi(ip, blkno, buffer);
	return 0;
}

static void ext2_breadi(struct ext2_inode *ip, long blkno, char *buffer)
{
	long dev_blkno;


#ifdef DEBUG_EXT2
	printk("ext2_breadi(): called for block %ld\n", blkno);
#endif

	dev_blkno = ext2_blkno(ip, blkno, 0);
#ifdef DEBUG_EXT2
	printk("ext2_breadi(): dev_blkno = %ld\n", dev_blkno);
#endif
	if (dev_blkno == 0) {

		/* This is a "hole" */
		memset(buffer, 0, blocksize);
	} else {

		/* Read it for real */
#ifdef DEBUG_EXT2
		printk
		    ("ext2_breadi(): reading block %ld at offset 0x%lx\n",
		     blkno, (dev_blkno * blocksize));
#endif
		if (device_read
		    (fd, buffer, blocksize,
		     (dev_blkno * blocksize)) != blocksize) {
			printk("ext2_bread: read error");
		}
	}
}


static struct ext2_inode *ext2_namei(char *name)
{
	char namebuf[256];
	char *component;
	struct ext2_inode *dir_inode = NULL, *file_inode;
	struct ext2_dir_entry *dp;
	int next_ino;
	int link_count = 0;


#ifdef DEBUG_EXT2
	printk("ext2_namei(): called for %s\n", name);
#endif

	/* Squirrel away a copy of "namebuf" that we can molest */
	strcpy(namebuf, name);

	/* Start at the root... */
	/* We'll get back here to these nifty labels if we encounter a symlink */
      abs:
	file_inode = ext2_iget(EXT2_ROOT_INO);
      rel:
	if (!file_inode)
		return NULL;

	component = strtok(namebuf, "/");
	while (component) {
		int diroffset;
		int blockoffset;
		int component_length;

		/* Search for the specified component in the current directory inode.
		 */

#ifdef MILO_SYMLINK_DIRS
		if (S_ISLNK(file_inode->i_mode)) {
			if (++link_count > 10)
				printk("%s: possibly circular symlink\n",
				       name);
			else {
				char *t;

				/* Set blkbuf to symbolic link content */
				if (file_inode->i_blocks)
					ext2_breadi(file_inode, 0, blkbuf);
				else
					strcpy(blkbuf,
					       (char *) file_inode->
					       i_block);

				/* Add rest of the path to it */
				t = blkbuf + strlen(blkbuf);
				while (component != NULL) {
					t =
					    stpcpy(stpcpy(t, "/"),
						   component);
					component = strtok(NULL, "/");
				}

				/* Put it back to namebuf */
				strcpy(namebuf, blkbuf);
#ifdef DEBUG_EXT2
				printk
				    ("substituted symlink + rest of path: %s\n",
				     namebuf);
#endif

				if (*namebuf == '/') {
					/* Absolute symlink */
					if (dir_inode)
						ext2_iput(dir_inode);
					if (file_inode)
						ext2_iput(file_inode);
					goto abs;
				} else {
					/* Relative symlink */
					if (file_inode)
						ext2_iput(file_inode);
					file_inode = dir_inode;
					goto rel;
				}
			}
		}
#endif				/* MILO_SYMLINK_DIRS */

		if (dir_inode)
			ext2_iput(dir_inode);
		dir_inode = file_inode;
		file_inode = NULL;
		next_ino = -1;

		if (!S_ISDIR(dir_inode->i_mode)) {
			printk("%s: path component is not a directory\n",
			       name);
			break;
		}

		component_length = strlen(component);
		diroffset = 0;
#ifdef DEBUG_EXT2
		printk("...dir_inode->i_size = %d\n", dir_inode->i_size);
#endif
		while (diroffset < dir_inode->i_size) {
			blockoffset = 0;
#ifdef DEBUG_EXT2
			printk("ext2_namei(): calling ext2_breadi\n");
#endif
			ext2_breadi(dir_inode, diroffset / blocksize,
				    blkbuf);
			while (blockoffset < blocksize) {
				dp =
				    (struct ext2_dir_entry *) (blkbuf +
							       blockoffset);
#if DEBUG_EXT2
				{
					int i;

					printk("...name is ");
					for (i = 0; i < dp->name_len; i++)
						printk("%c", dp->name[i]);
					printk("\n");
				}
#endif
				if ((dp->name_len == component_length) &&
				    (strncmp(component, dp->name,
					     component_length) == 0)) {

					/* Found it! */
					next_ino = dp->inode;
					break;
				}

				/* Go to next entry in this block */
				blockoffset += dp->rec_len;
			}
			if (next_ino >= 0) {
				break;
			}

			/* If we got here, then we didn't find the component. Try the next 
			 * block in this directory... */
			diroffset += blocksize;
		}		/* end-while */

		/* If next_ino is negative, then we've failed (gone all the way through
		 * without finding anything) */
		if (next_ino < 0) {
#ifdef DEBUG_EXT2
			printk("...we failed\n");
#endif
			break;
		}

		/* Otherwise, we can get this inode and find the next component
		 * string... */
		file_inode = ext2_iget(next_ino);

		component = strtok(NULL, "/");
	}

	/* If we get here, then we got through all the components. Whatever we got 
	 * must match up with the last one. */
	if (file_inode && S_ISLNK(file_inode->i_mode)) {	/* Is it a symlink? */
		if (++link_count > 5)
			printk("%s: possibly circular symlink\n", name);
		else {
			if (file_inode->i_blocks) {
				ext2_breadi(dir_inode, 0, blkbuf);
				strncpy(namebuf, blkbuf,
					sizeof namebuf - 1);
			} else
				strncpy(namebuf,
					(char *) file_inode->i_block,
					sizeof namebuf - 1);
			ext2_iput(file_inode);
			if (*namebuf == '/') {	/* absolute */
				ext2_iput(dir_inode);
				goto abs;
			}	/* otherwise it's relative */
			file_inode = dir_inode;
			goto rel;
		}
	}
	if (dir_inode)
		ext2_iput(dir_inode);
	return (file_inode);
}

static void ext2_ls(char *dirname, char *devname)
{
	char filename[512];
	char format_string[16];
	struct ext2_inode *dir_inode;
	struct ext2_inode *file_inode;
	struct ext2_dir_entry *dp;
	int diroffset;
	int blockoffset;
	int column_size = 0;
	int columns;
	int col;
	char typetag;


#ifdef DEBUG_EXT2
	printk("ext2_ls(): called for %s\n", dirname);
#endif

	/* Get the inode for the specified directory */
	dir_inode = ext2_namei(dirname);
	if (!dir_inode) {
		printk("%s: no such file or directory\n", dirname);
		return;
	}

	/* If this inode exists but is *not* a directory, just list it here */
	if ((dir_inode->i_mode & S_IFMT) != S_IFDIR) {
#ifdef BOOT_VERBOSE
		printk("Directory of: %s:/%s\n", devname, dirname);
#endif
		printk("%s\n", dirname);
		ext2_iput(dir_inode);
		return;
	}

	/* First pass: determine maximum column size */

	diroffset = 0;
	while (diroffset < dir_inode->i_size) {
		blockoffset = 0;
		ext2_breadi(dir_inode, diroffset / blocksize, blkbuf);
		while (blockoffset < blocksize) {
			dp =
			    (struct ext2_dir_entry *) (blkbuf +
						       blockoffset);
			if (dp->name_len > column_size) {
				column_size = dp->name_len;
			}
			blockoffset += dp->rec_len;
		}

		/* Go to the next  block in this directory... */
		diroffset += blocksize;
	}			/* end-while */

	/* Compute the format string for this listing... */
	column_size += 3;	/* Add spacer and type tag */
	if (column_size >= 79) {
		columns = 1;
		strcpy(format_string, "%s");
	} else {
		columns = 79 / column_size;
		column_size = 79 / columns;
		sprintf(format_string, "%%-%ds", column_size);
	}

	/* Second pass: Get all the filenames and modes and
	 * print them out...
	 */
	diroffset = 0;
	col = 0;
#ifdef BOOT_VERBOSE
	printk("Directory of: %s:/%s\n", devname, dirname);
#endif
	while (diroffset < dir_inode->i_size) {
		blockoffset = 0;
		ext2_breadi(dir_inode, diroffset / blocksize, blkbuf);
		while (blockoffset < blocksize) {
			dp =
			    (struct ext2_dir_entry *) (blkbuf +
						       blockoffset);
			if (dp->name_len > 0) {
				strncpy(filename, dp->name, dp->name_len);
				file_inode = ext2_iget(dp->inode);
				if (file_inode) {
					switch (file_inode->
						i_mode & S_IFMT) {
					case S_IFLNK:
						typetag = '@';
						break;
					case S_IFDIR:
						typetag = '/';
						break;
					case S_IFIFO:
					case S_IFSOCK:
						typetag = '=';
						break;
					default:
						typetag = ' ';
						break;
					}
					ext2_iput(file_inode);
				} else {
					typetag = ' ';
				}
				filename[dp->name_len] = typetag;
				filename[dp->name_len + 1] = '\0';
				printk(format_string, filename);
				if ((++col % columns) == 0) {
					printk("\n");
				}
			}
			blockoffset += dp->rec_len;
		}

		/* Go to the next  block in this directory... */
		diroffset += blocksize;
	}			/* end-while */

	if ((col % columns) != 0) {
		printk("\n");
	}

	/* We're done with the directory now... */
	ext2_iput(dir_inode);

}

static int ext2_open(char *filename)
{

	/* Unix-like open routine.  Returns a small integer (actually an index into
	 * the inode table... */
	struct ext2_inode *ip;

#ifdef DEBUG_EXT2
	printk("ext2_open() called with filename = %s\n", filename);
#endif
	ip = ext2_namei(filename);
#ifdef DEBUG_EXT2
	printk("...namei returned 0x%p\n", ip);
#endif

	if (ip) {
		struct inode_table_entry *itp;

		itp = (struct inode_table_entry *) ip;
		return (itp - inode_table);
	} else {
		return (-1);
	}
}


static void ext2_close(int fd)
{
	ext2_iput(&(inode_table[fd].inode));
#ifdef DEBUG_EXT2
	printk("Closing device, fd=%d\n", fd);
#endif
	device_close(fd);
}

