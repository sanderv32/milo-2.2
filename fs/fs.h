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

#ifndef MILO_FS_H
#define MILO_FS_H 1

/* 
 *  Milo has a very simple concept of devices and file systems.
 *  Basically, each time you use a device you must mount it.
 *  That means that effectively there is only ever one device
 *  (and therefore one file system) ever mounted.
 */

struct bootfs {
	char *name;
	int fs_type;
	int blocksize;

	int (*mount) (long dev, int quiet);

	int (*open) (char *filename);
	long (*fdsize) (int fd);
	int (*bread) (int fd, long blkno, char *buf);
	void (*ls) (char *dirname, char *devname);
	void (*close) (int fd);

};

extern struct bootfs *milo_open_fs;

#define __mount(fs, device) fs_mount((fs), (device))
#define __open(name) (milo_open_fs->open)((name))
#define __fdsize(fd) (milo_open_fs->fdsize)((fd))
#define __fread(fd,buffer,size,offset) \
               device_read(fd,buffer,size,offset)
#define __bread(fd,blkno,buf) \
               (milo_open_fs->bread)((fd),(blkno),(buf))
#define __ls(dir,dev) (milo_open_fs->ls)((dir),(dev))
#define __close(fd) (milo_open_fs->close)((fd))

extern int fs_name_to_type(char *name);
extern char *fs_type_to_name(int type);
extern void show_known_fs(void);
extern int fs_mount(char *fs_type, char *device);

#endif
