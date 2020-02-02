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
 *  HWRPB handling/setup code.
 *
 *  david.rusling@reo.mts.dec.com
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/mc146818rtc.h>

#include <asm/io.h>
#include <asm/hwrpb.h>

#define __EXTERN_INLINE
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#undef __EXTERN_INLINE

#include "milo.h"
#include "impure.h"
#include "uart.h"
#include "version.h"

int print_cpuinfo(void);

static inline __u32 rpcc(void)
{
	__u32 result;
	asm volatile ("rpcc %0" : "=r"(result));
	return result;
}

/*
 *  HWRPB
 */
struct hwrpb_struct *hwrpb;
unsigned long last_asn;


/*
 *  Milo reboot block
 */
Milo_reboot_t *milo_reboot_block = NULL;

/*****************************************************************************
 *  Macros.                                                                  *
 *****************************************************************************/
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Macro for exception fixup code to access integer registers.  */
#define dpf_reg(r)                                                      \
        (((unsigned long *)regs)[(r) <= 8 ? (r) : (r) <= 15 ? (r)-16 :  \
                                 (r) <= 18 ? (r)+8 : (r)-10])


#define PASS1_LCA_TYPE	       0x100000004ULL
					/* Pass 1 LCA chip detect */
/*
 *  External stuff.
 */
extern U64 milo_memory_size;
extern char *milo_memory_map;
extern volatile unsigned int firstpfn;
extern volatile unsigned int lastpfn;

extern unsigned search_exception_table(unsigned long addr);

#define CSERVE_K_RD_IMPURE     0x0B

/*****************************************************************************
 *  static inlines                                                           *
 *****************************************************************************/
static inline unsigned int cpuid(void)
{

	/*  Call two nasty little assembler routines to work out what sort of CPU  
	 * this.  This is the table:         Implver     AMASK 3  ev-4            0
	 *          3           -45           0          3          lca            
	 * 0          3          -45           0          3          ev-5          
	 *  1          3         ev-56           1          2          ev-6        
	 *    2          0       */
#undef amask
	extern unsigned int amask(void);
	unsigned long a, ver;
	unsigned int type = 0;

	a = amask();
	ver = implver();

	if (ver == IMPLVER_EV4) {

		/* ev-4,ev-45, lca, lca-45 */
#ifdef DC21066
		type = LCA4_CPU;
#endif
#ifdef DC21064
		type = EV4_CPU;
#endif
	} else {

		/* ev-5, ev56, pca56, ev-6 */
		switch (a) {
		case 3:
			type = EV5_CPU;
			break;
		case 2:
			type = PCA56_CPU;
			break;
		case 258:
			type = EV56_CPU;
			break;
		case 0:
			type = EV6_CPU;
			break;
		}
	}
	return type;
}

/*****************************************************************************
 *  Set up the boot parameters                                               *
 *****************************************************************************/
