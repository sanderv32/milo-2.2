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
 *  memory management code.
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
#include "impure.h"
#include "uart.h"
#include "fs.h"

typedef struct Mhead {
	U64 size;
	struct Mhead *next;
} Mhead_t;

Mhead_t *header;

/*****************************************************************************
 *  Global variables.                                                        *
 *****************************************************************************/
/*
 *  For device drivers and so on.
 */

/*
 *  memory allocation
 */
extern U64 milo_memory_size;

#define SLAB_OFFSET_BITS        16	/* could make this larger for 64bit archs */


char *milo_memory_map = NULL;
volatile unsigned int firstpfn = 0;
volatile unsigned int lastpfn = 0;

unsigned long max_mapnr = 0;
unsigned long num_physpages = 0;

static char *temp_memory_ptr = NULL;

#ifdef REUSE_MEMORY
static Mhead_t *free_cache = NULL;
#endif

/*****************************************************************************
 *  Memory allocation code                                                   *
 *****************************************************************************/
/*
 *  Allocate some space that we can free up later.
 */
#ifdef DEBUG_MALLOC
void *__vmalloc(unsigned long size, char *file, int line)
{
	void *p;

	p = __kmalloc(size, 1, file, line);
	return p;
}
#else
void *vmalloc(size_t size)
{
	void *p;

	p = kmalloc(size, 1);
	return p;
}
#endif

#ifdef REUSE_MEMORY
/* search the cache for free memory */
static inline void *search_cache(unsigned int size)
{
	char *result;
	Mhead_t *header;

	if (free_cache != NULL) {
		Mhead_t *last = NULL;
		header = free_cache;
		while (header != NULL) {
			if (header->size >= size) {
				/* hop over  the header */
				result = (char *) header + sizeof(Mhead_t);
#ifdef GUARD_BANDS
				/* hop over the first guard band */
				result += 16;
#endif
				if (header == free_cache)
					free_cache = header->next;
				else
					last->next = header->next;
				return (void *) result;
			}
			last = header;
			header = header->next;
		}
	}
	return NULL;
}
#endif

#ifdef DEBUG_MALLOC
void *__kmalloc(unsigned int size, int priority, const char *file,
		const int line)
#else
void *kmalloc(size_t size, int priority)
#endif
{
	char *result;
	int top, bottom, pages, room, pfn;
	char *mm;

#ifdef DEBUG_MEMORY
#ifdef DEBUG_MALLOC
	printk("kmalloc(size %d prio %d file %s:%d)\n",
	       size, priority, file ? file : "?.c", line);
#else
	printk("kmalloc(size %d prio %d)\n", size, priority);
#endif
#endif

	size += 8 - (size & 0x7);	/* align to an 8 byte boundary */
#ifdef GUARD_BANDS
	size += (4 * 8);
#endif

#ifdef REUSE_MEMORY
	/* check the cache of free buffers */
	result = search_cache(size);
	if (result != NULL)
		return result;

	/* failed to find it in the cache, allow for the header... */
	size += sizeof(Mhead_t);
#endif

	pages = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;

	/* is there already some memory that we might be able to use? */
	if (temp_memory_ptr == NULL) {
		/* nope, allocate some */
		pfn = allocate_many_pfn(pages, TEMPORARY_PAGE);
		temp_memory_ptr = (char *) _PFN_TO_PHYSICAL(pfn);
	} else {
		bottom = _PHYSICAL_TO_PFN(temp_memory_ptr);
		top = _PHYSICAL_TO_PFN(temp_memory_ptr + size);

		/* Is there room in this page?  If not, are the next pages free? */
		room = TRUE;
		if (bottom != top) {
			/* not enough room in this page */
			int saved_pages = pages;

			mm = (char *) milo_memory_map + bottom + 1;
			while (saved_pages--) {
				if (*mm++ != FREE_PAGE) {
					room = FALSE;
					break;
				}
			}
			if (room) {
				int saved_pages = pages;

				mm = (char *) milo_memory_map + bottom + 1;
				while (saved_pages--)
					*mm++ = TEMPORARY_PAGE;
			} else {
				/* no room, allocate more memory */
				pfn =
				    allocate_many_pfn(pages,
						      TEMPORARY_PAGE);
				temp_memory_ptr =
				    (char *) _PFN_TO_PHYSICAL(pfn);
			}
		}
	}

	/* ok, did we get or have enough memory? */
	if (temp_memory_ptr != NULL) {
		result = temp_memory_ptr;
		temp_memory_ptr += size;
	} else {
		return NULL;
	}

	memset(result, 0, size);

#ifdef REUSE_MEMORY
	/* add in the header */
	header = (Mhead_t *) result;
	size -= sizeof(Mhead_t);
	header->size = size;
	header->next = NULL;
	result += sizeof(Mhead_t);
#endif

#ifdef GUARD_BANDS
	{
		U64 *guard = (U64 *) result;

		*guard++ = 0xAAAABBBBCCCCDDDDUL;
		*guard = 0xAAAABBBBCCCCDDDDUL;

		guard = (U64 *) (result + size - 16);
		*guard++ = 0x1111222233334444UL;
		*guard = 0x1111222233334444UL;

		result = result + 16;

	}
#endif

#ifdef DEBUG_MEMORY
	printk("malloc(): returning 0x%p\n", result);
#endif
	return result;
}

