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

#ifndef _DOSFS_FS_H
#define _DOSFS_FS_H

#include <linux/types.h>
/*
 * The MSDOS filesystem constants/structures
 */

#define MSDOS_ROOT_INO  1	/* == MINIX_ROOT_INO */
#define SECTOR_SIZE     512	/* sector size (bytes) */
#define SECTOR_BITS	9	/* log2(SECTOR_SIZE) */
#define MSDOS_DPB	(MSDOS_DPS)	/* dir entries per block */
#define MSDOS_DPB_BITS	4	/* log2(MSDOS_DPB) */
#define MSDOS_DPS	(SECTOR_SIZE/sizeof(struct msdos_dir_entry))
#define MSDOS_DPS_BITS	4	/* log2(MSDOS_DPS) */
#define MSDOS_DIR_BITS	5	/* log2(sizeof(struct msdos_dir_entry)) */

#define MSDOS_SUPER_MAGIC 0x4d44	/* MD */

#define FAT_CACHE    8		/* FAT cache size */

#define MSDOS_MAX_EXTRA	3	/* tolerate up to that number of clusters which are
				   inaccessible because the FAT is too short */

#define ATTR_RO      1		/* read-only */
#define ATTR_HIDDEN  2		/* hidden */
#define ATTR_SYS     4		/* system */
#define ATTR_VOLUME  8		/* volume label */
#define ATTR_DIR     16		/* directory */
#define ATTR_ARCH    32		/* archived */

#define ATTR_NONE    0		/* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
	/* attribute bits that are copied "as is" */

#define DELETED_FLAG 0xe5	/* marks file as deleted when in name[0] */
#define IS_FREE(n) (!*(n) || *(const unsigned char *) (n) == DELETED_FLAG || \
  *(const unsigned char *) (n) == 0xF6)

#define MSDOS_VALID_MODE (S_IFREG | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO)
	/* valid file mode bits */

#define MSDOS_SB(s) (&((s)->u.msdos_sb))
#define MSDOS_I(i) (&((i)->u.msdos_i))

#define MSDOS_NAME 11		/* maximum name length */
#define MSDOS_DOT    ".          "	/* ".", padded to MSDOS_NAME chars */
#define MSDOS_DOTDOT "..         "	/* "..", padded to MSDOS_NAME chars */

#define MSDOS_FAT12 4078	/* maximum number of clusters in a 12 bit FAT */

/*
 * Conversion from and to little-endian byte order. (no-op on i386/i486)
 *
 * Naming: Ca_b_c, where a: F = from, T = to, b: LE = little-endian, BE = big-
 * endian, c: W = word (16 bits), L = longword (32 bits)
 */

#define CF_LE_W(v) (v)
#define CF_LE_L(v) (v)
#define CT_LE_W(v) (v)
#define CT_LE_L(v) (v)


struct msdos_boot_sector {
	__s8 ignored[3];	/* Boot strap short or near jump */
	__s8 system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8 sector_size[2];	/* bytes per logical sector */
	__u8 cluster_size;	/* sectors/cluster */
	__u16 reserved;		/* reserved sectors */
	__u8 fats;		/* number of FATs */
	__u8 dir_entries[2];	/* root directory entries */
	__u8 sectors[2];	/* number of sectors */
	__u8 media;		/* media code (unused) */
	__u16 fat_length;	/* sectors/FAT */
	__u16 secs_track;	/* sectors per track */
	__u16 heads;		/* number of heads */
	__u32 hidden;		/* hidden sectors (unused) */
	__u32 total_sect;	/* number of sectors (if sectors == 0) */
};

struct msdos_dir_entry {
	__s8 name[8], ext[3];	/* name and extension */
	__u8 attr;		/* attribute bits */
	__u8 unused[10];
	__u16 time, date, start;	/* time, date and first cluster */
	__u32 size;		/* file size (in bytes) */
};

/* Determine whether this FS has kB-aligned data. */

#define MSDOS_CAN_BMAP(sb) (!(((sb).cluster_size & 1) || \
    ((sb).data_start & 1)))

/* Convert attribute bits and a mask to the UNIX mode. */

#define MSDOS_MKMODE(a,m) (m & (a & ATTR_RO ? S_IRUGO|S_IXUGO : S_IRWXUGO))

/* Convert the UNIX mode to MS-DOS attribute bits. */

#define MSDOS_MKATTR(m) ((m & S_IWUGO) ? ATTR_NONE : ATTR_RO)

struct msdos_sb {
	unsigned int blocksize;
	unsigned int blocksize_bits;
	unsigned short cluster_size;	/* sectors/cluster */
	unsigned char fats, fat_bits;	/* number of FATs, FAT bits (12 or 16) */
	unsigned short fat_start, fat_length;	/* FAT start & length (sec.) */
	unsigned short dir_start, dir_entries;	/* root dir start & entries */
	unsigned short data_start;	/* first data sector */
	unsigned long clusters;	/* number of clusters */
	uid_t fs_uid;
	gid_t fs_gid;
	int quiet;		/* fake successful chmods and chowns */
	unsigned short fs_umask;
	unsigned char name_check;	/* r = relaxed, n = normal, s = strict */
	unsigned char conversion;	/* b = binary, t = text, a = auto */
};

#endif