/* returns true if we are rebooting */
static int setup_boot_arguments(void)
{

	/* the first place that we look is the reboot block.  This blank when we
	 * first boot but is written to by Milo as each boot (attempt) is made.
	 * That way when we issue "shutdown -r now" and return to Milo, we know
	 * what to do by default */

	int rebooting = FALSE;
	int milo_memory_mb;

	if (milo_verbose)
		printk("Looking for reboot block at 0x%p\n",
		       milo_reboot_block);

	/* first zero out the ZERO_PGE */
	memset((void *) ZERO_PGE, 0, 1024);

	if (hwrpb->id == HWRPB_SIGNATURE) {
		if (milo_verbose)
			printk("HWRPB signature is valid\n");

		/* The HWRPB _may_ be valid */
		if ((milo_reboot_block->signature1 == REBOOT_SIGNATURE) &&
		    (milo_reboot_block->signature2 == REBOOT_SIGNATURE)) {
			printk("Valid Milo reboot block found\n");
			rebooting = TRUE;
#if 0
		} else {	/* go through the HWRPB and count pages */
			unsigned long num_pages = 0;
			struct memdesc_struct *memdesc =
			    (struct memdesc_struct *)
			    ((char *) hwrpb + hwrpb->mddt_offset);
			struct memclust_struct *cluster = memdesc->cluster;
			int count = memdesc->numclusters;

			do
				num_pages += cluster++->numpages;
			while (--count);
			if (milo_verbose)
				printk
				    ("Memory detected from HWRPB: %lu pages\n",
				     num_pages);
			milo_memory_size = num_pages * hwrpb->pagesize;
#endif
		}
	}

	if (rebooting) {

		/* is there a valid memory saved in the Milo reboot block? */
		if (milo_reboot_block->memory_size != 0) {
			printk
			    ("Setting memory size to %d Mbytes from reboot parameter block\n",
			     milo_reboot_block->memory_size);
			milo_memory_size =
			    milo_reboot_block->memory_size * 1024 * 1024;
		}
	} else {

		/* No valid reboot block, but we may have been passed important
		 * information from WNT ARC or parameter block */

		/* zero out the HWRPB and associated data structures */
		zeropage_phys(_PHYSICAL_TO_PFN(hwrpb));

		/* initialise the Milo reboot block */
		milo_reboot_block->signature1 =
		    milo_reboot_block->signature2 = REBOOT_SIGNATURE;
		milo_reboot_block->flags = 0;
		milo_reboot_block->memory_size = 0;

		/* copy any WNT arguments into the ZERO_PGE */
		memcpy((void *) ZERO_PGE, (void *) (PALCODE_AT - 1024),
		       1024);

		/* check for the memory size being passed.   Linload v1.2 passes it at 
		 * +512 bytes and v1.3 passes it at +768 bytes */
		if (strncmp((char *) (ZERO_PGE + 768), "MILO", 4) == 0) {
			unsigned int *sptr = (int *) (ZERO_PGE + 768 + 4);

			milo_memory_mb = *sptr;
			milo_memory_size = milo_memory_mb * 1024 * 1024;
			if (milo_verbose)
				printk("...memory size is 0x%lx (%d MB)\n",
				       milo_memory_size, milo_memory_mb);

			/* now load up the reboot parameter block */
			milo_reboot_block->memory_size = milo_memory_mb;
			milo_reboot_block->flags |= REBOOT_CONSOLE;
		} else {
			if (strncmp((char *) (ZERO_PGE + 512), "MILO", 4)
			    == 0) {
				unsigned char *sptr =
				    (char *) (ZERO_PGE + 512 + 4);

				milo_memory_mb = *sptr;
				milo_memory_size =
				    milo_memory_mb * 1024 * 1024;
				if (milo_verbose)
					printk
					    ("...memory size (via ARC(v1.2)) is 0x%lx (%d MB)\n",
					     milo_memory_size,
					     milo_memory_mb);

				/* now load up the reboot parameter block */
				milo_reboot_block->memory_size =
				    milo_memory_mb;
				milo_reboot_block->flags |= REBOOT_CONSOLE;
			}
		}
	}
	return rebooting;
}

/*****************************************************************************
 *  HWRPB routines                                                           *
 *****************************************************************************/
/* create an entry for the HWRPB in the page tables */
void make_HWRPB_virtual(void)
{
	int pfn = _PHYSICAL_TO_PFN(hwrpb);

	/* make an entry for its virtual address */
	add_VA((U64) INIT_HWRPB, pfn, (_PAGE_KWE | _PAGE_KRE));

#if defined(DEBUG_BOOT)
	printk("...Hwrpb at virtual address 0x%p (pfn = 0x%x)\n",
	       INIT_HWRPB, pfn);
#endif
}

