/*
 *	linux/arch/alpha/kernel/core_apecs.c
 *
 * Code common to all APECS core logic chips.
 *
 * Rewritten for Apecs from the lca.c from:
 *
 * Written by David Mosberger (davidm@cs.arizona.edu) with some code
 * taken from Dave Rusling's (david.rusling@reo.mts.dec.com) 32-bit
 * bios code.
 */
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/ptrace.h>

#define __EXTERN_INLINE inline
#include <asm/io.h>
#include <asm/core_apecs.h>
#undef __EXTERN_INLINE

#define vuip	volatile unsigned int  *

/*
volatile unsigned int apecs_mcheck_expected = 0;
volatile unsigned int apecs_mcheck_taken = 0;
*/

int
apecs_hose_read_config_byte (u8 bus, u8 device_fn, u8 where, u8 *value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
apecs_hose_read_config_word (u8 bus, u8 device_fn, u8 where, u16 *value,
			     struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
apecs_hose_read_config_dword (u8 bus, u8 device_fn, u8 where, u32 *value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int
apecs_hose_write_config_byte (u8 bus, u8 device_fn, u8 where, u8 value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
apecs_hose_write_config_word (u8 bus, u8 device_fn, u8 where, u16 value,
			      struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

int 
apecs_hose_write_config_dword (u8 bus, u8 device_fn, u8 where, u32 value,
			       struct linux_hose_info *hose)
{
	return PCIBIOS_SUCCESSFUL;
}

void apecs_init_arch(unsigned long *mem_start, unsigned long *mem_end)
{
}

int
apecs_pci_clr_err(void)
{
	return 0;
}

void
apecs_machine_check(unsigned long vector, unsigned long la_ptr,
		    struct pt_regs * regs)
{
}
