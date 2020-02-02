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
 *  Main routine for the miniloader.
 *
 *  david.rusling@reo.mts.dec.com
 *
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/elf.h>
#include <linux/kernel_stat.h>

#include <asm/hwrpb.h>
#include <asm/pgtable.h>
#include <asm/core_pyxis.h>

#include "milo.h"
#include "impure.h"
#include "flags.h"
#include "uart.h"
#include "fs.h"

/* FLOPPY_HACK: this means that code will explicitly go and set
 * the floppy to a known state before anything else happens
 */

#ifdef CONFIG_BLK_DEV_FD
#if !defined(MINI_ALPHA_NONAME) && !defined(MINI_ALPHA_AVANTI) 	\
	&& !defined(MINI_ALPHA_SX164)
#define FLOPPY_HACK
#include "floppy.h"
#endif
#else
#undef FLOPPY_HACK
#endif

/*****************************************************************************
 *  Global variables.                                                        *
 *****************************************************************************/

unsigned long est_cycle_freq;	/* For device drivers and so on. */
U64 milo_memory_size = 0;	/* memory allocation */
int ramdisk_size;		/* for initial ramdisk */

unsigned long ptbr = 0;		/* memory mapping */
U64 vptbr = 0;
struct pcb_struct boot_pcb;

void *high_memory = 0;

static unsigned long memory_start = 0;
static unsigned long memory_end = 0;

/*
 *  Someplace to put the arguments to the kernel.
 */
U64 *pkernel_args;
U64 kernel_at;
U64 kernel_entry;
U64 kernel_pages;
U64 kernel_filesize;		/* size of .text and .data section */
U64 kernel_memsize;		/* size of .text, .data, and .bss
				 * section */
/*
 *  Counting stuff.
 */
unsigned int num_l2_pages = 0;
unsigned int num_l3_pages = 0;
struct kernel_stat kstat;

/*
 *  Other Stuff
 */
kernel_cap_t cap_bset = CAP_FULL_SET;

/*****************************************************************************
 *  Macros.                                                                  *
 *****************************************************************************/

#define PASS1_LCA_TYPE	       0x100000004ULL	/* Pass 1 LCA chip detect */
#define vulp volatile unsigned long *
#define HAE_ADDRESS PYXIS_HAE_ADDRESS

/*
 *  External stuff.
 */

extern void __start_again(void);	/* --> head.S when swapping palcode */

extern void entMM(void);	/* Memory fault handler */
extern void entIF(void);
extern void entUna(void);

extern void kfree(void *);
#if defined(MINI_ALPHA_PC164) || defined(MINI_ALPHA_LX164)
extern void SMC93x_Init(void);
#endif

extern struct alpha_machine_vector alpha_mv;

/*
 *  function prototypes.
 */

void __main(void);
void boot_main_cont(void);
// void trap_init(void);

/*
 * The DeskStation Ruffian motherboard firmware does not place
 * the memory size in the PALimpure area.  Therefore, we use
 * the Bank Configuration Registers in PYXIS to obtain the size.
 */

#ifdef MINI_ALPHA_RUFFIAN
static unsigned long ruffian_get_bank_size(unsigned long offset)
{
	unsigned long bank_addr, bank, ret = 0;

	/* Valid offsets are: 0x800, 0x840 and 0x880
	   since Ruffian only uses three banks.  */

	bank_addr = (unsigned long) PYXIS_MCR + offset;
	bank = *(vulp) bank_addr;

	/* Check BANK_ENABLE */
	if (bank & 0x01) {
		static unsigned long size[] = {
			0x40000000UL,	/* 0x00,   1G */
			0x20000000UL,	/* 0x02, 512M */
			0x10000000UL,	/* 0x04, 256M */
			0x08000000UL,	/* 0x06, 128M */
			0x04000000UL,	/* 0x08,  64M */
			0x02000000UL,	/* 0x0a,  32M */
			0x01000000UL,	/* 0x0c,  16M */
			0x00800000UL,	/* 0x0e,   8M */
			0x80000000UL,	/* 0x10,   2G */
		};

		bank = (bank & 0x1e) >> 1;
		if (bank < sizeof(size) / sizeof(*size))
			ret = size[bank];
	}

	return ret;
}
#endif