/* initialise the HWRPB (if there isn't already one) */
void init_HWRPB(void)
{
	struct percpu_struct *percpu;
	unsigned int pfn;
#if !defined(MINI_ALPHA_RUFFIAN)
#if defined(MILO_TRUST_IMPURE)
	U64 PalImpureBase;
#endif
	unsigned int cc1, cc2;
#endif

#if defined(DEBUG_BOOT)
	printk("Building a HWRPB (part I)\n");
#endif

	/* always use the first page (see init_mem() in memory.c) */
	pfn = 0;

	hwrpb = (struct hwrpb_struct *) _PFN_TO_PHYSICAL(pfn);
	milo_reboot_block =
	    (Milo_reboot_t *) ((char *) hwrpb +
			       sizeof(struct hwrpb_struct));

	/* we may be rebooting, in which case there is already a HWRPB that has
	 * been built (and which contains important information) */
	if (setup_boot_arguments()) {
		if (milo_verbose)
			printk("Rebooting\n");
	} else {
		if (milo_verbose)
			printk("Booting\n");

		/* fill out the basic bits */
		hwrpb->id = HWRPB_SIGNATURE;
		hwrpb->phys_addr = (U64) hwrpb & 0xffffffff;
		hwrpb->size = sizeof(struct hwrpb_struct);

		strcpy(hwrpb->ssn, "MILO-");
		strncpy(hwrpb->ssn + 5, version, 10);
		hwrpb->ssn[15] = '\0';
		/* System serial number ?? */
		hwrpb->sys_revision = 0;

#if !defined(MINI_ALPHA_RUFFIAN)

		hwrpb->cycle_freq = 0;

		if (!getenv("CPU_CLOCK")) {
			/* see if the impure area has a cycle_freq we can use */
#if defined(MILO_TRUST_IMPURE)
			PalImpureBase = (U64)(CNS_Q_BASE + cServe(0, 0, CSERVE_K_RD_IMPURE));
			if (ReadQ(PalImpureBase + CNS_Q_SIGNATURE) == IMPURE_SIGNATURE)  {
				hwrpb->cycle_freq =
					1000000000000L / ReadQ(PalImpureBase + CNS_Q_CYCLE_CNT);
			}
#endif

			do {} while (!(CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP));
			do {} while (CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP);

			/* Read cycle counter exactly on falling edge of update flag */
			cc1 = rpcc();

			/* If our cycle frequency isn't valid, go another round and give
			   a guess at what it should be.  */
			if (hwrpb->cycle_freq == 0) {
				do {} while (!(CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP));
				do {} while (CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP);
				cc2 = rpcc();
				hwrpb->cycle_freq = cc2 - cc1;
				printk
				    ("HWRPB cycle frequency estimated at %lu Hz\n",
				     hwrpb->cycle_freq);
			}
		}
#endif				/* !RUFFIAN */

		hwrpb->nr_processors = 1;
		hwrpb->processor_offset =
		    sizeof(struct hwrpb_struct) + sizeof(Milo_reboot_t);
		hwrpb->processor_size = sizeof(struct percpu_struct);
		percpu =
		    (struct percpu_struct *) (((char *) hwrpb) +
					      hwrpb->processor_offset);
		percpu->serial_no[0] = 0x73695f78756e694c;
		/* :-) */
		percpu->serial_no[1] = 0x002174616572475f;
		percpu->type = cpuid();

/*
 *  Set up the system type and variation.
 *  This is based on configure-time #defines 
 */

/*
 * the EV5/EV56 based platforms
 */
#ifdef MINI_ALPHA_EB164
		hwrpb->sys_type = ST_DEC_EB164;
		hwrpb->sys_variation = 1L << 10;	/* EB164 266mhz */
#endif
#ifdef MINI_ALPHA_PC164
		hwrpb->sys_type = ST_DEC_EB164;
		hwrpb->sys_variation = 3L << 10;	/* PC164 366mhz */
#endif
#ifdef MINI_ALPHA_LX164
		hwrpb->sys_type = ST_DEC_EB164;
		hwrpb->sys_variation = 8L << 10;	/* PC164-LX 400mhz */
#endif
#ifdef MINI_ALPHA_SX164
		hwrpb->sys_type = ST_DEC_EB164;
		hwrpb->sys_variation = 12L << 10;	/* PC164-SX 400mhz */
#endif
#ifdef MINI_ALPHA_MIATA
		hwrpb->sys_type = ST_DEC_MIATA;
		hwrpb->sys_variation = 1L << 10;	/* 433mhz */
#endif
#ifdef MINI_ALPHA_TAKARA
		hwrpb->sys_type = ST_DEC_TAKARA;
		hwrpb->sys_variation = 1L << 10;	/* ???mhz */
#endif
#ifdef MINI_ALPHA_ALCOR
		hwrpb->sys_type = ST_DEC_ALCOR;	/* 300mhz */
		hwrpb->sys_variation = 1L << 10;	/* ALCOR */
#endif
#ifdef MINI_ALPHA_XLT
		hwrpb->sys_type = ST_DEC_ALCOR;	/* 300mhz */
		hwrpb->sys_variation = 12L << 10;	/* BRET */
#endif
#ifdef MINI_ALPHA_RUFFIAN
		hwrpb->sys_type = ST_DTI_RUFFIAN;
		hwrpb->sys_variation = 1L << 10;
#endif

/*
 * the EV4/EV45/LCA4/LCA45 based platforms
 */
#ifdef MINI_ALPHA_EB64
		hwrpb->sys_type = ST_DEC_EB64;
		hwrpb->sys_variation = 1L << 10;
#endif
#ifdef MINI_ALPHA_EB66
		hwrpb->sys_type = ST_DEC_EB66;
		hwrpb->sys_variation = 1L << 10;
#endif
#ifdef MINI_ALPHA_EB66P
		hwrpb->sys_type = ST_DEC_EB66;
		hwrpb->sys_variation = 2L << 10;
#endif
#ifdef MINI_ALPHA_EB64P
		hwrpb->sys_type = ST_DEC_EB64P;
		hwrpb->sys_variation = 1L << 10;
#endif
#ifdef MINI_ALPHA_CABRIOLET
		hwrpb->sys_type = ST_DEC_EB64P;
		hwrpb->sys_variation = 2L << 10;
#endif
#ifdef MINI_ALPHA_AVANTI
		hwrpb->sys_type = ST_DEC_2100_A50;
		hwrpb->sys_variation = 5L << 10;	/* 233mhz */
#endif
#ifdef MINI_ALPHA_XL
		hwrpb->sys_type = ST_DEC_XL;
		hwrpb->sys_variation = 1L << 10;	/* 233mhz */
#endif
#ifdef MINI_ALPHA_NONAME
		hwrpb->sys_type = ST_DEC_AXPPCI_33;
		hwrpb->sys_variation = 1L << 10;
#endif
#ifdef MINI_ALPHA_LCA
		percpu->variation = ~PASS1_LCA_TYPE;
#endif

/*
 * platforms that could have either EV4 or EV5 CPUs
 */
#ifdef MINI_ALPHA_MIKASA
		hwrpb->sys_type = ST_DEC_1000;
#ifdef MINI_ALPHA_EV5
		hwrpb->sys_variation = 4L << 10;
#else
		hwrpb->sys_variation = 1L << 10;
#endif
#endif
#ifdef MINI_ALPHA_NORITAKE
		hwrpb->sys_type = ST_DEC_NORITAKE;
#ifdef MINI_ALPHA_EV5
		hwrpb->sys_variation = 2L << 10;
#else
		hwrpb->sys_variation = 1L << 10;
#endif
#endif


#if defined(DEBUG_BOOT)
		printk("...sys_type = %ld\n", hwrpb->sys_type);
#endif

		/* Clock interrupt frequency. */
		hwrpb->intr_freq = HZ * 4096;

		/* Max ASN */
#ifdef MINI_ALPHA_EV5
		hwrpb->max_asn = 127;
#else
		hwrpb->max_asn = 63;
#endif

		/* Clock frequency */
		hwrpb->intr_freq = HZ * 4096;

		/* CPU type */
		hwrpb->cpuid = cpuid();

		/* Page size */
		hwrpb->pagesize = 8192;

		/* Physical address bits. */
#ifdef MINI_ALPHA_EV5
		hwrpb->pa_bits = 40;
#else
		hwrpb->pa_bits = 34;
#endif
	}
}

