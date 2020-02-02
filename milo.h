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
 *  This module contains miniboot specific definitions and macros.
 */

#ifndef MINIBOOT_H
#define MINIBOOT_H 1

#include "autoconf.h"
#include <asm/io.h>
#include <linux/config.h>
#include <linux/delay.h>

#ifndef U64
#if defined(__linux__) && defined(__alpha__)
#define U64 unsigned long
#else
#define U64 unsigned long long
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MILO_MIN_MEMORY_SIZE 0x1000000	/* 16Mb, seems reasonable... */

/*****************************************************************************
 * external routine declarations.                                            *
 *****************************************************************************/

#ifdef DEBUG_MALLOC
#define kmalloc(size,prio) __kmalloc(size,prio,__FILE__,__LINE__)
#define vmalloc(size) __kmalloc(size,1,__FILE__,__LINE__)
extern void *__kmalloc(unsigned int size, int priority, const char *file,
		       const int line);
#else
extern void *kmalloc(size_t size, int priority);
extern void *vmalloc(size_t size);
#endif
#define malloc	vmalloc
extern void free(void *ptr);

extern void WriteQ(U64 where, U64 value);
extern U64 ReadQ(U64 where);

extern char uart_getchar(int port);
extern void uart_putchar(int port, char c);
extern int uart_init(void);
extern int uart_charav(int port);

extern int ipl(int newipl);
extern void wrfen(void);
extern U64 cServe(U64 a1, U64 a2, U64 a3);
extern void wrsp(U64 sp);

extern void milo_cpu_init(void);

extern void device_init(void);

extern void init_IRQ(void);

extern void init_timers(void);

extern void resetenv(void);
extern void setenv(char *var, char *value);
extern void unsetenv(char *var);
extern char *getenv(char *var);
extern void printenv(void);
extern void env_init(void);

extern void zeropage_phys(int pfn);
void init_mem(U64 Top);
extern void reset_mem(U64 Top);

#ifndef LINUX_AT
extern int device_open(const char *name);
extern void device_close(int fd);
extern int device_read(int fd, char *buffer, int size,
		       unsigned long offset);

extern void show_devices(void);
extern int need_device(const char *devname);
extern U64 load_image_from_device(char *fs, char *device, char *file);
extern void load_ramdisk_from_device(char *fs, char *device, char *file);
extern int device_mount(char *device, char *fs_type);
extern void build_HWRPB_mem_map(void);
extern void init_HWRPB(void);
extern void make_HWRPB_virtual(void);
#endif
extern void mark_many_pfn(int pfn, int count, int type);

extern void add_VA(U64 va, unsigned int vpfn, unsigned int protection);

extern void swap_to_palcode(void *palbase, void *pc, void *ksp,
			    U64 new_ptbr, void *new_vptbr);

extern void kbd_init(void);
extern int kbd_getc_with_timeout(int nsecs);
extern int kbd_getc(void);
extern int kbd_gets(char *charbuf, int maxlen);

extern int device_name_to_number(const char *name);
extern struct inode *device_curr_inode(void);
extern char *dev_name(unsigned int device);

extern int atoi(const char *s);
extern char *stpcpy(char *dest, const char *src);
extern char *copy(const char *src);
extern int tolower(int);
extern void error(const char *s);

extern int uncompress_kernel(int fd, void *where);

extern void nvram_copyin(int offset, char *dest, int size);
extern void nvram_copyout(int offset, char *src, int size);

extern void milo(void);

/*****************************************************************************
 * Page frame macros                                                         *
 *****************************************************************************/
extern unsigned int allocate_pfn(int type);
unsigned int allocate_many_pfn(unsigned int count, int type);

#define FREE_PAGE 0
#define ALLOCATED_PAGE 2
#define TEMPORARY_PAGE 4

#define _ALLOCATE_PFN()            allocate_pfn(ALLOCATED_PAGE)
#define _PFN_TO_PHYSICAL(pfn)      (((unsigned long)((unsigned long)(pfn) << PAGE_SHIFT)) | PAGE_OFFSET)
#define _PHYSICAL_TO_PFN(physical) (((unsigned long)(physical) & (~PAGE_OFFSET)) >> PAGE_SHIFT)

/*****************************************************************************
 * General macros                                                            *
 *****************************************************************************/

#define ENVIRON_ID	"MILO Environment"

/*
 *  Signatures for the impure areas.   
 */
#ifdef DC21064
#define IMPURE_SIGNATURE	0xDECb0001
					/* true at least for Cabriolet */
#endif

#ifdef DC21066
#if defined(MINI_ALPHA_NONAME) || defined(MINI_ALPHA_EB66P) || defined(MINI_ALPHA_EB66)
#define IMPURE_SIGNATURE	0xDECa0001
#endif
#if defined(MINI_ALPHA_P2K)
#define IMPURE_SIGNATURE        0xDECb0001
#endif
#endif

#ifdef DC21164
#define IMPURE_SIGNATURE	0xDECb0001
					/* true at least for Cabriolet */
#endif

#ifndef MAX_KERNEL_SIZE
// #define MAX_KERNEL_SIZE		0x400000
#define MAX_KERNEL_SIZE		0x800000
#endif

/*****************************************************************************
 * Reboot parameter block details.                                           *
 *****************************************************************************/
/* hwrpb signature is HWRPB\0\0\0 */
#define HWRPB_SIGNATURE 0x4857525042000000

typedef struct {
	unsigned long signature1;
	unsigned long flags;
	unsigned short boot_device;
	short boot_filesystem;
	unsigned short memory_size;
	unsigned short spare2;
	unsigned char boot_filename[32];
	unsigned char boot_string[64];
	unsigned long signature2;
} Milo_reboot_t;

#define REBOOT_SIGNATURE 0x4d49444f4f44494d
#define REBOOT_REBOOT 1
#define REBOOT_CONSOLE 2

#define IOBUF_SIZE 1024

extern Milo_reboot_t *milo_reboot_block;

#endif

extern int milo_verbose;

/* Version information */

extern char banner[], version[];
extern long build_time;
