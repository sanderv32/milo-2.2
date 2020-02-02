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
 *  devices.c.   This part of the miniloader contains the ersatz 
 *               (as in fake rollexs) Linux kernel.
 *
 *  david.rusling@reo.mts.dec.com.
 */

#include <linux/config.h>
#include <linux/pci.h>
#include <linux/locks.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include "milo.h"
#ifdef MINI_ALPHA_LCA
#include <asm/core_lca.h>
#endif
#include "uart.h"
#include "fs.h"

extern void video_init(void);
extern struct blk_dev_struct blk_dev[MAX_BLKDEV];

/*
 *  Globals
 */

struct device_struct {
	const char *name;
	struct file_operations *fops;
};

static struct device_struct chrdevs[MAX_CHRDEV] = {
	{NULL, NULL},
};

struct device_struct blkdevs[MAX_BLKDEV] = {
	{NULL, NULL},
};

extern unsigned long bh_active;
extern unsigned long bh_mask;

extern int *blk_size[];
extern int *blksize_size[];

int need_resched = 0;

struct wait_queue *wait_for_request;

kdev_t ROOT_DEV = 0;

#undef INIT_MM
#define INIT_MM {					\
                NULL, NULL, NULL,			\
                NULL,					\
                ATOMIC_INIT(1), 1,			\
                MUTEX,					\
                0,					\
                0, 0, 0, 0,				\
                0, 0, 0,				\
                0, 0, 0, 0,				\
                0, 0, 0,				\
                0, 0, 0, 0, NULL }

mem_map_t *mem_map = NULL;

struct drive_info_struct {
	char dummy[32];
} drive_info;

struct fs_struct init_fs = INIT_FS;
struct files_struct init_files = INIT_FILES;
struct signal_struct init_signals = INIT_SIGNALS;
struct mm_struct init_mm = INIT_MM;

unsigned securebits = SECUREBITS_DEFAULT;	/* systemwide security settings */

struct exec_domain default_exec_domain;

union task_union init_task_union __attribute__ ((section("init_task"))) = { task:INIT_TASK };

struct task_struct *task[NR_TASKS];
struct task_struct *current_set[NR_CPUS];

#undef current			/* defined in sched.h for SMP */
register struct task_struct *current = &init_task;

extern char *floppy_track_buffer;

#ifdef CONFIG_SCSI
int (*dispatch_scsi_info_ptr) (int ino, char *buffer, char **start,
			       off_t offset, int length, int inout)=NULL;
#endif

#define ALIGN(val,align)	(((val) + ((align) - 1)) & ~((align) - 1))

/*
 *  This module implements the dummy device layer in the miniloader.
 *  It must be run after the PCI initialization code is run.
 */
extern void entUna(void);
extern int init_timerirq(void);

extern void device_setup(void);
extern long blk_dev_init(long mem_start, long mem_end);

unsigned int current_device = 0;

static unsigned char *start_mem = 0;
static unsigned char *end_mem = 0;

void buffer_end_io(struct buffer_head *bh, int uptodate);


/*
 * The the hostid of the INDEX-th SCSI controller according to
 * environment variable SCSI<index>_HOSTID or a default value.  The
 * PCI configuration space register that controls the hostid is
 * located at offset WHERE (this obviously assumes that there is one
 * byte that controls the hostid).
 */
static void scsi_sethostid(unsigned char bus, unsigned char devfn,
			   unsigned char where, int index)
{
	char buf[16], *val, cur;
	int hostid;

	sprintf(buf, "SCSI%d_HOSTID", index);
	hostid = 7;
	val = getenv(buf);

	if (val)
		hostid = atoi(val);

	pcibios_read_config_byte(bus, devfn, where, &cur);

	if (cur != hostid) {
#ifdef DEBUG_DEVICES
		printk("dev_config: setting NCR SCSI%d host id to %d\n",
		       index, hostid);
#endif
		pcibios_write_config_byte(bus, devfn, where, hostid);
	}
}


/*
 * Configure Saturn PCI/ISA bridge (aka 82378).  In the future,
 * support for setting up ISA address decoding etc. should be added.
 */
