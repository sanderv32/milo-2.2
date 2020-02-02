/*
 * dumbirq.c
 * adds the neccessary dummy symbols to get milo through  
 * the relocate and stub stages. These symbols are needed
 * by most of the sys_* files in $(KSRC)/arch/alpha/kernel/
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Stefan Reinauer, <stepan@suse.de>
 *
 */

#include <linux/config.h>
#include <linux/pci.h>
#include "milo.h"

#ifdef CONFIG_ALPHA_GENERIC
int alpha_use_srm_setup, alpha_using_srm=0;
void srm_device_interrupt(unsigned long vector, struct pt_regs * regs)
{
}
#endif

unsigned long loops_per_sec = 100000000UL;
unsigned long _alpha_irq_masks[2] = {~0UL, ~0UL };

#if defined(MINI_ALPHA_NONAME) || defined(MINI_ALPHA_P2K) || defined(MINI_ALPHA_AVANTI) || defined(MINI_ALPHA_XL)

#include <linux/tty.h>

struct screen_info	screen_info;
struct pci_dev		*pci_devices;

int pcibios_read_config_dword (unsigned char bus, unsigned char dev_fn,
                               unsigned char where, unsigned int *val)
{
	return 0;
}

int pcibios_write_config_dword (unsigned char bus, unsigned char dev_fn,
                                unsigned char where, unsigned int val)
{
	return 0;
}

#endif


void enable_irq(unsigned int irq_nr)
{
#if 0
	printk ("enable_irq\n");
#endif
}

void handle_irq(int irq, int ack, struct pt_regs * regs)
{
#if 0
	printk ("handle_irq\n");
#endif
}

void request_region(unsigned long from, unsigned long num, const char *name)
{
#if 0
	printk ("request_region\n");
#endif
}

void
process_mcheck_info(unsigned long vector, unsigned long la_ptr,
                  struct pt_regs *regs, char *machine,
                  unsigned int debug, unsigned int expected)
{
#if 0
	printk ("process_mcheck_info\n");
#endif
}

/* ---- */

void ev5_flush_tlb_current(struct mm_struct *mm)
{
}

void
ev5_flush_tlb_other(struct mm_struct *mm)
{
#if 0
        mm->context = 0;
#endif
}

void
ev5_flush_tlb_current_page(struct mm_struct * mm,
                           struct vm_area_struct *vma,
                           unsigned long addr)
{
}

void ev5_get_mmu_context(struct task_struct *p)
{
}

void ev4_flush_tlb_current(struct mm_struct *mm)
{
}

void ev4_flush_tlb_other(struct mm_struct *mm)
{
#if 0
        mm->context = 0;
#endif
}

void
ev4_flush_tlb_current_page(struct mm_struct * mm,
                           struct vm_area_struct *vma,
                           unsigned long addr)
{
}

void
ev4_get_mmu_context(struct task_struct *p)
{
}

#ifndef MINI_ALPHA_RUFFIAN

void isa_device_interrupt(unsigned long vector, struct pt_regs * regs) 
{
}

void layout_all_busses(unsigned long io_base, unsigned long mem_base)
{
}

void common_pci_fixup(int (*map_irq)(struct pci_dev *dev, int slot, int pin),
                 int (*swizzle)(struct pci_dev *dev, int *pin))
{
}

void generic_init_pit(void)
{
}

void generic_ack_irq(unsigned long irq)
{
}

#endif

#if !defined(MINI_ALPHA_RUFFIAN) && !defined(MINI_ALPHA_MIATA)
void enable_ide(void)
{
}
#endif

int common_swizzle(struct pci_dev *dev, int *pinp)
{
	return 0;
}

void generic_kill_arch(int mode, char *restart_cmd)
{
#ifdef DEBUG_KERNEL
	printk("generic_kill_arch()\n"); 
#endif
	halt();
}

void es1888_init(void)
{
}