#ifdef FLOPPY_HACK
static void fdasleep(int nmsec)
{
	int i;

	while (nmsec--) {
		i = 50000;
		do {
		} while (--i);
	}
}

static int fdasts(void)
{
	int nsts;
	int byte;
	char sts[NSTS];

#ifdef DEBUG_BOOT
	int i;
#endif

	nsts = 0;
	for (;;) {
		while (((byte = inb(FDAMSR)) & MSRRQM) == 0);
		if ((byte & MSRDIO) == 0)
			break;
		byte = inb(FDADR);
		if (nsts < NSTS)
			sts[nsts++] = byte;
	}

#if defined(DEBUG_BOOT)
	printk("sts:");
	for (i = 0; i < nsts; ++i)
		printk(" %02x", sts[i] & 0xFF);
	printk("\n");
#endif

	if ((sts[0] & ST0IC) != ST0NT) {
		return (0);
	}
	return (1);
}

static void fdacmd(char cmd[], int ncmd)
{
	int i;

#if defined(DEBUG_BOOT)
	printk("cmd:");
	for (i = 0; i < ncmd; ++i)
		printk(" %02x", cmd[i] & 0xFF);
	printk("\n");
#endif

	for (i = 0; i < ncmd; ++i) {
		while ((inb(FDAMSR) & (MSRRQM | MSRDIO)) != MSRRQM);
		outb(cmd[i], FDADR);
	}
}


static void floppy_init_hack(void)
{
	char cmd[1];

#if defined(DEBUG_BOOT)
	printk("hack alert! Initialising the floppy\n");
#endif

	outb(DCRENAB | DCRNRES, FDADCR);
	fdasleep(50);
	outb(DRR250, FDADRR);
	cmd[0] = NSINTS;
	fdacmd(cmd, 1);
	(void) fdasts();
	cmd[0] = NSPEC;
	cmd[1] = SRT | HUT;
	cmd[2] = HLT | ND;
	fdacmd(cmd, 3);
}

#endif

/*****************************************************************************
 *  Swap to a new PALcode                                                    *
 *****************************************************************************/
void swap_to_palcode(void *palbase, void *pc, void *ksp, U64 new_ptbr,
		     void *new_vptbr)
{
	extern void swppal(void *, void *, struct pcb_struct *, void *);

/*
 *  Set up the initial pcb.
 */

	boot_pcb.ksp = (U64) ksp;
	boot_pcb.usp = 0;
	boot_pcb.ptbr = new_ptbr;
	boot_pcb.pcc = 0;
	boot_pcb.asn = 0;
	boot_pcb.unique = 0;
	boot_pcb.flags = 1;

/*
 *  Call swppal.  The arguments are:
 *
 *  a0:      PAL address
 *  a1:      PC
 *  a2:      pcb
 *  a3:      vptbr
 */
	swppal(palbase, pc, &boot_pcb, new_vptbr);
}

/*****************************************************************************
 *  Kernel Loading code                                                      *
 *****************************************************************************/
/*
 *  Extract the interesting info from an ECOFF or ELF image.
 */