static void saturn_config(struct pci_bus *bus, u8 devfn)
{
	struct pci_dev *dev;
	u16 max_devsel;
	u8 old;

	/*
	 * Walk through devices on bus and determine subtractive decoding
	 * sampling point:
	 */
	max_devsel = PCI_STATUS_DEVSEL_FAST;
	for (dev = bus->devices; dev; dev = dev->sibling) {
		u16 status;

		pcibios_read_config_word(bus->number, dev->devfn,
					 PCI_STATUS, &status);
		status &= PCI_STATUS_DEVSEL_MASK;
		if (status > max_devsel) {
			max_devsel = status;
		}
	}
	max_devsel = 2 - (max_devsel >> 9);

	/*
	 * Set devsel sampling point, Interrupt Acknowledge Enable,
	 * PCI Posted Write Buffer Enable, ISA Master Line Buffer enable,
	 * DMA Line Buffer enable.
	 */

	pcibios_write_config_byte(bus->number, devfn, 0x40,
				  0x27 | max_devsel << 3);

	/*
	 * Enable IRQ12/M mouse function enable bit in the ISA Clock
	 * Divisor Register.  This has the effect that a read from port
	 * 0x60 will release IRQ12 (see section "4.5.5 Reset UBus IRQ12
	 * Register" on page 103 of the Saturn manual.
	 */

	pcibios_read_config_byte(bus->number, devfn, 0x4d, &old);
	pcibios_write_config_byte(bus->number, devfn, 0x4d, old | 0x10);
}


/*
 * Do devices configuration that Linux doesn't want to worry
 * about.  This includes setting SCSI hostids and setting up
 * PCI bridges (in particular the PCI-to-ISA bridge).
 */
unsigned char default_PCI_master_latency = 32;
void dev_config(void)
{
	struct pci_dev *dev;
	int scsi_index = 0;
	unsigned char dlc;
	unsigned short cmd;

	/* go through PCI devices and setup things that we know about: */
	for (dev = pci_devices; dev != 0; dev = dev->next) {
		switch (dev->vendor) {
		case PCI_VENDOR_ID_NCR:
			switch (dev->device) {
			case PCI_DEVICE_ID_NCR_53C810:
			case PCI_DEVICE_ID_NCR_53C815:	/* not sure about this */
			case PCI_DEVICE_ID_NCR_53C820:	/* not sure about this */
			case PCI_DEVICE_ID_NCR_53C825:	/* not sure about this */
			case PCI_DEVICE_ID_NCR_53C875:	/* not sure about this */
			case PCI_DEVICE_ID_NCR_53C875J:	/* not sure about this */
				scsi_sethostid(dev->bus->number,
					       dev->devfn, 0x84,
					       scsi_index++);
				break;
			default:
				/*
				 * If NCR ever comes up with something other than
				 * a SCSI controller, this will have to become
				 * silent...
				 */
#ifdef DEBUG_DEVICES
				printk
				    ("dev_config: don't know how to set host id "
				     "of NCR device %x.\n", dev->device);
#endif
			}
			break;

		case PCI_VENDOR_ID_INTEL:
			switch (dev->device) {
			case PCI_DEVICE_ID_INTEL_82378:
				saturn_config(dev->bus, dev->devfn);
				break;
			case PCI_DEVICE_ID_INTEL_82371SB_0:
				pci_read_config_byte(dev, 0x82, &dlc);
				if (!(dlc & 1<<1)) {
					printk("PIIX3: Enabling Passive Release\n");
					dlc |= 1<<1;
					pci_write_config_byte(dev, 0x82, dlc);
				}
				break;
			default:
				if ((dev->class >> 8) ==
				    PCI_CLASS_BRIDGE_ISA) {
					printk
					    ("dev_config: don't know about ISA bridge %x:%x\n",
					     dev->vendor, dev->device);
				}
			}
#ifdef MINI_ALPHA_SX164
		case PCI_VENDOR_ID_CONTAQ:
			if (dev->device == PCI_DEVICE_ID_CONTAQ_82C693)
				continue;	/* skip completely */
#endif				/* SX164 */

		default:
#ifdef DEBUG_DEVICES
			if ((dev->class >> 8) == PCI_CLASS_STORAGE_SCSI) {
				printk
				    ("dev_config: don't know how to set hostid of SCSI device (%x:%x).\n",
				     dev->vendor, dev->device);
			}
#endif
			break;
		}		/* end switch on vendor */
		/* now set up the PCI latency for bus mastering devices */
		pcibios_read_config_word(dev->bus->number, dev->devfn,
					 PCI_COMMAND, &cmd);
		if (cmd & PCI_COMMAND_MASTER) {
			unsigned char latency;

			pcibios_read_config_byte(dev->bus->number,
						 dev->devfn,
						 PCI_LATENCY_TIMER,
						 &latency);

			if (latency != default_PCI_master_latency) {
#ifdef DEBUG_DEVICES
				printk
				    ("dev_config:fixing up latency from %d to %d for %x:%x\n",
				     latency, default_PCI_master_latency,
				     dev->vendor, dev->device);
#endif
				pcibios_write_config_byte(dev->bus->number,
							  dev->devfn,
							  PCI_LATENCY_TIMER,
							  default_PCI_master_latency);
			}
		}
	}			/* end for all PCI devices */
}



/*
 * dummy routines that device_init() calls
 */
void chr_dev_init(void)
{
	int i;
	for (i = 0; i < MAX_CHRDEV; i++) {
		chrdevs[i].name = NULL;
		chrdevs[i].fops = NULL;
	}
}

