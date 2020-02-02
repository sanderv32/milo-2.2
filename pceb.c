/* file:	alcor_init.c
 *
 * Copyright (C) 1993 by
 * Digital Equipment Corporation, Maynard, Massachusetts.
 * All rights reserved.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  of  the  terms  of  such  license  and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person.  No title to and  ownership of the  software is  hereby
 * transferred.
 *                          
 * The information in this software is  subject to change without notice
 * and  should  not  be  construed  as a commitment by digital equipment
 * corporation.
 *
 * Digital assumes no responsibility for the use  or  reliability of its
 * software on equipment which is not supplied by digital.
 */

/*
 *++
 *  FACILITY:
 *
 *      Alpha Firmware for ALCOR
 *
 *  MODULE DESCRIPTION:     
 *
 *	ALCOR platform specific initialization routines.
 *
 *  AUTHORS:
 *
 *	Jim Hamel
 *
 *  CREATION DATE:
 *  
 *      10-Mar-1994
 *
 *  MODIFICATION HISTORY:
 *
 *	jrh	10-March-1994	    Brokeup ALCOR.C into more ordered modular breakdown
 *--
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

#if 0

/*+
 * ============================================================================
 * = platform_init1 - First platform specific initialization                  =
 * ============================================================================
 *
 * OVERVIEW:
 *
 *	Perform console initialization that is required by the platform.
 *	This routine should do any specific initialization for setting
 *	up the console data structures.
 *
 *	This routine is called as the first thing in KRN$_IDLE.
 *
 * FORM OF CALL:
 *
 *	platform_init1 (); 
 *
 * RETURNS:
 *
 *      Nothing
 *                          
 * ARGUMENTS:
 *
 *      None
 *
 * SIDE EFFECTS:
 *
 *      None
 *
-*/
platform_init1(void)
{
	unsigned __int64 csr_ptr;
	unsigned __int64 BaseCSR;
	struct SLOT *slot;
	int id;

	id = whoami();		/* get cpu id */

	/* Get the CPU speed in Mhz from the srom mailbox */
	cpu_mhz = srom_mbx_ptr->srom_r_platform.plt_q_cpu_mhz[0];

	/*  Initialize ALCOR specific registers .... */

	/* set mode in the CIA cnfg reg  jfr IOA_BWEN        0x1 PCI_MWEN
	 *        0x10 PCI_DWEN        0x20 PCI_WLEN        0x100 */
	BaseCSR = CIA_MAIN_CSRS;
	csr_ptr = CSR_L(CSR_cia_cnfg);
	*((unsigned long *) csr_ptr) = 0x21;


	mb();

	/* read cia revision */
	csr_ptr = CSR_L(CSR_cia_rev);
	cia_asic_revision = read_cia_csr(csr_ptr);


	/* reset the PCI */
	csr_ptr = CSR_L(CSR_cia_ctrl);
	*((unsigned long *) csr_ptr) = 0x0;
	mb();

/*krn$_sleep( 10 ); *//*  milliseconds */
	krn$_micro_delay(1000);

	/* deassert reset on PCI and then init CIA control                      */
	/*                                                                      */
	/* NOTE .... EN_ARB_LINK, PCI PERR and PCI ADDR_PE is DISABLED ...      */
	/*                                                                      */
	/* Fix for bug on EV5 below pass 4.                                     */
	/* ==================================================================== */
	/* Disable reporting of CPU_PE, and MEM_NEM errors, 8-feb-1995.         */
	/*                                                                      */

	slot = (struct SLOT *) (	/* get the slot pointer     */
				       HWRPB +
				       *((struct HWRPB *) HWRPB)->
				       SLOT_OFFSET +
				       (id * sizeof(struct SLOT)));

	/* EV5 pass 4 is identified by a 5 in the minor cpu_type field of the   */
	/* cpu slot.                                                            */
	/*                                                                      */
	if (slot->CPU_TYPE[1] < 5) {
		if (cia_asic_revision == 2)
			*((unsigned long *) csr_ptr) = 0x2104d8f5;
		else
			*((unsigned long *) csr_ptr) = 0x2104d8f1;
	} else {
		*((unsigned long *) csr_ptr) = 0x2104dcf1;
	}
	mb();


#if MODULAR
	xdelta_shared_adr = 0;
#endif

	srom_put_char('I');
	srom_put_char('.');

	csr_ptr = CSR_L(0x80);
	srom_printf("\nrev     %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0xc0);
	srom_printf("lat     %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x100);
	srom_printf("ctrl    %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x140);
	srom_printf("cnfg    %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x400);
	srom_printf("hae_mem %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x440);
	srom_printf("hae_io  %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x480);
	srom_printf("cfg     %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x600);	/* cack */
	srom_printf("cack_en %x\n", read_cia_csr(csr_ptr));
	BaseCSR = CIA_MEMO_CSR;
	csr_ptr = CSR_L(0x0);
	srom_printf("mcr  %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x600);
	srom_printf("mba0 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x680);
	srom_printf("mba2 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x700);
	srom_printf("mba4 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x780);
	srom_printf("mba6 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x800);
	srom_printf("mba8 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x880);
	srom_printf("mbaA %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x900);
	srom_printf("mbaC %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0x980);
	srom_printf("mbaE %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0xb00);
	srom_printf("tmg0 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0xb40);
	srom_printf("tmg1 %x\n", read_cia_csr(csr_ptr));
	csr_ptr = CSR_L(0xb80);
	srom_printf("tmg2 %x\n", read_cia_csr(csr_ptr));


	/* dismiss any pending errors */
	csr_ptr = CSR_L(CSR_cia_err);
	*((unsigned long *) csr_ptr) = 0xfff;
	mb();

/* initialize controllers */
	dr_controllers = 0;
	er_controllers = 0;
	ew_controllers = 0;
	fr_controllers = 0;
	fw_controllers = 0;
	pk_controllers = 0;
	pu_controllers = 0;
	vg_controllers = 0;
	no_controllers = 0;
	isa_eisa_count = 0;

}

#endif

/*+
 * ============================================================================
 * = platform_init2 - Second platform specific initialization                  =
 * ============================================================================
 *
 * OVERVIEW:
 *
 *	Perform console initialization that is required by the platform.
 *	This routine should do any specific initialization for setting
 *	up the console data structures.
 *
 *	This routine is called in KRN$_IDLE after some memory/variable 
 *      initialization has occured. 
 *
 * NOTE: I am not too sure why this code needs to be placed here rather than 
 *       the routine platform_init1();
 *
 * FORM OF CALL:
 *
 *	platform_init2 (); 
 *
 * RETURNS:
 *
 *      Nothing
 *                          
 * ARGUMENTS:
 *
 *      None
-*/
#define incfgb(a,v)  pcibios_read_config_byte(0,PCI_DEVFN(7,0),(a),(v))
#define outcfgb(a,v) pcibios_write_config_byte(0,PCI_DEVFN(7,0),(a),(v))
#define outcfgw(a,v) pcibios_write_config_word(0,PCI_DEVFN(7,0),(a),(v))
#define outcfgl(a,v) pcibios_write_config_dword(0,PCI_DEVFN(7,0),(a),(v))

void pceb_init(void)
{
	unsigned char revision;
	unsigned char pac;
	unsigned char temp_clk_div;

#if 0
	printk("pceb_init: starting...\n");
#endif

	/*   Initialize the PCEB       IF this is greater than revision 2 then     
	 *     read the PCI control and or in the IACK enable      setup MLT to max
	 * value      Write EPMRA register to enable buffering in region 0 PCI mem 
	 * region      Write MEM region addresses to dedode to 4 Gigabyte range */
	incfgb(0x8, &revision);
	revision &= 0x0f;

	/* Read pci control         */
	incfgb(0x40, &pac);

	/* If Rev 3 of pceb chip...bit 2    */
	/* posted write buffers are not     */
	/* supported and must be clear      */
	/* Enable Line Buffers              */
	/* slow sample point-default                */
	if (revision > 2) {
		pac &= ~(2);
		pac |= 0x40;
		outcfgb(0x40, pac);
	}


	/* master latency timer setup */
	outcfgb(0x000d, 0xf8);

	/* Set up the PCI arbiter Control   */
	/* 16 pciclks, bus park, gat ON     */
	outcfgb(0x0041, 0x8d);

	/* Priority of arbitration          */
	outcfgb(0x42, 0xf0);

	/* Memory Chip Select Control       */
	/* Disable BIOS 80000 to F0000      */
	/* Enable Master Chip Select        */
	outcfgb(0x44, 0x10);

	/* Bottom of Hole--disabled         */
	outcfgb(0x45, 0x00);

	/* Top of Hole--disabled            */
	outcfgb(0x46, 0x00);

	/* Top of Memory                    */
	outcfgb(0x47, 0x00);

	/* Eisa Address Decode               */
	/* Low memory .... enable 0-896kbyte */
	outcfgw(0x48, 0xff07);

	/* Memory Chip Selects              */
	outcfgb(0x54, 0x00);
	outcfgb(0x55, 0x00);
	outcfgb(0x56, 0x00);

	/* positive decode 8259's */
	outcfgb(0x0058, 0x20);

	/* Enable line buffers in the PCEB */
	outcfgb(0x005c, 0x0f);

	/* Set up PCI to EISA window ... */
	outcfgl(0x0060, 0x7fff0000);
	outcfgl(0x0064, 0xffffc000);
	outcfgl(0x0068, 0x7fff0000);
	outcfgl(0x006c, 0x7fff0000);

	/* same for I/O space */
	outcfgl(0x70, 0x7fff0000);
	outcfgl(0x74, 0x7fff0000);
	outcfgl(0x78, 0x7fff0000);
	outcfgl(0x7c, 0x7fff0000);


	/*  Initialize the ESC chip ..... */

	/* write ESCID register with a 0f so we con program config regs */
	outw(0x0f02, 0x22);

	/* set up EISA clock divisor for divide by 4 ( 33/4 = 8.25 MHz EISA) */

	/* read current value of Eisa Clock divisor */
	outb(0x4d, 0x22);
	temp_clk_div = inb(0x23);


	/* clear out clock divisor bits to divide by 4 */
	temp_clk_div &= 0xf8;

	/* write out new value to ESC */
	outb(0x4d, 0x22);
	outb(temp_clk_div, 0x23);

	/* write mode select so we have 8 eisa masters, SERR enabled, CFG RAM
	 * enable */
	outw(0x6740, 0x22);

	/* disable access to motherboard BIOS */
	outw(0x0042, 0x22);
	outw(0x0043, 0x22);

	/* write peripheral CSa so we can talk to RTC, Keyboard */
	outw(0x034e, 0x22);

	/* write peripheral CSb so we can talk to Config RAM */
	outw(0xff4f, 0x22);


	/* write out the EISA motherboard ID ... 10a35100 */
	outw(0x1050, 0x22);
	outw(0xa351, 0x22);
	outw(0x5152, 0x22);
	outw(0x0053, 0x22);

#if 0

	/* set up GPCS0 to address 530h IIC bus for OCP */
	outw(0x3064, 0x22);
	outw(0x0565, 0x22);
#endif

	/* set up GPCS0 to address 604h IIC bus for OCP */
	outw(0x0464, 0x22);
	outw(0x0665, 0x22);

	/* Setup mask bits for GPCS0 ... IIC will be selected for address 530-531h
	 */
	outw(0x0166, 0x22);

	/* allow XBUSOE to occur on GPCS0 (IIC controller) GPCS1,2 are disabled */
	outw(0x016f, 0x22);

	/* reinitialize the Super IO (87312) chip */
	inb(0x398);
	inb(0x398);
	outb(0, 0x398);
	outb(0xf, 0x399);
	outb(0xf, 0x399);

	/* reinitialize the COMM port for output .... */

	/* set baud rate to 9600 baud ... */
	outb(0x80, 0x3fb);
	outb(0x0c, 0x3f8);
	outb(0x00, 0x3f9);

	/* set char width etc .... */
	outb(0x3, 0x3fb);

	/* enable fifo */
	outb(0x7, 0x3fa);

#if 0
	printk("pceb_init: finished...\n");
#endif

}
#if 0

/*+
 * ============================================================================
 * = initialize_hardware - initialize hardware                                =
 * ============================================================================
 *
 * OVERVIEW:
 *
 *	Perform hardware initialization on the ALCOR
 *
 *	This routine is called in KRN$_IDLE after the dynamic memory data  
 *      structure initialization has occured. The routine pci_size_config.c
 *      uses malloc and free so this initialization needs to occur 
 *
 * FORM OF CALL:
 *
 *	initialize_hardware (); 
 *
 * RETURNS:
 *
 *      Nothing
 *                          
 * ARGUMENTS:
 *
 *      None
-*/
initialize_hardware()
{

	unsigned __int64 csr_ptr;
	unsigned __int64 BaseCSR;
	struct SLOT *slot;
	int id;

	id = whoami();

	/*  Initialize ALCOR specific registers .... */

	BaseCSR = CIA_MAIN_CSRS;

	/* Set up PCI latency to be full latency */
	csr_ptr = CSR_L(CSR_pci_lat);
	*((unsigned long *) csr_ptr) = 0xff00;
	mb();

	/* Set up CACK enable to only CACK BC_Victim */
	/*  csr_ptr = CSR_L(CSR_cack_en);  *((unsigned long *) csr_ptr) = 0x8;
	 *  mb(); */

	/* make sparse memory regions 0,1 contiguous starting at address 2 gigabyte
	 * sparse region 2 is used for EISA ... so it is set to a 0 */
	csr_ptr = CSR_L(CSR_hae_mem);
	*((unsigned long *) csr_ptr) = 0x8000a000;
	mb();

	/* make both I/O regions contiguous ... regions 1 starts at 32 Mbyte */
	csr_ptr = CSR_L(CSR_hae_io);
	*((unsigned long *) csr_ptr) = 0x2000000;
	mb();

	/* dismiss any pending errors */
	csr_ptr = CSR_L(CSR_cia_err);
	*((unsigned long *) csr_ptr) = 0xffff;
	mb();

	/* get pointer to the cia err mask register */
	csr_ptr = CSR_L(CSR_cia_err_mask);

	/* enable reporting of errors...except pci parity                       */
	/*                                                                      */
	/* Fix for bug on EV5 below pass 4.                                     */
	/* ==================================================================== */
	/* Disable reporting of CPU_PE, and MEM_NEM errors, 8-feb-1995.         */
	/*                                                                      */

	slot = (struct SLOT *) (	/* get the slot pointer     */
				       HWRPB +
				       *((struct HWRPB *) HWRPB)->
				       SLOT_OFFSET +
				       (id * sizeof(struct SLOT)));

	/* EV5 pass 4 is identified by a 5 in the minor cpu_type field of the   */
	/* cpu slot.                                                            */
	/*                                                                      */
	/* 24-jul-1995                                                          */
	/* -------------------------------------------------------------------- */
	/* we don't want to see perrs, because some devices give bogus pty      */
	/*  if (slot->CPU_TYPE[1] < 5) */
	/* just ignore errors for now, turn them on in powerup_alcor */
	{
		*((unsigned long *) csr_ptr) = 0;
		/*0xf93; */
	}
/*
    else
	{
        *((unsigned long *) csr_ptr) = 0xf9f;
	}
*/
	mb();


	/*  invalidate all TB entries */

	BaseCSR = CIA_XLAT_CSR;
	csr_ptr = CSR_L(CSR_tbia);
	*((unsigned long *) csr_ptr) = 3;
	mb();

	/*  Window 0 will be:  PCI Address 0x800000  Size of 8Megabyte   point to
	 * 8k page in console */

	csr_ptr = CSR_L(CSR_w_mask0);
	*((unsigned long *) csr_ptr) = wmask_k_sz8mb;
	mb();

	csr_ptr = CSR_L(CSR_t_base0);
	*((unsigned long *) csr_ptr) = WIN0_SG_TABLE;
	mb();


	csr_ptr = CSR_L(CSR_w_base0);
	*((unsigned long *) csr_ptr) = 0x800000 | wbase_m_w_en |
	    wbase_m_sg | wbase_m_memcs_enable;
	mb();

	/*  Window 1 will be:  PCI Address 0x40000000 (1 gigabyte)  Size of 1
	 * gigabyte  point to memory address 0 */

	csr_ptr = CSR_L(CSR_w_mask1);
	*((unsigned long *) csr_ptr) = wmask_k_sz1gb;
	mb();

	csr_ptr = CSR_L(CSR_t_base1);
	*((unsigned long *) csr_ptr) = 0x0;
	mb();

	csr_ptr = CSR_L(CSR_w_base1);
	*((unsigned long *) csr_ptr) = 0x40000000 | wbase_m_w_en;
	mb();


	/*  Window 2 and 3 will be disabled */
	csr_ptr = CSR_L(CSR_w_base2);
	*((unsigned long *) csr_ptr) = 0;
	mb();

	csr_ptr = CSR_L(CSR_w_base3);
	*((unsigned long *) csr_ptr) = 0;
	mb();


	/* initialize interrupt controller registers */
	BaseCSR = GRU_ROM_CSR;

	/* clear all interrupts */
	csr_ptr = CSR_L(CSR_int_clr);
	*((unsigned long *) csr_ptr) = 0xffffffff;
	mb();

	*((unsigned long *) csr_ptr) = 0x0;
	mb();

	/* Set all interrupts as level sensitive */
	csr_ptr = CSR_L(CSR_int_edge);
	*((unsigned long *) csr_ptr) = 0;
	mb();

	/* Set all interrupts as level sensitive */
	csr_ptr = CSR_L(CSR_int_hilo);
	*((unsigned long *) csr_ptr) = 0x80000000;
	mb();

	csr_ptr = CSR_L(CSR_int_msk);
/*    *((unsigned long *) csr_ptr) = 0x800fffff;*/

	/* enable eisa interrupts in the GRU */
	*((unsigned long *) csr_ptr) = 0x80000000;
	mb();



	outportb(0, 0x61, inportb(0, 0x61) | 0x04);
	outportb(0, 0x61, inportb(0, 0x61) & 0xfb);

	/*  Initialize the embedded 8259's */
	outportb(0, 0xa0, 0x11);	/* ICW1 ... need a ICW4 */
	outportb(0, 0xa1, 0x08);	/* ICW2 ... vector base of 8h */
	outportb(0, 0xa1, 0x02);	/* ICW3 ... Cascaded to IRQ of master
					 */
	outportb(0, 0xa1, 0x01);	/* ICW4 ... 8086 , normal EOI */
	outportb(0, 0x4d1, 0x00);
	outportb(0, 0xa1, 0xff);	/* OCW1 ... interrupts are disabled */


	outportb(0, 0x20, 0x11);	/* ICW1 ... need a ICW4 */
	outportb(0, 0x21, 0x00);	/* ICW2 ... Vector base of a 0h */
	outportb(0, 0x21, 0x04);	/* ICW3 ... Controller connected to
					 * IRQ2 */
	outportb(0, 0x21, 0x01);	/* ICW4 ... 8086 , normal EOI */
	outportb(0, 0x4d0, 0x00);
	outportb(0, 0x21, 0xfB);	/* OCW1 ... interrupts IRQ2 enabled */



	/*  Setup interrupt handlers for the system .... */
	int_vector_set(SCB$Q_SLE, do_bpt, 0);
	int_vector_set(SCB$Q_SYS_CORR_ERR, mcheck_handler_620,
		       SCB$Q_SYS_CORR_ERR);
	int_vector_set(SCB$Q_PR_CORR_ERR, mcheck_handler_630,
		       SCB$Q_PR_CORR_ERR);
	int_vector_set(SCB$Q_SYS_MCHK, mcheck_handler, SCB$Q_SYS_MCHK);
	int_vector_set(SCB$Q_PR_MCHK, mcheck_handler, SCB$Q_PR_MCHK);
	int_vector_set(SCB$Q_6F0, alcor_unexpected_int, 0);
	mchq_660.flink = &mchq_660;
	mchq_660.blink = &mchq_660;
}

/*+
 * ============================================================================
 * = reset_hardware - reset hardware                                =
 * ============================================================================
 *
 * OVERVIEW:
 *
 *	Perform hardware reset on the ALCOR
 *
 * FORM OF CALL:
 *
 *	reset_hardware (log); 
 *
 * RETURNS:
 *
 *      Nothing
 *                          
 * ARGUMENTS:
 *
 *      log - ???
-*/
reset_hardware(int log)
{

	tga_reinit(0);		/* Reinit the TGA but do not clear the 
				 * screen */
	vga_reinit();

	/* disable all option interrupts */
	outportb(0, 0xa1, 0xff);

	/* disable keyboard interrupts to get rid of stray */
	io_disable_interrupts(0, KEYBD_VECTOR);
	kbd_reinit();
}

#endif