static long extract_header_info(char *header)
{
	struct elfhdr *elf;
	struct elf_phdr *phdr;
	long offset;

	elf = (struct elfhdr *) header;

	/* is at an elf object file? */
	if (elf->e_ident[0] != 0x7f
	    || strncmp(elf->e_ident + 1, "ELF", 3) != 0) {
		printk("Not an ELF file\n");
		return -1;
	}
	/* looks like an ELF binary: */
	if (elf->e_type != ET_EXEC) {
		printk("Not an executable ELF file\n");
		return -1;
	}
	if (!elf_check_arch(elf->e_machine)) {
		printk("ELF executable not for this machine\n");
		return -1;
	}
	if (elf->e_phnum != 1) {
		printk("Expected 1, not %d program headers\n",
		       elf->e_phnum);
		return -1;
	}
	if (elf->e_phoff + sizeof(*phdr) > IOBUF_SIZE) {
		printk("ELF program header not in first block (%ld)\n",
		       (long) elf->e_phoff);
		return -1;
	}
	phdr = (struct elf_phdr *) (header + elf->e_phoff);

	kernel_entry = elf->e_entry;
	kernel_at = phdr->p_vaddr;
	offset = phdr->p_offset;
	kernel_filesize = phdr->p_filesz;
	kernel_memsize = phdr->p_memsz;

#ifdef DEBUG_BOOT
	printk("   entry: %lx\n", kernel_entry);
	printk("   text_start: %lx, ", kernel_at);
	printk("   filesize: %lx\n", kernel_filesize);
	printk("   memsize: %lx\n", kernel_memsize);
	printk("   offset: %lx\n", offset);
#endif

	/* work around ELF binutils bug: */
	/* DO WE STILL NEED THIS?? */

	if (kernel_at < kernel_entry) {
		unsigned long delta = kernel_entry - kernel_at;

		offset += delta;
		kernel_filesize -= delta;
		kernel_memsize -= delta;
		kernel_at = kernel_entry;
#ifdef DEBUG_BOOT
		printk("   adjusted text_start: %lx, ", kernel_at);
		printk("   adjusted filesize: %lx\n", kernel_filesize);
		printk("   adjusted memsize: %lx\n", kernel_memsize);
		printk("   adjusted offset: %lx\n", offset);
#endif
	}
#ifdef DEBUG_BOOT
	printk("   Text+Data: 0x%lx - 0x%lx\n",
	       kernel_at, kernel_at + kernel_filesize - 1);
	printk("   BSS: 0x%lx - 0x%lx\n",
	       kernel_at + kernel_filesize,
	       kernel_at + kernel_memsize - 1);
	printk("   Entry: 0x%lx\n", kernel_entry);
#endif
	return offset;
}

/*
 *  Returns the entry point of the kernel, or zero if kernel fails to load.
 */

U64 load_image_from_memory(char *address, unsigned int pages)
{
	long offset;

#ifdef DEBUG_BOOT
	printk("Loading image from memory (at 0x%p)\n", address);
#endif

	/* check the kernel header */
	offset = extract_header_info((char *) address);
	if (offset < 0)
		return 0;

	/* How many pages does the kernel take up?  Don't forget to include the
	 * pages between KERNEL_START and START_ADDR 
	 */

	kernel_pages = (kernel_memsize + PAGE_SIZE - 1) >> PAGE_SHIFT;
	kernel_pages +=
	    ((kernel_entry - KERNEL_START + PAGE_SIZE - 1) >> PAGE_SHIFT);

#ifdef DEBUG_BOOT
	printk("Kernel uses %ld page(s)\n", kernel_pages);
#endif

/*
 *  Copy the kernel.
 */
#ifdef DEBUG_BOOT
	printk("...copying the Kernel into physical memory @ 0x%lx\n",
	       kernel_at);
#endif

	memcpy((char *) kernel_at, address + offset, kernel_filesize);
/*
 *  Zero the bss part of the image.
 */
#ifdef DEBUG_BOOT
	printk("...clearing bss memory between 0x%p and 0x%p\n",
	       (char *) kernel_at + kernel_filesize,
	       (char *) kernel_at + kernel_memsize);
#endif
//	memset((char *) kernel_at + kernel_filesize, 0,
//	       kernel_memsize - kernel_filesize);
#ifdef DEBUG_BOOT
	printk("... done.\n");	
#endif
	return kernel_entry;
}

/*
 *  Returns the entry point of the kernel, or zero if kernel fails to load.
 */
