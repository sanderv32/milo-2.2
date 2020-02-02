/*
 *	linux/arch/alpha/kernel/core_pyxis.c
 *
 * Code common to all PYXIS core logic chips.
 *
 * Based on code written by David A Rusling (david.rusling@reo.mts.dec.com).
 *
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/init.h>

#include <asm/ptrace.h>
#include <asm/system.h>

#define __EXTERN_INLINE inline
#include <asm/io.h>
// #include <asm/core_pyxis.h>
#undef __EXTERN_INLINE

#if 1
unsigned long pyxis_virt_to_bus(void * address)
{
	return 0;
}

void * pyxis_bus_to_virt(unsigned long address)
{
	return NULL;
}

unsigned int pyxis_bw_inb(unsigned long addr)
{
	return 0;
}

unsigned int pyxis_bw_inw(unsigned long addr)
{
	return 0;
}

unsigned int pyxis_bw_inl(unsigned long addr)
{
	return 0;
}

void pyxis_bw_outb(unsigned char b, unsigned long addr)
{
}

void pyxis_bw_outw(unsigned short b, unsigned long addr)
{
}

void pyxis_bw_outl(unsigned int b, unsigned long addr)
{
}

unsigned long pyxis_bw_readb(unsigned long addr)
{
	return 0;
}

unsigned long pyxis_bw_readw(unsigned long addr)
{
	return 0;
}

unsigned long pyxis_bw_readl(unsigned long addr)
{
	return 0;
}

unsigned long pyxis_bw_readq(unsigned long addr)
{
	return 0;
}


void pyxis_bw_writeb(unsigned char b, unsigned long addr)
{
}

void pyxis_bw_writew(unsigned short b, unsigned long addr)
{
}

void pyxis_bw_writel(unsigned int b, unsigned long addr)
{
}

void pyxis_bw_writeq(unsigned long b, unsigned long addr)
{
}

unsigned long pyxis_dense_mem(unsigned long addr)
{
	return 0;
}
#endif 
int
pyxis_hose_read_config_byte (u8 bus, u8 device_fn, u8 where, u8 *value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
pyxis_hose_read_config_word (u8 bus, u8 device_fn, u8 where, u16 *value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
pyxis_hose_read_config_dword (u8 bus, u8 device_fn, u8 where, u32 *value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
pyxis_hose_write_config_byte (u8 bus, u8 device_fn, u8 where, u8 value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
pyxis_hose_write_config_word (u8 bus, u8 device_fn, u8 where, u16 value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
pyxis_hose_write_config_dword (u8 bus, u8 device_fn, u8 where, u32 value,
			       struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

void __init
pyxis_enable_errors (void)
{
}

int __init
pyxis_srm_window_setup (void)
{
	return 1;
}

void __init
pyxis_native_window_setup(void)
{
}

void __init
pyxis_finish_init_arch(void)
{
}

void __init
pyxis_init_arch(unsigned long *mem_start, unsigned long *mem_end)
{
}

void
pyxis_machine_check(unsigned long vector, unsigned long la_ptr,
		    struct pt_regs * regs)
{
}