void build_HWRPB_mem_map(void)
{
	struct memclust_struct *cluster;
	struct memdesc_struct *memdesc;
	char *page;
	unsigned int pfn;

#if defined(DEBUG_BOOT)
	int i;
#endif

#if defined(DEBUG_BOOT)
	printk("Building HWRPB (part II)\n");
#endif

	/* mark as free any pages currently marked as TEMPORARY_PAGE */
#ifdef DEBUG_BOOT
	printk("...freeing temporary pages\n");
#endif
	page = milo_memory_map;
	for (pfn = 0; pfn < lastpfn + 1; pfn++) {
		if (*page == TEMPORARY_PAGE)
			*page = FREE_PAGE;
		page++;
	}

	memdesc = (struct memdesc_struct *)
	    ((char *) hwrpb + sizeof(struct hwrpb_struct)
	     + sizeof(Milo_reboot_t)
	     + sizeof(struct percpu_struct));

/*
 *  build clusters describing the clumps of memory that are available.
 *
 *  usage = 0 = free
 *          1 = free, but nonvolatile
 *          2 = reserved to console
 */
#ifdef DEBUG_BOOT
	printk("...memdesc @ 0x%lx\n", (long) memdesc);
#endif
	hwrpb->mddt_offset = (char *) memdesc - (char *) hwrpb;
	memdesc->numclusters = 0;
	cluster = (struct memclust_struct *) (&memdesc->cluster[0]);

/*
 *  Build the memory clusters from the memory map.
 */
	page = milo_memory_map;
	memdesc->numclusters = 1;
	cluster->start_pfn = 0;
	cluster->usage = *page++;
	cluster->numpages = 1;
	pfn = 1;
	while (pfn < lastpfn + 1) {

		/* find the next free cluster */
		if (*page == cluster->usage) {
			cluster->numpages++;
		} else {

			/* new cluster */
			cluster = (struct memclust_struct *)
			    (&memdesc->cluster[memdesc->numclusters++]);
			cluster->usage = *page;
			cluster->start_pfn = pfn;
			cluster->numpages = 1;
		}
		page++;
		pfn++;
	}

/*
 *  Now dump out the clusters.
 */
#if defined(DEBUG_BOOT)
	printk("...Memory Clusters\n");
	for (i = 0; i < memdesc->numclusters; i++) {
		cluster =
		    (struct memclust_struct *) (&memdesc->cluster[i]);
		printk
		    ("     Cluster %d, start pfn = %04lu, size = %4lu, %s\n",
		     i, cluster->start_pfn, cluster->numpages,
		     cluster->usage == FREE_PAGE ? "free" : "reserved");
	}
#endif
}

unsigned long asn_cache = ASN_FIRST_VERSION;

void get_new_mmu_context(struct task_struct *p, struct mm_struct *mm)
{
	unsigned long asn = asn_cache;

	if ((asn & HARDWARE_ASN_MASK) < MAX_ASN)
		++asn;
	else {
		tbiap();
		imb();
		asn = (asn & ~HARDWARE_ASN_MASK) + ASN_FIRST_VERSION;
	}
	asn_cache = asn;
	mm->context = asn;	/* full version + asn */
	p->tss.asn = asn & HARDWARE_ASN_MASK;	/* just asn */
}