void net_dev_init(void)
{
}

static void block_device_init(void)
{
	int i;

	extern int milo_suppress_printk;
	extern void device_setup(void);

	for (i = 0; i < MAX_BLKDEV; i++) {
		blkdevs[i].name = NULL;
	}

	/* do the setup */
	printk("Please wait while setting up devices%s",
	       milo_verbose ? "\n" : "");
	milo_suppress_printk = !milo_verbose;

	sti();
	device_setup();		/* in genhd.c */

	milo_suppress_printk = 0;
	if (i) {
		/*
		 * Now tell the world what devices there are.
		 */
		printk("\nMajor devices are: ");
		for (i = 0; i < MAX_BLKDEV; i++) {
			if (blkdevs[i].name != NULL)
				printk("%s (%04X) ", blkdevs[i].name, i);
		}
		printk("\n");
	} else
		printk("\n\n");


	/* allow interrupts to happen again (just in case they got 
	   turned off) */
	sti();
	/* Apparently this doesn't help much, as they get turned off somewhere else.
	   To be on the safe side, I have put sti() in kbd.c as well. */
}

int need_device(const char *device)
{
	static int inited = 0;

	if (!inited) {
		inited = 1;
		block_device_init();
	}

	if (device == NULL)
		return 1;
	else {
		int devnum = device_name_to_number(device);

		/* Return false if the specified device doesn't exist */
		if (devnum < 0)
			return 0;
		else
			return blkdevs[(devnum >> 8) & 0xff].name != NULL;
	}
}

void immediate_bh(void)
{
	run_task_queue(&tq_immediate);
}

/*****************************************************************************
 * Initialise minimal set of devices                                         *
 *****************************************************************************/
void device_init(void)
{
	unsigned int i;
	/*
	 * init
	 */
	for (i = 0; i < NR_TASKS; i++)
		task[i] = 0;
	task[0] = &init_task;
	current = task[0];
	current_set[0] = &init_task;

	/*
	 *  Initialise the driver bottom halves.
	 */
	init_bh(IMMEDIATE_BH, immediate_bh);

	if (milo_verbose)
		printk("Initializing unaligned access traps\n");
	wrent(entUna, 4);

	current->pid = 0;
	current->blocked = (sigset_t) { {0} };
	current->signal = (sigset_t) { {1} };

	if (milo_verbose)
		printk("Initializing non-block devices\n");

	/*
	 * Get some temporary memory.
	 */
	start_mem = kmalloc(8 * PAGE_SIZE, 0);
	end_mem = start_mem + (8 * PAGE_SIZE);
#ifdef DEBUG_DEVICES
	printk("...start_mem @ 0x%p, end_mem @ 0x%p\n", start_mem,
	       end_mem);
#endif

/*
 *  If we are setting up PCI, then do so.   
 */
#ifdef CONFIG_PCI
#ifdef DC21066
	printk("Disable parity checking in PCI\n");
	/* disable parity on the LCA */
	*(volatile unsigned long *) LCA_IOC_PAR_DIS = 1UL << 5;
#endif

	if (milo_verbose)
		printk("Configuring PCI\n");

	pci_init();
	alpha_mv.pci_fixup();
#endif

	init_timerirq();
	init_IRQ();
	init_timers();

	/* initialise the keyboard */
	if (milo_verbose)
		printk("Initializing keyboard\n");

	kbd_init();

	/* init the video */
	video_init();

#if defined(CONFIG_RTC) && !defined(CONFIG_RTC_LIGHT)
	rtc_init_pit();
#else
	alpha_mv.init_pit();
#endif

	printk("Initializing timers\n");
	init_timers();
	init_IRQ();

	/* do the device init that Linux doesn't do */
	dev_config();
}

void show_devices(void)
{
	extern char *disk_name(struct gendisk *hd, int minor, char *buf);
	struct gendisk *p;
	char buf[16];
	int i;

	need_device(NULL);
	printk("Devices:\n");
	for (i = 0; i < MAX_BLKDEV; i++) {
		if (blkdevs[i].name != NULL)
			printk("    %s (%04X) ", blkdevs[i].name, i << 8);
	}
	printk("\n");

	/* for every device */
	for (p = gendisk_head; p; p = p->next) {
		int drive;

		printk("    ");
		/* for every drive on this device */
		for (drive = 0; drive < p->nr_real; drive++) {
			int first_minor = drive << p->minor_shift;
			int mask = (1 << p->minor_shift) - 1;
			int i;

			/* eg sda:, sdb: */
			disk_name(p, first_minor, buf);
			printk("%s:  ", buf);

			/* for every partition */
			for (i = 1; i < mask; i++) {
				printk(" ");
				if (p->part[first_minor + i].nr_sects) {
					disk_name(p, first_minor + i, buf);
					printk(buf);
				}
			}
		}
		printk("\n");
	}
}