U64 load_image_from_device(char *fs_type, char *device, char *file)
{
	int channel;
	int fd;
	int blkno;

	char *iobuf;
	char *where;

/*
 * We must read an image from a device
 */

#ifdef DEBUG_BOOT
	printk("Searching for %s on device %s\n", file, device);
#endif

	channel = device_mount(device, fs_type);

	if (channel)
		return 0;

/*
 *  Look for the file
 */
	fd = __open(file);
#ifdef DEBUG_BOOT
	printk("...%s\n", fd >= 0 ? "found it" : "failed to find it");
#endif

	if (fd < 0)
		return (U64) NULL;

/*
 * First, try and load it as if it were a zipped image.
 */
	where = (char *) kmalloc(MAX_KERNEL_SIZE + 512, 0) + 512;
	if (!where) {
		printk
		    ("ERROR: failed to allocate memory to hold the uncompressed kernel\n");
		return (U64) NULL;
	}
#ifdef DEBUG_BOOT
	printk("..trying to uncompress image into 0x%p\n", where);
#endif
	if (uncompress_kernel(fd, where) < 0) {
		char *ptr;

		/* no, it wasn't a compressed kernel, carry on as normal
		   Allocate an iobuf for the transfers and read the first block of the
		   file into it (the section headers).  Make it 512 bytes aligned. */
		iobuf = kmalloc(IOBUF_SIZE + 512, 0) + 512;
		iobuf = (char *) ((unsigned long) iobuf & (~(512 - 1)));
#ifdef DEBUG_BOOT
		printk("...allocated iobuf at 0x%p\n", iobuf);
#endif
		ptr = where;
		blkno = 0;	/* logical block number */
		while (__bread(fd, blkno++, iobuf) == 0) {
			printk("#");
			memcpy((char *) ptr, iobuf, IOBUF_SIZE);
			ptr += IOBUF_SIZE;
			if ((ptr - where) > MAX_KERNEL_SIZE)
				break;
		}

		/* free off the iobuf */
		kfree(iobuf);

		/* close the device */
		__close(channel);
	}

	/* ok, now load the kernel image from wherever we allocated memory */
	kernel_entry =
		load_image_from_memory((char *) where,
				   _PHYSICAL_TO_PFN(MAX_KERNEL_SIZE));
#ifdef DEBUG_BOOT
	printk("... Image loaded.\n");	
#endif
	/* free off the allocated memory */
	kfree(where);

	/* tell the world (maybe) */
	if (milo_verbose)
		printk("...Image loaded into memory, entry @ 0x%lx\n",
		       kernel_entry);

	return kernel_entry;
}


/*****************************************************************************
 *  Virtual address mapping (page table) code.                               *
 *****************************************************************************/
/*
 *  Given all of the information, build a valid PTE.
 */
static pte_t build_pte(unsigned long pfn, pgprot_t protection)
{
	pte_t pte;

	pte_val(pte) = pte_val(mk_pte(_PFN_TO_PHYSICAL(pfn), protection));
	pte_val(pte) = pte_val(pte) | _PAGE_VALID;

#ifdef DEBUG_BOOT
	printk("build_pte: pte = 0x%lX,0x%lX\n", pte_val(pte) >> 32,
	       pte_val(pte));
#endif

	return pte;
}

/*
 *  Given a VA, make a set of page table entries for it (if they
 *  don't already exist).
 *
 *  Args:
 *
 *  va            virtual address to add page table entries for.
 *  vpfn          the PFN that the virtual address is to be mapped to.
 *  protection    the protection to set the L3 page table entry to (all L1 and
 *                L2 pages are marked as Kernel read/write).
 */

