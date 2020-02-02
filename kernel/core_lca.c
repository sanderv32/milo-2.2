/*
 *	linux/arch/alpha/kernel/core_lca.c
 *
 * Code common to all LCA core logic chips.
 *
 * Written by David Mosberger (davidm@cs.arizona.edu) with some code
 * taken from Dave Rusling's (david.rusling@reo.mts.dec.com) 32-bit
 * bios code.
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>

#define __EXTERN_INLINE inline
#include <asm/io.h>
#include <asm/core_lca.h>
#undef __EXTERN_INLINE

int
lca_hose_read_config_byte (u8 bus, u8 device_fn, u8 where, u8 *value,
			   struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
lca_hose_read_config_word (u8 bus, u8 device_fn, u8 where, u16 *value,
			   struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
lca_hose_read_config_dword (u8 bus, u8 device_fn, u8 where, u32 *value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
lca_hose_write_config_byte (u8 bus, u8 device_fn, u8 where, u8 value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
lca_hose_write_config_word (u8 bus, u8 device_fn, u8 where, u16 value,
			    struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
lca_hose_write_config_dword (u8 bus, u8 device_fn, u8 where, u32 value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

void __init
lca_init_arch(unsigned long *mem_start, unsigned long *mem_end)
{
}

void
lca_machine_check (unsigned long vector, unsigned long la,
		   struct pt_regs *regs)
{
}

