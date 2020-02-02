/*
 *	linux/arch/alpha/kernel/core_cia.c
 *
 * Code common to all CIA core logic chips.
 *
 * Written by David A Rusling (david.rusling@reo.mts.dec.com).
 * December 1995.
 *
 */
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/ptrace.h>

#define __EXTERN_INLINE inline
#include <asm/io.h>
// #include <asm/core_cia.h>
#undef __EXTERN_INLINE


#define vuip	volatile unsigned int  *

unsigned long cia_virt_to_bus(void * address)
{
	return 0;
}

void * cia_bus_to_virt(unsigned long address)
{
	return NULL;
}

unsigned long cia_dense_mem(unsigned long addr)
{
	return 0;
}

unsigned long cia_readb(unsigned long addr)
{
	return 0;
}
unsigned long cia_readw(unsigned long addr)
{
	return 0;
}
void cia_writeb(unsigned char b, unsigned long addr)
{
}
void cia_writew(unsigned short b, unsigned long addr)
{
}
unsigned long cia_readl(unsigned long addr)
{
	return 0;
}
unsigned long cia_readq(unsigned long addr)
{
	return 0;
}

void cia_writel(unsigned int b, unsigned long addr)
{
}

void cia_writeq(unsigned long b, unsigned long addr)
{    
}

unsigned int cia_inb(unsigned long addr)
{
	return 0;
}

void cia_outb(unsigned char b, unsigned long addr)
{
}

unsigned int cia_inw(unsigned long addr)
{
	return 0;
}
void cia_outw(unsigned short b, unsigned long addr)
{
}
unsigned int cia_inl(unsigned long addr)
{
	return 0;
}
void cia_outl(unsigned int b, unsigned long addr)
{
}


int
cia_hose_read_config_byte (u8 bus, u8 device_fn, u8 where, u8 *value,
			   struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
cia_hose_read_config_word (u8 bus, u8 device_fn, u8 where, u16 *value,
			   struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
cia_hose_read_config_dword (u8 bus, u8 device_fn, u8 where, u32 *value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
cia_hose_write_config_byte (u8 bus, u8 device_fn, u8 where, u8 value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
cia_hose_write_config_word (u8 bus, u8 device_fn, u8 where, u16 value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
cia_hose_write_config_dword (u8 bus, u8 device_fn, u8 where, u32 value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

void __init
cia_init_arch(unsigned long *mem_start, unsigned long *mem_end)
{
}

void
cia_machine_check(unsigned long vector, unsigned long la_ptr,
		  struct pt_regs * regs)
{
}