/*****************************************************************************
 * Device routines                                                           *
 *****************************************************************************/

/*
 *  These device routines do not need locking (via cli() and sti()) as they
 *  are only called by Milo, which is single threaded.
 *  FIXME: Is this still true for Milo 2.2?
 */

static struct file file;
static struct inode inode;
static struct dentry dentry;

struct inode *device_curr_inode(void)
{
	return &inode;
}

int device_open(const char *device)
{
	int number, major, minor, result;

#ifdef DEBUG_DEVICES
	printk ("device_open() called.");
#endif

	/* parse the device name into a device number, -1 means that we failed */
	number = device_name_to_number(device);

	/* we need the device */
	if (number < 0 || !need_device(device))
		return -1;

	/* set the device's block size */
	set_blocksize(number, 512);

	/* carry on, major is the index intothe blkdevs[] structure */
	major = (number >> 8) & 0xff;
	minor = (number >> 0) & 0xff;

	/* get the set of things that this device can do for us */
	file.f_op = blkdevs[major].fops;

/*
 *  Fill out the static structures that describe to the block device what we
 *  have opened.
 */
	file.f_pos = 0x0;

	file.f_dentry = &dentry;
	dentry.d_inode = &inode;

	file.f_mode = MS_RDONLY;
	inode.i_rdev = number;

/*
 *  If there's a device specific open routine, then go call it.
 *  The two device open routines that I've checked (floppy_open() and 
 *  sd_open() both return 0 for success and -ERROR for failure.
 */
	if (file.f_op->open != NULL) {
		result = (file.f_op->open) (&inode, &file);
		if (result) {
			printk
			    ("ERROR: failed to open device 0x%04X (result = %d)\n",
			     number, result);
			return -1;	/* error */
		}
	}
	return 0;
}

/*
 *  This called from boot main to mount the device so that it can get an
 *  image from it.
 */
int device_mount(char *device, char *fs_type)
{
#ifdef DEBUG_DEVICES
	printk("device_mount(): called for %s\n", device);
#endif

/*
 *  Now mount the filesystem. 
 */
	return __mount(fs_type, device);
}


/*
 *  Attempt to read an opened device.  fd is the fd returned by device_open()
 *  and is an index into the blkdevs[] array.
 */
int device_read(int fd, char *buffer, int size, unsigned long offset)
{
	loff_t ppos = 0;

#ifdef DEBUG_DEVICES
	printk
	    ("device_read: called, %d bytes at offset 0x%lx into buffer @ 0x%p\n",
	     size, offset, buffer);
#endif

	if (file.f_op == NULL) {
		printk("...invalid device (f_op = NULL)\n");
		return 0;	/* invalid device */
	}

/*
 *  if there is one, call the read routine.
 */
	if (file.f_op->read != NULL) {
		int count;
#ifdef DEBUG_DEVICES
		int i;
#endif

		file.f_pos = offset;

/* int block_read(struct inode * inode, struct file * filp, char * buf, int count) */
/* ssize_t block_read(struct file *filp, char *buf, size_t count, loff_t *ppos)            */
/*	count = (file.f_op->read)(&inode, &file, buffer, size);  */

		count =
		    (int) (file.f_op->read) (&file, buffer, size, &ppos);


		if (count < 0) {
			printk("device_read: failed (%d)\n", count);
			count = 0;
		} else {

#ifdef DEBUG_DEVICES
			printk("device_read: %d bytes read\n", count);
			for (i = 0; i < 20; i++)
				printk("%02x ", buffer[i] & 0xFF);
			printk("\n");
#endif
		}
		return count;
	} else {
		printk("...no read routine for this device\n");
		return 0;
	}

}

void device_close(int fd)
{
#ifdef DEBUG_DEVICES
	printk
	    ("device_close: current_device = 0x%04X, release is @0x%p\n",
	     current_device, file.f_op->release);
#endif
	inode.i_rdev = current_device;
#if 0
	if (file.f_op->release)
		(file.f_op->release) (&inode, &file);
	if (file.f_op->revalidate)
		(file.f_op->revalidate) (current_device);
#endif
	current_device = -1;
}

/*****************************************************************************
 * Support routines                                                          *
 *****************************************************************************/
/*
 *  These are routines that are required by Linux device drivers and for
 *  various reasons are not taken from the Kernel (as the irq handling
 *  code is).
 */

static inline void __sleep_on(struct wait_queue **p, int state)
{
	int *ptr = (int *) p;

#ifdef DEBUG_DEVICES
	printk("__sleep_on: queue = 0x%p, *p = 0x%p\n", p, *p);
#endif

	/* Wait 100us. */
	*ptr = *ptr + 1;
	while (*ptr > 0)
		schedule();
}