void free(void *where)
{
#ifdef REUSE_MEMORY
	Mhead_t *header;

#ifdef GUARD_BANDS
	where -= 16;
#endif
	where -= sizeof(Mhead_t);
	header = (Mhead_t *) where;

	header->next = free_cache;
	free_cache = header;
#endif
}

void kfree(void *where)
{
	free(where);
}
void kfree_s(const void *objp, size_t size)
{
	free((void *) objp);
}

/*
 *  Allocate many free pages.
 */
unsigned int allocate_many_pfn(unsigned int count, int type)
{
	unsigned int pfn = 0;
	unsigned int block_size, count_remainder = count % 512;
#ifdef DEBUG_MEMORY
	printk("allocate_many_pfn: called, count = %d\n", count);
#endif

	/*
	 * Seek the smallest power-of-2 block_size greater than or equal
	 * to count, but stop at 512, the largest number of pages that
	 * Alpha can lump together into a translation buffer superpage.
	 * If count is more than 512 pages, then for alignment purposes
	 * we really only care about the remainder (above).
	 */
	for (block_size = 1; block_size < 512; block_size += block_size)
		if (count <= block_size)
			break;

	while ((pfn <= lastpfn) && (pfn + count <= lastpfn + 1)) {
		char *first, *last;

		for (first = &milo_memory_map[pfn], last = first + count;
		     first < last; ++first)
			if (*first != FREE_PAGE)
				break;	/* Didn't find count pages starting at pfn, */
		if (first != last) {	/* so try again just beyond the failure. */
			unsigned int pfn_start_aligned, pfn_end_aligned;

			pfn = (first - milo_memory_map) + 1;
			/* Round up to the next block_size boundary. */
			pfn_start_aligned =
			    (pfn + (block_size - 1)) & ~(block_size - 1);
			/* Back down by the number of pages (mod 512) needed. */
			pfn_end_aligned =
			    pfn_start_aligned - count_remainder;
			/* Choose the smaller value if it isn't too small. */
			pfn =
			    (pfn_end_aligned <
			     pfn) ? pfn_start_aligned : pfn_end_aligned;
			continue;
		}
		/* SUCCESS */
		memset(&milo_memory_map[pfn], type, count);
#ifdef DEBUG_MEMORY
		printk("allocate_many_pfn: returning %d (@ 0x%p)\n",
		       pfn, _PFN_TO_PHYSICAL(pfn));
#endif
		return pfn;
	}
	error("Not enough memory");
	return 0;
}

void mark_many_pfn(int pfn, int count, int type)
{
	char *page;
	int i;

#ifdef DEBUG_MEMORY
	printk("(pfn %04d to %04d)\n", pfn, pfn + count - 1);
#endif
	page = milo_memory_map + pfn;
	for (i = 0; i < count; i++)
		*page++ = type;
}

/*
 *  Allocate a free page.
 */
