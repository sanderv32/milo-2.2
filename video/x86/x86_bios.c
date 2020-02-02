#define __KERNEL__
#include <linux/pci.h>
#include "x86_bios.h"

extern int printk(const char *fmt, ...);

void (*bios_intr_tab[256]) (SysEnv * m, int intno);


static void undefined_intr(SysEnv * m, int intno)
{
	printk("x86_bios: interrupt %#x is undefined!\n", intno);
}


static void int10(SysEnv * m, int intno)
{
	if (m->x86.R_AH == 0x12 && m->x86.R_BL == 0x32) {
		if (m->x86.R_AL == 0) {
			/* enable CPU accesses to video memory */
			sys_outb(m, 0x3c2, sys_inb(m, 0x3cc) | 0x02);
		} else if (m->x86.R_AL == 1) {
			/* disable CPU accesses to video memory */
			sys_outb(m, 0x3c2, sys_inb(m, 0x3cc) & ~0x02);
		} else {
			printk
			    ("x86_bios.int10: unknown function AH=0x12, BL=0x32, AL=%#02x\n",
			     m->x86.R_AL);
		}
	} else {
		printk
		    ("x86_bios.int10: unknown function AH=%#02x, BL=%#02x\n",
		     m->x86.R_AH, m->x86.R_BL);
	}
}


static void int1a(SysEnv * m, int intno)
{
        struct pci_dev *dev = NULL;
     //   int cnt = 0;

	printk("x86_bios.int1a: called function AX=%#04x\n",
	       m->x86.R_AX);

	switch (m->x86.R_AX) {
	case 0xb101:		/* pci bios present? */
		m->x86.R_EAX = 0x00;	/* no config space/special cycle generation support */
		m->x86.R_AL = 0x01;
		
		m->x86.R_EDX = 0x20494350;	/* " ICP" */
		m->x86.R_EBX = 0x0210;	/* version 2.10 */
		m->x86.R_ECX &= 0xFF00;
		m->x86.R_CL = 0;	/* max bus number in system FIXME! */
		CLEAR_FLAG(m, F_CF);
		break;

	case 0xb102:		/* find pci device */
/*
		dev = pci_find_device(m->x86.R_DX, m->x86.R_CX, dev);
//		while ((dev = pci_find_device(m->x86.R_DX, m->x86.R_CX, dev)))
		if (m->x86.R_SI == 2) {
				m->x86.R_BH = dev->bus->number;
				m->x86.R_BL = dev->devfn;
				m->x86.R_AH = PCIBIOS_SUCCESSFUL;
		} else 
			m->x86.R_AH =PCIBIOS_DEVICE_NOT_FOUND;
*/

 		m->x86.R_AH =
		    pcibios_find_device(m->x86.R_DX, m->x86.R_CX,
					m->x86.R_SI-2, &m->x86.R_BH,
					&m->x86.R_BL);


		if (m->x86.R_AH == PCIBIOS_SUCCESSFUL) 
			printk ("  pci device vid=0x%04x did=0x%04x found on bus 0x%02x devfn 0x%02x\n",
				m->x86.R_DX, m->x86.R_CX, m->x86.R_BH, m->x86.R_BL);
		else
			printk ("  device not found on bus 0x%02x devfn 0x%02x. AH=0x%04x, SI=0x%04x \n",
				m->x86.R_BH, m->x86.R_BL, m->x86.R_AH, m->x86.R_SI);

		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb103:		/* find pci class code */
		m->x86.R_AH =
		    pcibios_find_class(m->x86.R_ECX, m->x86.R_SI,
				       &m->x86.R_BH, &m->x86.R_BL);
		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb108:		/* read configuration byte */
		m->x86.R_AH =
		    pci_read_config_byte(pci_find_slot((unsigned int)m->x86.R_BH,(unsigned int) m->x86.R_BL),
					     m->x86.R_DI, &m->x86.R_CL);
		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb109:		/* read configuration word */
		m->x86.R_AH =
		    pci_read_config_word(pci_find_slot((unsigned int)m->x86.R_BH, (unsigned int)m->x86.R_BL),
					     m->x86.R_DI, &m->x86.R_CX);
		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb10a:		/* read configuration dword */
		m->x86.R_AH =
		    pci_read_config_dword(pci_find_slot((unsigned int)m->x86.R_BH, (unsigned int)m->x86.R_BL),
					      m->x86.R_DI, &m->x86.R_ECX);

	printk ("  pci_read_dword bus=0x%02x devfn 0x%02x. DI=0x%04x, ECX=0x%08x \n",
				m->x86.R_BH, m->x86.R_BL, m->x86.R_DI, m->x86.R_ECX);

		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb10b:		/* write configuration byte */
		m->x86.R_AH =
		    pci_write_config_byte(pci_find_slot((unsigned int)m->x86.R_BH, (unsigned int)m->x86.R_BL),
					      m->x86.R_DI, m->x86.R_CL);
	

		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb10c:		/* write configuration word */
		m->x86.R_AH =
		    pci_write_config_word(pci_find_slot((unsigned int)m->x86.R_BH, (unsigned int)m->x86.R_BL),
					      m->x86.R_DI, m->x86.R_CX);
		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	case 0xb10d:		/* write configuration dword */
		m->x86.R_AH =
		    pci_write_config_dword(pci_find_slot((unsigned int)m->x86.R_BH, (unsigned int)m->x86.R_BL),
					       m->x86.R_DI, m->x86.R_ECX);
		CONDITIONAL_SET_FLAG((m->x86.R_AH != PCIBIOS_SUCCESSFUL),
				     m, F_CF);
		break;

	default:
		printk("x86_bios.int1a: unknown function AX=%#04x\n",
		       m->x86.R_AX);
	}
}


void x86_bios_init(SysEnv * m)
{
	int i;

	for (i = 0; i < 256; ++i) {
		((u32 *) m->mem_base)[i] = BIOS_SEG << 16;
		bios_intr_tab[i] = undefined_intr;
	}
	bios_intr_tab[0x10] = int10;
	bios_intr_tab[0x1a] = int1a;
}