void interruptible_sleep_on(struct wait_queue **p)
{
	unsigned long flags;

	save_flags(flags);
	sti();
#ifdef DEBUG_DEVICES
	printk("interruptible_sleep_on: queue = 0x%p, ipl = %lu\n", p, flags);
#endif
	__sleep_on(p, TASK_INTERRUPTIBLE);
	restore_flags(flags);
}

void sleep_on(struct wait_queue **p)
{
	unsigned long flags;

#ifdef DEBUG_DEVICES
	printk("sleep_on: queue @ 0x%p\n", p);
#endif
	save_flags(flags);
	sti();

	udelay(100);		/* Wait 100us. */
	schedule();
	restore_flags(flags);
}


int check_disk_change(kdev_t dev)
{
	struct file_operations *fops;
	int i;

	/* check for a check_media_change operation from this device */
	i = MAJOR(dev);
	if (i >= MAX_BLKDEV || (fops = blkdevs[i].fops) == NULL)
		return 0;
	if (fops->check_media_change == NULL)
		return 0;

#ifdef DEBUG_DEVICES
	printk("calling per-device media change routine @ 0x%p\n",
	       fops->check_media_change);
#endif

	sti();
	if (!fops->check_media_change(dev)) {
		return 0;
	}

	printk("VFS: Disk change detected on device %d/%d\n",
	       MAJOR(dev), MINOR(dev));

	if (fops->revalidate) {
		sti();
#ifdef DEBUG_DEVICES
		printk("...calling revalidate routine @ 0x%p\n",
		       fops->revalidate);
#endif
		fops->revalidate(dev);
	}

	/* put things back the way they were and leave */
	return 1;
}

#define DEBUG_DEVICES
void __invalidate_buffers(kdev_t dev, int i)
{
#ifdef DEBUG_DEVICES
	printk("invalidate_buffers called\n");
#endif
}
#undef DEBUG_DEVICES

void sync_dev(kdev_t dev)
{
#ifdef DEBUG_DEVICES
	printk("sync_dev called\n");
#endif
}

/*
 *  Generic read routine for block devices.  We must set up and call the 
 *  request function for this particular device.
 */

ssize_t block_read(struct file *filp, char *buf, size_t count,
		   loff_t * ppos)
{
	int major;
	struct inode *inode = filp->f_dentry->d_inode;


#ifdef DEBUG_DEVICES
	printk("block_read: device 0x%04x, f_pos = %lu\n",
		inode->i_rdev, (unsigned long) filp->f_pos);
#endif

	major = MAJOR(inode->i_rdev);
	if (blk_dev[major].request_fn == NULL) {
		printk("...ERROR: request_fn = NULL\n");
		printk("\tmajor = %d\n", major);
		return 0;
	} else {
		unsigned long read;
		int blocksize, block;
		struct buffer_head *bh;
/*
 *  Work out what the blocksize is.
 */
		blocksize = BLOCK_SIZE;
		if (blksize_size[MAJOR(inode->i_rdev)]) {
			if (blksize_size[MAJOR(inode->i_rdev)]
			    [MINOR(inode->i_rdev)]) blocksize =
				    blksize_size[MAJOR(inode->i_rdev)]
				    [MINOR(inode->i_rdev)];
		}
#ifdef DEBUG_DEVICES
		printk("...block size is %d\n", blocksize);
#endif

/*
 *  We need to read all of the blocks that contain the user's buffer.  We
 *  are at filp->f_pos and the block size is blocksize.
 */
		read = 0;
		while (read < count) {

/*
 *  Which block should we read?
 */
			block = filp->f_pos / blocksize;
/*
 *  Issue the request to read the file a block at a time.
 */
#ifdef DEBUG_DEVICES
			printk
			    ("block_read: issuing request to read block %d (size = %d)\n",
			     block, blocksize);
#endif
			bh = bread(inode->i_rdev, block, blocksize);
			if (!bh) {
/*
 *  Failed, return just how much we managed to read.
 */
				printk
				    ("block_read: read failed (after %lu bytes)\n",
				     read);
				*ppos += read;
				return read;
			} else {
				int size;
/*
 *  Success, so now update the read count and copy over the data into
 *  the request buffer.  We have just read block "block" and filp->f_pos
 *  tells us where we were in this file before we read this block.  
 *  block * blocksize gives us the offset of the first byte we actually have from 
 *  this file.
 */
				if (read == 0) {
					int first;
					int offset;
/*
 *  This is the first block that we've read, (read == 0).
 *  We need only the last part of this block (or all of it).
 */
					first = block * blocksize;
					offset = filp->f_pos - first;
					if (count > blocksize)
						size = blocksize;
					else
						size = count;
					memcpy(buf,
					       (char *) (bh->b_data) +
					       offset, size);
				} else {	/* read != 0 */
/*
 *  nth block read.
 *  We need either all of this block or only the first part of it.
 */
					if ((count - read) > blocksize)
						size = blocksize;
					else
						size = count - read;
					memcpy(buf, (char *) (bh->b_data),
					       size);
				}
				buf += size;
				read += size;
              filp->f_pos += size;

/*
 *  Have we read enough?
 */
				if (read == count) {
					*ppos += read;
					return read;
				}

			}
		}		/* end while not read everything */
		*ppos += read;
		return read;
	}			/* else valid device */
}