unsigned int allocate_pfn(int type)
{
	unsigned int pfn;
	char *page;

	page = milo_memory_map;
	for (pfn = 0; pfn < lastpfn + 1; pfn++) {
#if 0
#ifdef DEBUG_MEMORY
		printk("pfn = 0x%x, usage = %d\n", pfn, *page);
#endif
#endif
		if (*page == FREE_PAGE) {
			*page = type;
			return pfn;
		}
		page++;
	}
	error("Not enough memory");
	return 0;
}

void zeropage_phys(int pfn)
{
	unsigned long *where;
	int size, i;

	where = (unsigned long *) _PFN_TO_PHYSICAL(pfn);
	size = PAGE_SIZE / sizeof(unsigned long);
	for (i = 0; i < size; i++)
		*where++ = 0;
}

/*
 *  Attempt to find out how much memory we have.
 *  Returns the number of contiguous pages.
 */
/* #define MEM_PROBE	0x123456789abcdef0ul */

#ifdef MEM_PROBE
static unsigned long size_mem()
{
	unsigned long pfn, size = 0;
	volatile unsigned long *p;
	int i, l = 2;

	for (pfn = 16 * 128 /* 16 Mb */ ; pfn < 1024 * 128 /* 1 Gb */ ;
	     pfn++) {
		p = _PFN_TO_PHYSICAL(pfn);
		*p = MEM_PROBE;
		i = *p != MEM_PROBE || (*p = ~MEM_PROBE, *p != ~MEM_PROBE);
		if (i != l)
			l =
			    i, printk("%s at pfn %lu (%lu Mb)\n",
				      i ? "no mem" : "mem ok", pfn,
				      pfn / 128);
		if (i && !size)
			size = pfn;
	}
	return size;
}
#endif

/*
 *  Initialise the memory allocation scheme.  This is called early on.  It 
 *  assumes that the miniloader is loaded at PALCODE_AT (PALcode first, 
 *  followed by an image) and that the stack is below the image in memory.
 *  
 *  Memory is marked as:
 *
 *          0 = free
 *          1 = free, but nonvolatile
 *          2 = reserved to console
 */