void add_VA(U64 va, unsigned int vpfn, unsigned int protection)
{
	U64 l1, l2, l3;
	unsigned long pfn, newpfn;
	U64 pa;
	pte_t pte;

/*
 *  Dismantle the VA.
 */

	l1 = 0;
	l2 = (va & 0xff800000) >> 20;
	l3 = (va & 0x007fe000) >> 10;

#ifdef DEBUG_BOOT
	printk("...Adding PTEs for virtual address 0x%lX, pfn = %X\n", va,
	       vpfn);
	printk
	    ("...L1 offset = 0x%lx, L2 offset = 0x%lx, L3 offset = 0x%lx\n",
	     l1, l2, l3);
#endif

/*
 *  Now build a set of entries for it (if they do not already
 *  exist).  In the next blocks of code, pfn is always the pfn of
 *  the current entry and newpfn is the pfn that is referenced from
 *  it.
 */

/*
 *  L1.  The L1 PT PFN is held in the global integer ptbr. 
 */
	pa = _PFN_TO_PHYSICAL(ptbr) + l1;

	if (ReadQ(pa) == 0) {
		newpfn = _ALLOCATE_PFN();	/* for L2 */
		num_l2_pages++;

#ifdef DEBUG_BOOT
		printk
		    ("\tAllocated L2 page table at PFN 0x%lx (physical = 0x%lx)\n",
		     newpfn, _PFN_TO_PHYSICAL(newpfn));
#endif
		zeropage_phys(newpfn);
		pte = build_pte(newpfn, __pgprot(_PAGE_KWE | _PAGE_KRE));
		WriteQ(pa, pte_val(pte));
	} else
		pte_val(pte) = ReadQ(pa);

#ifdef DEBUG_BOOT
	printk("L1 PTE at PFN(0x%lx) + 0x%04lX = 0x%lX\n", ptbr, l1,
	       ReadQ(pa));
#endif
	pfn = _PHYSICAL_TO_PFN(pte_page(pte));

/*
 *  L2, pfn inherited from L1 entry (which may have been allocated
 *  above).
 */
	pa = _PFN_TO_PHYSICAL(pfn) + l2;
	if (ReadQ(pa) == 0) {
		num_l3_pages++;
		newpfn = _ALLOCATE_PFN();
#ifdef DEBUG_BOOT
		printk
		    ("\tAllocated L3 page table at PFN 0x%08lX (physical = 0x%lX)\n",
		     newpfn, _PFN_TO_PHYSICAL(newpfn));
#endif
		zeropage_phys(newpfn);
		pte = build_pte(newpfn, __pgprot(_PAGE_KWE | _PAGE_KRE));
		WriteQ(pa, pte_val(pte));
	} else
		pte_val(pte) = ReadQ(pa);

#ifdef DEBUG_BOOT
	printk("L2 PTE at PFN(0x%lx) + 0x%04lX = 0x%08lX\n", pfn, l2,
	       ReadQ(pa));
#endif
	pfn = _PHYSICAL_TO_PFN(pte_page((pte_t) pte));

/*
 *  L3, pfn inherited from L2 entry above.  vpfn contains the page frame
 *  number of the real thing whose PTEs we have just invented above.
 */
	pa = _PFN_TO_PHYSICAL(pfn) + l3;
	if (ReadQ(pa) == 0) {
		pte = build_pte(vpfn, __pgprot(protection));
		WriteQ(pa, pte_val(pte));
	} else
		pte_val(pte) = ReadQ(pa);

#ifdef DEBUG_BOOT
	printk("L3 PTE at PFN(0x%lx) + 0x%04lX = 0x%08lX\n", pfn, l3,
	       ReadQ(pa));
#endif

}

/*****************************************************************************
 *  System specific routine that returns the top of memory (ie the           *
 *  amount of physical memory in the system).                                *
 *****************************************************************************/

#define CSERVE_K_RD_IMPURE     0x0B

static U64 memory_size(void)
{
	U64 size = 0;

#ifndef MINI_ALPHA_RUFFIAN
	U64 PalImpureBase;
#endif

/* 
 * The MEMORY_SIZE environment variable will override all else 
 */
	size = atoi(getenv("MEMORY_SIZE"));
	if (size > 0) {

		/* convert from Mbytes to bytes */
		size = size * 1024 * 1024;
	}

/*
 *  Try and work out the size from the PALcode impure area.
 */

/*
 *  HACK ALERT! EB66 mem size is at 368 and the EB64+ is at 350
 */
#ifdef MINI_ALPHA_EB64
	size = 0x2000000;
#else

#if defined(MINI_ALPHA_RUFFIAN)
	/* RUFFIAN doesn't save the memory size in the impure area... */
	size = ruffian_get_bank_size(0x0800UL);
	size += ruffian_get_bank_size(0x0840UL);
	size += ruffian_get_bank_size(0x0880UL);
#else				/* RUFFIAN */

	PalImpureBase =
	    (U64) (CNS_Q_BASE + cServe(0, 0, CSERVE_K_RD_IMPURE));
	if ((ReadQ(PalImpureBase + CNS_Q_SIGNATURE) & 0xffffffffUL)
	    == IMPURE_SIGNATURE) {
		size = ReadQ(PalImpureBase + CNS_Q_MEM_SIZE);
	}
#endif				/* RUFFIAN */

#endif

	if (size == 0) {
#if defined(MINI_ALPHA_NONAME)
#ifdef DEBUG_BOOT
		printk
		    ("Failed to determine memory size from PAL impure area - using size of 24MB\n");
#endif
		size = 0x1800000;
#else				/* NONAME/UDB */
#ifdef DEBUG_BOOT
		printk
		    ("Failed to determine memory size from PAL impure area - using size of 32MB\n");
#endif
		size = 0x2000000;
#endif				/* NONAME/UDB */
	}
#ifdef DEBUG_BOOT
	printk("memory_size(), returning %ld\n", size);
#endif

	return size;
}