ssize_t block_write(struct file * file, const char *buffer, size_t size,
		    loff_t * offset)
{
#ifdef DEBUG_DEVICES
	printk("block_write called\n");
#endif
	return -EACCES;
}

int block_fsync(struct file *file, struct dentry *dir)
{
#ifdef DEBUG_DEVICES
	printk("block_fsync called\n");
#endif
	return 0;
}

int request_dma(unsigned int dmanr, const char * device_id)
{
#ifdef DEBUG_DEVICES
	printk("request_dma: dmanr=%d, device=%s\n",dmanr,device_id);
#endif
	return 0;
}

int __verify_write(unsigned long start, unsigned long size)
{
#ifdef DEBUG_DEVICES
	printk("__verify_write called\n");
#endif
	return 0;
}

void __wait_on_buffer(struct buffer_head *bh)
{
#ifdef DEBUG_DEVICES
	printk("__wait_on_buffer called for bh = 0x%p\n", bh);
#endif

	while (buffer_locked(bh)) {
		run_task_queue(&tq_disk);
		schedule();
		__asm__ __volatile__("":::"memory");
	}

#ifdef DEBUG_DEVICES
	printk("...returning\n");
#endif
}

int verify_area(int type, const void *where, unsigned long size)
{
#ifdef DEBUG_DEVICES
	printk("verify_area: called\n");
#endif
	return 1;
}

static struct buffer_head *bh = NULL;
static char *buf = NULL;

struct buffer_head *getblk(kdev_t dev, int block, int size)
{
#ifdef DEBUG_DEVICES
	printk("getblk: called (dev = 0x%x), size = %d\n", dev, size);
#endif

	/* allocate some memory for it (if we have to). */
	if (bh == NULL) {
		bh =
		    (struct buffer_head *)
		    kmalloc(sizeof(struct buffer_head), 0);
		/* make sure that the buffer is size aligned */
		buf = kmalloc(size * 2, 0) + size;
		buf = (char *) ((unsigned long) buf & ~(size - 1));
#ifdef DEBUG_DEVICES
		printk
		    ("getblk: allocating *new* bh (@ 0x%p) and buf (@ 0x%p)\n",
		     bh, buf);
#endif
	}
	memset((void *) bh, 0, sizeof(struct buffer_head));
	memset((void *) buf, 0, size);

	/* now fill it out */
	bh->b_count = 1;
	bh->b_list = BUF_CLEAN;
	bh->b_flushtime = 0;
	bh->b_dev = dev;
	bh->b_blocknr = block;
	bh->b_size = size;

	bh->b_data = buf;
	bh->b_state = 0;	/* Uptodate, Dirty, Lock, Req, Has_aged */

/*    set_bit(BH_Touched, &bh->b_state); */
/* BH_Touched is deprecated. Don't know if we really need this */

	bh->b_end_io = buffer_end_io;
	bh->b_next = NULL;
	bh->b_prev_free = NULL;
	bh->b_next_free = NULL;
	bh->b_this_page = NULL;
	bh->b_reqnext = NULL;

	return bh;
}

void __brelse(struct buffer_head *buf)
{
#ifdef DEBUG_DEVICES
	printk("brelse: called\n");
#endif

	if (buf->b_count)
		buf->b_count--;
}

/* Added this because the generic SCSI devices need it */
int register_chrdev(unsigned int major, const char *name,
		    struct file_operations *fops)
{
	if (major == 0) {
		for (major = MAX_CHRDEV - 1; major > 0; major--) {
			if (chrdevs[major].fops == NULL) {
				chrdevs[major].name = name;
				chrdevs[major].fops = fops;
				return major;
			}
		}
		return -EBUSY;
	}
	if (major >= MAX_CHRDEV)
		return -EINVAL;
	if (chrdevs[major].fops && chrdevs[major].fops != fops)
		return -EBUSY;
	chrdevs[major].name = name;
	chrdevs[major].fops = fops;
	return 0;
}

int register_blkdev(unsigned int major, const char *name,
		    struct file_operations *fops)
{
#ifdef DEBUG_DEVICES
	printk("register_blkdev: called for %s\n", name);
#endif

	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (blkdevs[major].fops)
		return -EBUSY;
	blkdevs[major].name = name;
	blkdevs[major].fops = fops;
	return 0;
}