void init_mem(U64 Top)
{
	int map_pages;

	if (milo_verbose)
		printk("Initialising the memory, size is 0x%lx\n", Top);
#ifdef MEM_PROBE
	size_mem();
#endif

	milo_memory_size = (U64) high_memory = Top;
/*
 *  Work out the first and last pfns.  These are used all over the code to 
 *  allocate pages and to build memory maps for Linux itself.  Very important 
 *  to get this right.
 */
	firstpfn = 0;
	lastpfn = _PHYSICAL_TO_PFN(Top - PAGE_SIZE);
#ifdef DEBUG_MEMORY
	printk("...last pfn is %04d\n", lastpfn);
#endif
/*
 *  Take the top page(s) for the memory map.  Each entry takes one byte.
 */
	map_pages = (_PHYSICAL_TO_PFN(Top) + (PAGE_SIZE - 1)) / PAGE_SIZE;
	milo_memory_map =
	    (char *) (PAGE_OFFSET |
		      (MILO_MIN_MEMORY_SIZE - (PAGE_SIZE * map_pages)));
#if defined(DEBUG_MEMORY)
	printk("...memory map @ 0x%x (pfn = %04d), %d pages\n",
	       milo_memory_map, _PHYSICAL_TO_PFN(milo_memory_map),
	       map_pages);
#endif
/*
 *  Initialise the memory map.
 */

	/* mark everything as free (initially) */
#if defined(DEBUG_MEMORY)
	printk("...mark all pages as free: ");
#endif
	mark_many_pfn(0, lastpfn + 1, FREE_PAGE);

	/* mark the memory map as not free */
#if defined(DEBUG_MEMORY)
	printk("...marking page(s) containing memory map as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(milo_memory_map), map_pages,
		      TEMPORARY_PAGE);

	/* mark page 0 as not free - it will contain the HWRPB */
#if defined(DEBUG_MEMORY)
	printk("...marking HWRPB as occupied: ");
#endif
	mark_many_pfn(0, 1, ALLOCATED_PAGE);

#ifdef NOT_NOW

	/* mark the ZERO_PGE as not free */
#if defined(DEBUG_MEMORY)
	printk("...marking ZERO_PGE as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(ZERO_PGE), 1, ALLOCATED_PAGE);

	/* mark the INIT_STACK as not free */
#if defined(DEBUG_MEMORY)
	printk("...marking INIT_STACK as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(INIT_STACK), 1, ALLOCATED_PAGE);

#endif				/* NOT_NOW */

#ifdef DEBUG_USE_DBM
	/* REMOVE THIS IF YOU WANT MORE MEMORY, LEAVE IT IF YOU WANT TO RUN THE
	 * MINIDEBUGGER */

	/* for now mark the area occupied by the Debug Monitor palcode as not free
	 */
#if defined(DEBUG_MEMORY)
	printk("...(TEMP) marking Debug Monitor PALcode as occupied: ");
#endif
	mark_many_pfn(0, _PHYSICAL_TO_PFN(PALCODE_SIZE), ALLOCATED_PAGE);

	/* for now, mark the area occupied by the debug monitor as not free */
#if defined(DEBUG_MEMORY)
	printk("...(TEMP) marking Debug Monitor as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(PALCODE_SIZE),
		      _PHYSICAL_TO_PFN(0x100000), ALLOCATED_PAGE);
#endif				/* DEBUG_USE_DBM */

	/* for now, mark the area occupied by the miniloader as not free.
	   Assume that the Miniloader is 1 Mbyte long (an overestimate). */
#if defined(DEBUG_MEMORY)
	printk("...(TEMP) marking Miniloader as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(LOADER_AT),
		      _PHYSICAL_TO_PFN(0x100000), TEMPORARY_PAGE);

	/* Mark the area occupied by the stub and compressed miniloader as not free.
	   Assume that it's half a meg long (an overestimate) */
#if defined(DEBUG_MEMORY)
	printk("...marking stub and compressed Miniloader as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(STUB_AT),
		      _PHYSICAL_TO_PFN(0X80000), ALLOCATED_PAGE);

	/* mark the memory that will be occupied by the kernel as reserved.  The
	 * number of pages is a guess; put it right when we read in the image */
#if defined(DEBUG_MEMORY)
	printk("...marking Linux kernel as occupied (TEMP): ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(KERNEL_START),
		      _PHYSICAL_TO_PFN(MAX_KERNEL_SIZE), TEMPORARY_PAGE);

	/* mark the area occupied by the palcode as not free */
#if defined(DEBUG_MEMORY)
	printk("...marking PALcode as occupied: ");
#endif
	mark_many_pfn(_PHYSICAL_TO_PFN(PALCODE_AT),
		      _PHYSICAL_TO_PFN(PALCODE_SIZE), ALLOCATED_PAGE);

}

#if 0
/* BE CAREFUL WITH THIS!!! was called from x86_sysenv.c for some reason */
void clean_mem(void)
{
	int pfn = _PHYSICAL_TO_PFN(0x200000);
	char *page = milo_memory_map;

	while (pfn < (lastpfn + 1)) {
		if (*page == TEMPORARY_PAGE)
			*page = FREE_PAGE;
		page++;
		pfn++;
	}
}
#endif

void reset_mem(U64 Top)
{
	unsigned int old_map_pfns, new_map_pfns;

#ifdef MINI_DEBUG
	printk("Re-Initialising the memory, size is 0x%x\n", Top);
#endif

	/* the memory map stays where it is, but it's size changes */
	old_map_pfns = _PHYSICAL_TO_PFN(milo_memory_size - PAGE_SIZE);
	new_map_pfns = _PHYSICAL_TO_PFN(Top - PAGE_SIZE);
	if (old_map_pfns < new_map_pfns) {
#if defined(DEBUG_MEMORY)
		printk("...mark new pages as free: ");
#endif
		mark_many_pfn(old_map_pfns + 1,
			      new_map_pfns - old_map_pfns, FREE_PAGE);
	}

	/* finally, reset the memory size */
	milo_memory_size = (U64) high_memory = Top;
	if (milo_reboot_block != NULL)
		milo_reboot_block->memory_size =
		    milo_memory_size / (1024 * 1024);
	lastpfn = _PHYSICAL_TO_PFN(Top - PAGE_SIZE);
}

#if 0
void do_entMM(u64 a0, u64 a1, u64 a2, u64 ps, u64 pc, u64 gp)
{
	/* inform user of memory fault and regs */
	printk("\nMemory fault:\n");
	printk("    ps = 0x%016lx, pc = 0x%016lx, gp = 0x%016lx\n", ps, pc,
	       gp);
	printk("    a0 = 0x%016lx, a1 = 0x%016lx, a2 = 0x%016lx\n", a0, a1,
	       a2);

	/* wait for keypress */
	printk("\nPress any key to reboot... ");
	kbd_getc();
}
#endif


#ifdef DEBUG_MALLOC
#undef kmalloc
#undef vmalloc
#undef free

void *kmalloc(const int osize, const int priority)
{
	return __kmalloc(osize, priority, "LINUX.C", 0);
}
void *vmalloc(unsigned long size)
{
	return __kmalloc(size, 1, "LINUXvmalloc.C", 0);
}
#endif

void *vremap(unsigned long offset, unsigned long size)
{
	return (void *) offset;
}

void vfree(void *addr)
{
}

typedef struct kmem_slab_s {
	struct kmem_bufctl_s *s_freep;	/* ptr to first inactive obj in slab */
	struct kmem_bufctl_s *s_index;
	unsigned long s_magic;
	unsigned long s_inuse;	/* num of objs active in slab */

	struct kmem_slab_s *s_nextp;
	struct kmem_slab_s *s_prevp;
	void *s_mem;		/* addr of first obj in slab */
	unsigned long s_offset:SLAB_OFFSET_BITS, s_dma:1;
} kmem_slab_t;

typedef struct kmem_cache_s kmem_cache_t;

struct kmem_cache_s {
	kmem_slab_t *c_freep;	/* first slab w. free objs */
	unsigned long c_flags;	/* constant flags */
	unsigned long c_offset;
	unsigned long c_num;	/* # of objs per slab */

	unsigned long c_magic;
	unsigned long c_inuse;	/* kept at zero */
	kmem_slab_t *c_firstp;	/* first slab in chain */
	kmem_slab_t *c_lastp;	/* last slab in chain */

	spinlock_t c_spinlock;
	unsigned long c_growing;
	unsigned long c_dflags;	/* dynamic flags */
	size_t c_org_size;
	unsigned long c_gfporder;	/* order of pgs per slab (2^n) */
	void (*c_ctor) (void *, kmem_cache_t *, unsigned long);	/* constructor func */
	void (*c_dtor) (void *, kmem_cache_t *, unsigned long);	/* de-constructor func */
	unsigned long c_align;	/* alignment of objs */
	size_t c_colour;	/* cache colouring range */
	size_t c_colour_next;	/* cache colouring */
	unsigned long c_failures;
	const char *c_name;
	struct kmem_cache_s *c_nextp;
	kmem_cache_t *c_index_cachep;
#if     SLAB_STATS
	unsigned long c_num_active;
	unsigned long c_num_allocations;
	unsigned long c_high_mark;
	unsigned long c_grown;
	unsigned long c_reaped;
	atomic_t c_errors;
#endif				/* SLAB_STATS */
};

typedef struct kmem_bufctl_s {
	union {
		struct kmem_bufctl_s *buf_nextp;
		kmem_slab_t *buf_slabp;	/* slab for obj */
		void *buf_objp;
	} u;
} kmem_bufctl_t;

inline void *kmem_cache_alloc(kmem_cache_t * cachep, int flags)
{
	return (NULL);
}


kmem_cache_t *kmem_cache_create(const char *name, size_t size,
				size_t offset, unsigned long flags,
				void (*ctor) (void *, kmem_cache_t *,
					      unsigned long),
				void (*dtor) (void *, kmem_cache_t *,
					      unsigned long))
{
	return (NULL);

}

inline void kmem_cache_free(kmem_cache_t * cachep, const void *objp)
{
	return;
}