/*****************************************************************************
 *  Build the Page Tables                                                    *
 *****************************************************************************/
/*
 *  We are running with mapping 1-to-1 physical.  Before we can turn on memory
 *  mapping we must set up the PTEs that we need.
 *
 *  We need memory allocation/mapping set up before we can do this.
 */
static void build_mm(void)
{
	pte_t pte;

#if defined(DEBUG_BOOT)
	printk("Building the Page Tables\n");
#endif

	/* use the same pfn that Linux will use later (SWAPPER_PGD (0x300000)).
	 * This is the first page in the kernel (KERNEL_START).  It saves us
	 * allocating a page and then telling Linux that it isn't available. */

	ptbr = _PHYSICAL_TO_PFN(SWAPPER_PGD);
	zeropage_phys(ptbr);
#if defined(DEBUG_BOOT)
	printk("...L1 PT (ptbr) at PFN 0x%lx, physical address 0x%lx\n",
	       ptbr, _PFN_TO_PHYSICAL(ptbr));
#endif
/*
 *  The virtual page table base pointer is used as a shortcut to get to the
 *  L3 entry for any given faulting address.  It is a special entry in the L1
 *  page table.  This part is important.   Linux 1.2 sets up the vptbr so that
 *  the *last* entry in the L1 PT contains the L1 pfn.  I honour that here.
 *  Change this at your peril.
 */
#if defined(DEBUG_BOOT)
	printk("...setting up the virtual page table base register\n");
#endif
	vptbr = 0xfffffffe00000000UL;
	pte = build_pte(ptbr, __pgprot(_PAGE_KWE | _PAGE_KRE));
	WriteQ(_PFN_TO_PHYSICAL(ptbr) + (sizeof(pte) * 0x3ff),
	       pte_val(pte));

#if 0

/*
 *  Just for now, add the debug monitor and this code in as virtual addresses
 *  matching their physical addresses.
 */

	do {
		unsigned long va;
		int pfn;

		printk("...Temporarily mapping in debug monitor\n");
		va = 0;
		pfn = 0;
		while (va < ((LOADER_AT + LOADER_SIZE) & 0xffffffff)) {
			add_VA(va, pfn, (_PAGE_KWE | _PAGE_KRE));
			pfn++;
			va += PAGE_SIZE;
		}
	} while (0);
#endif
}

/*****************************************************************************
 *  Utility routines                                                         *
 *****************************************************************************/

static char *devnames[] = {
	"hda", "hdb", "hdc", "hdd",
	"sda", "sdb", "sdc", "sdd", "sde",
	"fd", "xda", "xdb", "sr", "scd",
	"sdf", "sdg", "sdh",
	"floppy", "cdrom", NULL
};

static unsigned int devnums[] = {
	0x300, 0x340, 0x1600, 0x1640,
	0x800, 0x810, 0x820, 0x830, 0x840,
	0x200, 0xD00, 0xD40, 0x0B00, 0x0B00,
	0x850, 0x860, 0x870,
	0x200, 0x0B00, 0x0
};

char *dev_name(unsigned int device)
{
	static char name[32];
	int number, i, len;

#ifdef DEBUG_BOOT
	printk("parse_device: device is 0x%04x\n", device);
#endif

	/* drop of the trailing number */
	number = device & 0xf;
	device = device & 0xfff0;

	/* look for the device */
	for (i = 0; devnames[i] != NULL; i++) {
		if (device == devnums[i]) {

			/* we've found it */
			strcpy(name, devnames[i]);

			len = strlen(name);
			name[len] = '0' + number;
			name[len + 1] = '\0';
#ifdef DEBUG_BOOT
			printk("...returning %s\n", name);
#endif
			return name;
		}
	}

	strcpy(name, "void");
#ifdef DEBUG_BOOT
	printk("...returning %s\n", name);
#endif
	return name;
}