int unregister_blkdev(unsigned int major, const char *name)
{
#ifdef DEBUG_DEVICES
	printk("unregister_blkdev: called for %s\n", name);
#endif

	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (!blkdevs[major].fops)
		return -EINVAL;
	if (strcmp(blkdevs[major].name, name))
		return -EINVAL;
	blkdevs[major].name = NULL;
	blkdevs[major].fops = NULL;
	return 0;
}
int unregister_chrdev(unsigned int major, const char *name)
{
#ifdef DEBUG_DEVICES
	printk("unregister_chrdev: called for %s\n", name);
#endif

	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (!chrdevs[major].fops)
		return -EINVAL;
	if (strcmp(chrdevs[major].name, name))
		return -EINVAL;
	chrdevs[major].name = NULL;
	chrdevs[major].fops = NULL;
	return 0;
}

void free_dma(unsigned int dmanr)
{
#ifdef DEBUG_DEVICES
	printk("free_dma: called\n");
#endif
}


/* ------------------------------------------------------------ */


asmlinkage void schedule(void)
{
	unsigned long flags;

#ifdef DEBUG_DEVICES
#if 0
	printk("[s]");
#endif
#endif

	/* drop the ipl for a while so that interrupts can happen */
	save_flags(flags);
	sti();

	/* wait a little while so that things _can_ happen */
	udelay(10);

	/* now see what we have to do */
	if (bh_active & bh_mask)
		do_bottom_half();

	run_task_queue(&tq_scheduler);

	/* put things back the way that they were */
	restore_flags(flags);
}

signed long schedule_timeout(signed long timeout)
{
	/* This may not be enough */
	schedule();
        return timeout < 0 ? 0 : timeout;
}


void refile_buffer(struct buffer_head *buf)
{
#ifdef DEBUG_DEVICES
	printk("refile_buffer(): called with buf  = 0x%p\n", buf);
#endif
}

#undef wake_up
#undef __wake_up

void wake_up(struct wait_queue **p)
{
	struct wait_queue *tmp;
	int *ptr = (int *) p;
	unsigned long flags;

	/* valid parameters? */
	if (!p || !(tmp = *p))
		return;

	save_flags(flags);

#ifdef DEBUG_DEVICES
	printk("wake_up called for queue 0x%p, *p = 0x%p\n", p, *p);
#endif
	cli();
	if (*ptr > 0)
		*ptr = *ptr - 1;
	restore_flags(flags);
}

void __wake_up(struct wait_queue **p, unsigned int mode)
{
	wake_up(p);
}

void __up(struct semaphore *sem)
{
#ifdef DEBUG_DEVICES
	printk("__up(): called for 0x%p\n", sem);
#endif
	atomic_inc_return(&sem->count);
}

void __down(struct semaphore *sem)
{
#ifdef DEBUG_DEVICES
	printk("__down(): called for 0x%p\n", sem);
#endif
	if (atomic_dec_return(&sem->count) < 0) {
		do {
			schedule();
		} while (atomic_read(&sem->count) < 0);
	}
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head *bread(kdev_t dev, int block, int size)
{
	struct buffer_head *bh;

#ifdef DEBUG_DEVICES
	printk("bread(%x, %d, %d): called\n", dev, block, size);
#endif

	/* this is a bit hacky, but someplace during ll_rw_block(), 
	   the ipl gets set low, so always set and reset it during this
	   routine. */
	cli();
	if (!(bh = getblk(dev, block, size))) {
		printk("VFS: bread: READ error on device %d/%d\n",
		       MAJOR(dev), MINOR(dev));
		return NULL;
	}
	if (buffer_uptodate(bh))
		return bh;

	cli();
	ll_rw_block(READ, 1, &bh);
	wait_on_buffer(bh);

	if (buffer_uptodate(bh)) {
		cli();
		return bh;
	}

	brelse(bh);
	cli();
	return NULL;
}

/* Buffer functions */

void unlock_buffer(struct buffer_head *bh)
{
	clear_bit(BH_Lock, &bh->b_state);
	wake_up(&bh->b_wait);
}

void mark_buffer_uptodate(struct buffer_head *bh, int on)
{
	if (on)
		set_bit(BH_Uptodate, &bh->b_state);
	else
		clear_bit(BH_Uptodate, &bh->b_state);
}

void buffer_end_io(struct buffer_head *bh, int uptodate)
{
	mark_buffer_uptodate(bh, uptodate);
	unlock_buffer(bh);
}

/*
 * Called every time a block special file is opened
 */

int blkdev_open(struct inode *inode, struct file *filp)
{
	int i;

#ifdef DEBUG_DEVICES
	printk("blkdev_open: called\n");
#endif

	i = MAJOR(inode->i_rdev);
	if (i >= MAX_BLKDEV || !blkdevs[i].fops)
		return -ENODEV;
	filp->f_op = blkdevs[i].fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode, filp);
	return 0;
}

int blkdev_release(struct inode *inode)
{
	return 0;
}

int fsync_dev(kdev_t dev)
{
#ifdef DEBUG_DEVICES
	printk("fsync_dev(): called\n");
#endif
	return 0;
}

unsigned long __get_free_pages(int mask, unsigned long order)
{
	int size;
	char *ptr;
	int count, pfn;

#ifdef DEBUG_DEVICES
#if 0
	printk("__get_free_pages: called, order = %lu\n", order);
#endif
#endif

	/* the size of memory that we want is based on order */
	size = PAGE_SIZE << order;
	count = size / PAGE_SIZE;
	pfn = allocate_many_pfn(count, TEMPORARY_PAGE);
	ptr = (char *) _PFN_TO_PHYSICAL(pfn);

#ifdef DEBUG_DEVICES
#if 0
	printk("__get_free_pages(): returning 0x%p\n", ptr);
#endif
#endif
	return (unsigned long) ptr;
}

void free_pages(unsigned long addr, unsigned long order)
{
#ifdef DEBUG_DEVICES
	printk("free_pages: called\n");
#endif
}

/* filesystem functions */

void exit_fs(struct task_struct *current)
{
#ifdef DEBUG_DEVICES
	printk("exit_fs() called.\n");
#endif
}

struct file_operations *is_reg_blkdev(unsigned int major)
{
        return blkdevs[major].fops;
}

char *kdevname(kdev_t dev)
{
	static char buffer[32];
	sprintf(buffer, "%02x:%02x", MAJOR(dev), MINOR(dev));
	return buffer;
}

void set_blocksize(kdev_t dev, int size)
{
	if (!blksize_size[MAJOR(dev)])
		return;

	switch (size) {
	default:
		panic("Invalid blocksize passed to set_blocksize");
	case 512:
	case 1024:
	case 2048:
	case 4096:;
	}

	if (blksize_size[MAJOR(dev)][MINOR(dev)] == 0
	    && size == BLOCK_SIZE) {
		blksize_size[MAJOR(dev)][MINOR(dev)] = size;
		return;
	}
	if (blksize_size[MAJOR(dev)][MINOR(dev)] == size)
		return;

	blksize_size[MAJOR(dev)][MINOR(dev)] = size;
}

/*
 * entUna has a different register layout to be reasonably simple. It
 * needs access to all the integer registers (the kernel doesn't use
 * fp-regs), and it needs to have them in order for simpler access.
 *
 * Due to the non-standard register layout (and because we don't want
 * to handle floating-point regs), we disallow user-mode unaligned
 * accesses (we'd need to do "verify_area()" checking, as well as
 * do a full "ret_from_sys_call" return).
 *
 * Oh, btw, we don't handle the "gp" register correctly, but if we fault
 * on a gp-register unaligned load/store, something is _very_ wrong
 * in the kernel anyway..
 */

struct allregs {
	unsigned long regs[32];
	unsigned long ps, pc, gp, a0, a1, a2;
};

asmlinkage void do_entUna(void *va, unsigned long opcode,
			  unsigned long reg, unsigned long a3,
			  unsigned long a4, unsigned long a5,
			  struct allregs regs)
{
	static int cnt = 0;

	if (++cnt < 5) {
		printk("kernel: unaligned trap at %016lx: %p %lx %ld\n",
		       regs.pc - 4, va, opcode, reg);
	}

	/* $16-$18 are PAL-saved, and are offset by 19 entries */
	if (reg >= 16 && reg <= 18)
		reg += 19;
	switch (opcode) {
	case 0x28:		/* ldl */
		*(reg + regs.regs) = get_unaligned((int *) va);
		return;
	case 0x29:		/* ldq */
		*(reg + regs.regs) = get_unaligned((long *) va);
		return;
	case 0x2c:		/* stl */
		put_unaligned(*(reg + regs.regs), (int *) va);
		return;
	case 0x2d:		/* stq */
		put_unaligned(*(reg + regs.regs), (long *) va);
		return;
	}
	printk("Bad unaligned kernel access at %016lx: %p %lx %ld\n",
	       regs.pc, va, opcode, reg);
	panic("do_entUna");
}

/*****************************************************
 *     Random dummies                                *
 *****************************************************/

void rand_initialize_irq(int irq)
{
#ifdef DEBUG_KERNEL
	printk("rand_initialize_irq\n");
#endif
} 

void add_interrupt_randomness(int irq)
{
#ifdef DEBUG_KERNEL
	printk("add_interrupt_randomness\n");
#endif
}

void add_blkdev_randomness(int irq)
{
#ifdef DEBUG_KERNEL
	printk("add_blkdev_randomness\n");
#endif
}

void get_random_bytes(void *buf, int nbytes)
{
	// leave as is,.. random :-)
}