int device_name_to_number(const char *name)
{
	int i;
	int number, len, dev;

	if (!strcmp(name, "disk") || !strcmp(name, "disc")) {
		name = getenv("BOOT_DEV");
		if (!name)
			name = MILO_BOOT_DEV;
	}

	/* get the true length of the name */
	len = strlen(name);
	while (len > 0 && ('0' <= name[len - 1] && name[len - 1] <= '9'))
		--len;
	if (name[len] != 0)
		number = atoi(name + len);
	else
		number = 0;

	/* compare it to the list of devices */
	for (i = 0; devnames[i] != NULL; i++) {
		if (strncmp(devnames[i], name, len) == 0) {

			/* we've found it */
			dev = devnums[i] + number;
#ifdef DEBUG_BOOT
			printk("parse_device_name()...returning 0x%04x\n",
			       dev);
#endif
			return dev;
		}
	}

	if (milo_verbose)
		printk("MILO: unknown device %s\n", name);
	return -1;
}

/*****************************************************************************
 *   Main entry point for system primary bootstrap code                      *
 *****************************************************************************/

/* 
 * At this point we have been loaded into memory at LOADER_AT and we are running in Kernel
 * mode. The Kernel stack base is at the top of memory (wherever that is).  Assume that it
 * takes up two 8K pages. We are running 1-to-1 memory mapping (ie physical).
 */

void __main(void)
{

/*****************************************************************************
 *  Initialize.                                                              *
 *****************************************************************************/

/*
 *  Copy the verbosity flag into a separate variable to simplify checks.
 */
	milo_verbose = (milo_global_flags & VERBOSE_FLAG) != 0;

/*
 *  Initialize the Evaulation Board environment.
 */

	/* let's get explicit about some variables we care about. */
	num_l2_pages = num_l3_pages = 0;

#if !defined(MINI_ALPHA_RUFFIAN)
	/* not sure about this, their source still has it active... ??? */
	/* This will not work, since there's no mv yet?!? */
	set_hae(alpha_mv.hae_cache);
#endif				/* !RUFFIAN */

#if defined(MINI_ALPHA_PC164) || defined(MINI_ALPHA_LX164)
/*
 *  The PC164/LX164 employ the SMC FDC37C93X Ultra I/O Controller.
 *  After reset all of the devices are disabled. SMC93x_Init() will
 *  autodetect the FDC37C93X and enable each of the devices.
 */
	SMC93x_Init();
#endif

#ifdef MINI_SERIAL_ECHO
	/* we can only output to the serial port *after* we initialise it */
	uart_init();
#endif

#ifdef FLOPPY_HACK
	floppy_init_hack();	/* hack */
#endif

	/* change the sp to be under the palcode minus a quad */
	wrsp(PALCODE_AT - 1024);

#if !defined(MINI_ALPHA_RUFFIAN)
	wrfen();
#endif				/* !RUFFIAN */

	milo_memory_size = (U64) high_memory = memory_size();

#if defined(DEBUG_BOOT)
	printk("Memory size is 0x%lx\n", milo_memory_size);
#endif

/*
 *  Raise the IPL so that no interrupts can occur
 */

	ipl(7);

/*****************************************************************************
 *  Build the basic HWRPB (needed by lots of stuff)                          *
 *****************************************************************************/
	init_HWRPB();

/*****************************************************************************
 *  Initialise the memory map                                                *
 *****************************************************************************/
	init_mem(milo_memory_size);

/*****************************************************************************
 *  Building the page tables.                                                *
 *****************************************************************************/

	/* we cannot do this until we've set up the memory mapping and are able to 
	 * allocate memory for page tables */
	build_mm();

/*****************************************************************************
 *  Swap to the new PALcode                                                  *
 *****************************************************************************/

	/* if we're trying to debug the code, then we cannot swap to a new PALcode.
	 *   So, we just carry straight on.  We'll swap to the new PALcode when we 
	 * start up Linux */

#if defined(DEBUG_USE_DBM) || defined(MINI_ALPHA_MIKASA)
	boot_main_cont();
#else

	/* don't understand why, but on MIKASA this crashes us, so we avoid it */

	/* swap to the PALcode in this image, but stay in this image and make it
	 * 1-to-1 virtual to physical address mapping */

#if defined(DEBUG_BOOT)
	printk("Swapping to new PALcode...\n");
#endif

	swap_to_palcode((void *) PALCODE_AT, (void *) __start_again,
			(void *) (INIT_STACK + PAGE_SIZE), 0, (void *) 1);
#endif
}

/*****************************************************************************
 *   Secondary entry point for system primary bootstrap code                 *
 *****************************************************************************/

/* 
 *  At this point we have set up the memory map and swapped to the new PALcode.
 *  VGA is initialised and ipl is at 7.
 */

void boot_main_cont(void)
{
/*****************************************************************************
 *  Do any CPU specific initialisation required                              *
 *****************************************************************************/

	/* Tell PAL-code what global pointer we want in the kernel. */

	register unsigned long gptr __asm__("$29");

	wrkgp(gptr);

	/* enable memory fault handler */
	wrent(entMM, 2);

	/* we need the hwrpb set up before we can do this */
	/* This gets us keyboard and VGA (I hope) */
	milo_cpu_init();

	/* Initialize the machine. Usually has to do with 
	   setting DMA windows and the like.  */
	if (alpha_mv.init_arch)
		alpha_mv.init_arch(&memory_start, &memory_end);

//	trap_init();

/*****************************************************************************
 *  Startup any device drivers required                                      *
 *****************************************************************************/
	/* we need the hwrpb set up before we can do this */
	device_init();

/*****************************************************************************
 *  Initialise the environment variable stuff                                *
 *****************************************************************************/
	env_init();

/*****************************************************************************
 *  Startup the keyboard so that we can get some input.                      *
 *****************************************************************************/
	milo();

/*
 *  If we ever get to this point, something has gone seriously wrong!!!
 */

#ifdef NEVER
	printk("If you can read this, something went VERY wrong!\n");
	while (1);
#endif
}

void load_ramdisk_from_device(char *fs_type, char *device, char *file)
{
	int channel, fd, blkno, ramdisk_num_pages;
	char *where, *ptr;
	long initdisk_size;
/*
 * We must read an image from a device
 */

	*(unsigned long *) (ZERO_PGE + 0x100) = 0;
	*(unsigned long *) (ZERO_PGE + 0x108) = 0;

#ifdef DEBUG_BOOT
	printk("Searching for %s on device %s\n", file, device);
#endif

	if ((channel = device_mount(device, fs_type)))
		return;

/*
 *  Look for the file
 */
	fd = __open(file);
#ifdef DEBUG_BOOT
	printk("...%s\n", fd >= 0 ? "found it" : "failed to find it");
#endif

	if (fd < 0)
		return;
	initdisk_size = __fdsize(fd);

	where = (char *) (LOADER_AT - initdisk_size - IOBUF_SIZE);
	if (where < (char *) (kernel_at + kernel_memsize)) {
		printk("initdisk too large, would overwrite kernel.\n");
		printk("aborting.\n");
	}
	ramdisk_num_pages = (initdisk_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	mark_many_pfn(_PHYSICAL_TO_PFN(where),
		      ramdisk_num_pages, ALLOCATED_PAGE);

#ifdef DEBUG_BOOT
	printk("..trying to load compressed ramkdisk image into 0x%p\n",
	       where);
	printk("..trying to load compressed ramkdisk end   at   0x%p\n",
	       where + initdisk_size);
#endif

	ptr = where;
	blkno = 0;		/* logical block number */
	while (1) {
		__bread(fd, blkno++, ptr);
//		printk("#");
		ptr += IOBUF_SIZE;
		if ((ptr - where) > initdisk_size)
			break;
	}

	__close(channel);	/* close the device */

	/* tell the world (maybe) */
	if (milo_verbose) {
		printk
		    ("...Ramdisk Image loaded into memory, starting @ 0x%p\n",
		     where);
		printk("...Ramdisk Image size is  0x%lx\n", initdisk_size);
	}

	*(unsigned long *) (ZERO_PGE + 0x100) = (unsigned long) where;
	*(unsigned long *) (ZERO_PGE + 0x108) = initdisk_size;
}

//void trap_init(void)
//{
//	/* Tell PAL-code what global pointer we want in the kernel. */

//	register unsigned long gptr __asm__("$29");

//	wrkgp(gptr);

//	wrent(entMM, 2);
//	wrent(entIF, 3);
//	wrent(entUna, 4);
//}

