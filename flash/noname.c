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
 *  Utility that the miniloader can load to update the flash (with new miniloaders?).
 *
 *  david.rusling@reo.mts.dec.com
 */
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/string.h>
#include <asm/system.h>
#include <asm/io.h>

#include "milo.h"
#include "flash.h"

#ifdef MINI_ALPHA_NONAME
/*****************************************************************************
 *  Build Controlling Definitions.                                           *
 *****************************************************************************/
/*
 *  These are not in config.in as they'd clutter things up too much.  So, they
 *  are here.
 */
/* DEBUG_FLASH: this turns on much more printk() statements.  Defining this
 * *may* help you find a problem */
#undef DEBUG_FLASH 1

/*****************************************************************************/


#define TRUE 1
#define FALSE 0

/* The directive defines the offset at which we should switch	*/
/* a bit in a register in order to access the rest of the flash.*/
#define FLASH_BIT_SWITCH_OFFSET 0x80000

#define MAX_VERIFY_ERRORS 10	/* Max number of errors before flash */
					/* utility quits verifying data */
#define MAX_RETRIES   3		/* Number of retries. */
#define DEFAULT_SRC 0x300000	/* source stars at 3 MEG */

/* Establish base address of System ROM (Flash) */
unsigned int f_rom_base = 0xfff80000;

/*--------------------------------------------------------------------------*/
/*									    */
/* How to write to the AXPPCI33 Flash: A tutorial				    */
/*									    */
/* The AXPPCI33 flash roms are accessed using the WARTBUS at address	    */
/* 1.D000.0200.  The code is able to access 1 byte at a time via the	    */
/* following longword:							    */
/*									    */
/*       DDDD.DDDD.XXXX.XXAA.AAAA.AAAA.AAAA.AAAA			    */
/*	 |||| |||| |||| |||| |||| |||| |||| ||||			    */
/*	 |||| |||| ||||	||\\ \\\\ \\\\ \\\\ \\\\			    */
/*	 |||| |||| |||| || \\ \\\\ \\\\ \\\\ \\\\			    */
/*	 |||| |||| |||| ||  \\ \\\\ \\\\ \\\\ \\\\			    */
/*	 |||| |||| |||| ||   \\ \\\\ \\\\ \\\\ \\\\			    */
/*	 |||| |||| |||| ||    =========================			    */
/*	 |||| |||| |||| ||    Bits 17:00 are the flash			    */
/*	 |||| |||| |||| ||    rom physical address from			    */
/*	 |||| |||| |||| ||    a base addr of 0.				    */
/*	 |||| |||| |||| ||    =========================			    */
/*	 |||| |||| |||\ \\						    */
/*	 |||| |||| ||| \ \\						    */
/*	 |||| |||| |||  \ \\						    */
/*	 |||| |||| |||   \ \\						    */
/*	 |||| |||| |||   =======================================	    */
/*	 |||| |||| |||   Bits 20:18  are the chip selects		    */
/*	 |||| |||| |||   for the flash roms.  On a AXPPCI33 or UTD	    */
/*	 |||| |||| |||   board the following table shows		    */
/*	 |||| |||| |||   the correspondence:				    */
/*	 |||| |||| |||      bits        AXPPCI33             UTD		    */
/*	 |||| |||| |||       000: 128K flash(w/mirror)	 uverom socket	    */
/*	 |||| |||| |||       001: 256K flash #1		 uverom socket	    */
/*	 |||| |||| |||       010: 256K flash #2		 256K flash	    */
/*	 |||| |||| |||       011: 256K flash #3		 256K flash	    */
/*	 |||| |||| |||       100: jumpers		 jumpers	    */
/*	 |||| |||| |||       101:					    */
/*	 |||| |||| |||       110:	    		 LEDS		    */
/*	 |||| |||| |||       111:					    */
/*	 |||| |||| |||   =============================================	    */
/*       |||| |||| ||\							    */
/*	 |||| |||| || \							    */
/*	 |||| |||| |\  \====================================		    */
/*	 |||| |||| | \  bit 21 rom write enable				    */
/*	 |||| |||| \  \=====================================		    */
/*       |||| ||||  \  bit 22 rom output enable				    */
/*	 |||| ||||   \======================================		    */
/*	 |||| ||||    bit 23 wartbus buffering off (off = 1)		    */
/*	 |||| ||||   =======================================		    */
/*	 \\\\ \\\\							    */
/*	  \\\\ \\\\							    */
/*	   ========================					    */
/*	   bits 31-24 are the byte					    */
/*	   of data or command to be					    */
/*	   to the flash roms						    */
/*	   ========================					    */
/*									    */
/*   There are various modes into which the flash can be put.  The flash    */
/*   can be put in these modes using the following commands:		    */
/*									    */
/*		 ID_ROM              0x90				    */
/*		 ERASE_CMD           0x20				    */
/*		 ERASE_VERIFY_CMD    0xA0				    */
/*		 WRITE_CMD           0x40				    */
/*		 WRITE_VERIFY_CMD    0xC0				    */
/*		 RESET		     0xFF				    */
/*		 READ_CMD	     0x00				    */
/*									    */
/*    To write a command out the following sequence must be followed	    */
/*    (using reset to flash 0 as an example):				    */
/*									    */
/*        write address and command          FF000000			    */
/*	  write addr and command w/WrEn      FF200000			    */
/*	  wait a short while						    */
/*	  write address and command (noWrEn) FF000000			    */
/*									    */
/*	  All of the above write were to 1.D000.0200			    */
/*									    */
/*     Commands like RESET and ERASE have set-up/command sequences where    */
/*     you have to pump the command twice to the device to actually	    */
/*     activate the command.  Writing a byte to a location in the flash	    */
/*     is similar to writing a command once the write command has been	    */
/*     sent to the flash.						    */
/*									    */
/*     Reads on the other hand follow the following sequence (once	    */
/*     the flash is put into READ mode by writing out the read command):    */
/*									    */
/*        write address			       FF000000			    */
/*	  write addr w/OutEn and w/BfOff       FF600000			    */
/*	  wait a short while						    */
/*	  read port and the byte should be at bits 31:24		    */
/*									    */
/*	  All of the above writes/read were to 1.D000.0200		    */
/* -------------------------------------------------------------------------*/

/* -------------------------------------------------------------------------*/
/* Flash rom bit and command defintions					    */
/*									    */
/* bits									    */
#define rom_we 1<<21
#define rom_oe 1<<22
#define bb_off 1<<23
#define MFG_ID_OFFSET  0
#define DEV_ID_OFFSET  1
/*									    */
/*flash commands							    */
/*									    */
#define ID_ROM              (unsigned int) 0x90
#define ERASE_CMD           (unsigned int) 0x20
#define ERASE_VERIFY_CMD    (unsigned int) 0xA0
#define WRITE_CMD           (unsigned int) 0x40
#define WRITE_VERIFY_CMD    (unsigned int) 0xC0
#define RESET_CMD           (unsigned int) 0xFF
#define READ_CMD            (unsigned int) 0x00

/*     								            */
/*miscellaneous definitions						    */
/*									    */
#define FLASHBASE (unsigned long)0xfffffc0038000000
					/* kept to 7 bits so that ulong can be 
					 * created in routines*/
#define FLASH_SIZE_512K      0x80000	/* 256K part 27F020 */
#define FLASH_SIZE_256K      0x40000	/* 256K part 27F020 */
#define FLASH_SIZE_128K      0x20000	/* 256K part 27F020 */

/* forward declarations */
void flash(void);
#ifdef SERIAL_IO
int get_choice(void);
#endif
void get_flash_id(int rom_port, int *mfg_id, int *dev_id);
void fl_wait(int usecs);
int activate_vpp(void);
void deactivate_vpp(void);
#ifdef SERIAL_IO
void flash_dump(unsigned int rom_port);
#endif
void flash_program(unsigned int rom_port);

/* -----------------------------------------------------------------------------------------------------------------------*/

/*
 *  Add a noname header.  This is of the format:
 *
 * ------------------------------------------------------------------------------------------
 *  Size    Name  
 * ------------------------------------------------------------------------------------------
 *   1      Width
 *   1      Stride
 *   1      size
 *   1      filler
 *   4      pattern (55-00-aa-ff) 
 *   8      format, ascii (V1.0)
 *   8      vendor, ascii (DEC)
 *   8      module id, ascii (EFEAME)
 *   4      fw type, ascii (ALPH)
 *   4      flags
 *   1      code version
 *   1      filler 
 *   1      pass_2 flag  
 *   1      pass_3 flag
 *   4      code length
 *   12     ROM object name
 *   4      code edit
 *  
 * ------------------------------------------------------------------------------------------
 */
typedef struct nrh {
	char width;
	char stride;
	char size;
	char filler_1;
	unsigned int pattern;
	char format[8];
	char vendor[8];
	char module_id[8];
	char fw_type[4];
	unsigned int flags;
	char code_version;
	char filler_2;
	char pass_2_flag;
	char pass_3_flag;
	unsigned int code_length;
	char object_name[12];
	unsigned int code_edit;
} nrh_t;

extern char flash_image[];
extern unsigned int flash_image_size;

void *add_noname_flash_header(void *where)
{
	nrh_t *header = (nrh_t *) where;

	header->width = 1;
	header->stride = 1;
	header->size = 8;
	header->filler_1 = 0;

	header->pattern = 0xffaa0055;

	strcpy(header->format, "v.10");
	strcpy(header->vendor, "DEC");
	strcpy(header->module_id, "NONAME");
	strcpy(header->fw_type, "ALPH");

	header->flags = 0;
	header->code_version = 0;
	header->filler_2 = 0;
	header->pass_2_flag = 0;
	header->pass_3_flag = 0;

	header->code_length = flash_image_size;
	strcpy(header->object_name, "Miniloader");

	header->code_edit = 0x11223344;

#ifdef DEBUG_FLASH
	do {
		int i;
		char *ptr = (char *) where;

		for (i = 0; i < 70; i++)
			printk("0x%02x, ", *ptr++);
	} while (0);
#endif

	where = where + sizeof(nrh_t);
	return where;
}

/*
 *  Copy the raw data for the image someplace and add the appropriate header to it.
 */
unsigned long int bootadr;
void prepare_flash_image(void)
{
	extern unsigned long long _end;
	void *where;

	/* set up the image that we are going to load into flash. */
	where = (void *) (&_end + 8);
	where = (void *) ((unsigned long long) where & ~0x7);
	bootadr = (unsigned long int) where;

#ifdef DEBUG_FLASH
	printk("prepare_flash_image: adding board specific header\n");
#endif
	where = add_noname_flash_header(where);

#ifdef DEBUG_FLASH
	printk("prepare_flash_image: moving image to 0x%p\n", where);
#endif
	memmove(where, flash_image, flash_image_size);

	return;
}

#ifdef MINI_SERIAL_IO
int get_choice(void)
{
	int c, v;

	printk("Choose one of:\n");
	printk("  1: look at the contents of flash\n");
	printk("  2: program an image into flash\n");
	printk("  3: exit\n");
	printk("\n>");

	while (1) {
		c = GetChar();
		v = c - '0';
		if ((v > 0) && (v < 4)) {
			PutChar(c);
			PutChar('\n');
			return v;
		}
	}
}

#endif

/* ----------------------------------K-E-Y-B-O-A-R-D---R-O-U-T-I-N-E-S----------------------------------------------------*/
/*									    */
/* Keyboard controller definitions					    */
/*									    */
#define KBD_STATUS_REG      (unsigned int) 0x64
#define KBD_CNTL_REG        (unsigned int) 0x64
#define KBD_DATA_REG	    (unsigned int) 0x60
#define KBD_OBF		    (unsigned int) 0x01
#define KBD_IBF		    (unsigned int) 0x02
#define KBD_OUTPORT_ON	    (unsigned int) 0xc8
#define KBD_OUTPORT_OFF	    (unsigned int) 0xc7
#define KBD_SELF_TEST	    (unsigned int) 0xAA
#define KBD_READ_MODE	    (unsigned int) 0x20
#define KBD_WRITE_MODE	    (unsigned int) 0x60
#define KBD_WRITEABLE	    (unsigned int) 0x19
#define KBD_VPP_ON	    (unsigned int) 0x10
#define KBD_VPP_OFF	    (unsigned int) 0x10

void kbd_write(int address, int data)
{
	int status;

	status = inb(KBD_STATUS_REG);	/*get the input buffer status */
	while (status & KBD_IBF)
		status = inb(KBD_STATUS_REG);	/*spin until input buffer empty */
	outb(data, address);	/*write out the data */

}

/*+
 * ===================================================
 * = activate_vpp - tap kbd controller to send vpp to flash=
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     kbd controller must be touched to activate vpp on
 *     flash parts
 *  
 * FORM OF CALL:
 *  
 *     status = activate_vpp();
 *  
 *
 * RETURNS:
 *
 *     1 - success in accessing kbd controller
 *     0 - failure to activate Vpp via kbd controller
 *
 * ARGUMENTS:
 *
 *	none
 *
 * 
 *
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	This routine writes to the kbd controller to activate
 *	Vpp to the flash roms.  The reason this has to happen is
 *	that the kbd controller is a 12 volt device, and in PC
 *	world, the extra output port on the controller is used
 *	to drive 12 volts to the flash.
 *
 *
 *
 * CALLS:
 *
 *	inb - console isa bus device input routine
 *	outb - console isa bus device output routine
 *	printk - console supplied output rouine
 *	kbd_write - writes a command to the kbd controller
 *	fl_wait	- a usec timeout routine
 *
 *
 * CONSTANTS:
 *  
 *      KBD_STATUS_REG       0x64
 *      KBD_CNTL_REG         0x64
 *	KBD_DATA_REG	     0x60
 *	KBD_OBF		     0x01
 *	KBD_OUTPORT	     0xD1
 *	KBD_SELF_TEST	     0xAA
 *	KBD_WRITEABLE	     0x19
 *	KBD_VPP_ON	     0xFE
 *     
 *
 * LOCAL VARIABLES:
 *  
 *     datab - a longword containing a input byte from the kbd controller
 *     	
 *
 * ALGORITHM:
 *
 *    -Wake up the kbd controller by sending a self test command
 *    -Wait for the driver to finish operation by checking the OBF
 *    -Connect to the KBD output port
 *    -flip the line that is connected to flash Vpp
 *    -wait for the driver to finish operation by checking the OBF
 *    -wait for Vpp to settle and return
 *    [@tbs@]...
 *
--*/

int activate_vpp()
{
	unsigned int datab;
	unsigned int resultb;

#define armen
#ifdef armen

	/* now,  you have to enable Vpp to the roms by writing to */
	/* the keyboard controller                                */

	/* Tell the kbd controller to disable interrupts, turn off SYS mode */
	kbd_write(KBD_CNTL_REG, KBD_WRITE_MODE);
	kbd_write(KBD_DATA_REG, 0x60);

	kbd_write(KBD_CNTL_REG, KBD_SELF_TEST);
	/* turn on kbd controller */
	datab = (inb(KBD_STATUS_REG) & 0xff);
	/* wait for the output buffer to clear
	 */
	while (!(datab & KBD_OBF)) {
		datab = (inb(KBD_STATUS_REG) & 0xff);
		/* wait for self test to complete */
	}

	resultb = inb(KBD_DATA_REG) & 0xff;
	if (resultb == 0x55) {
		printk("Keyboard self-test succeeded\n");
	} else {
		printk
		    ("Keyboard self-test failed: result = 0x%x, expected 0x55\n",
		     resultb);
		return (0);
	}

	datab = inb(KBD_STATUS_REG & 0xff);
	while (datab & KBD_IBF) {
		datab = inb(KBD_STATUS_REG & 0xff);
	}
	;

	if ((datab & 0xfe) == 0x18) {
		printk("Controller on...\n");
	} else {
		printk("18/19 didn't come back = %x\n", datab);
		return (0);
	}
#endif
	datab = (inb(KBD_DATA_REG) & 0xff);	/*flush the input buffer */

	/* now that kbd controller is initialized to a known state, enable Vpp */

	kbd_write(KBD_CNTL_REG, KBD_OUTPORT_ON);
	/* activate the bit so that we can
	 * read*/
	datab = inb(KBD_STATUS_REG);
	while (datab & KBD_OBF) {	/* loop until the output buffer is
					   * empty */
		datab = inb(KBD_DATA_REG);	/* flush the buffer
						 *                   */
		datab = inb(KBD_STATUS_REG) & 0xFF;
		/*get the staus                 */
	}

	printk("Activate Programming...\n");
	kbd_write(KBD_DATA_REG, KBD_VPP_ON);
	/* FF means turn off and FE means turn 
	 * on */

	datab = inb(KBD_STATUS_REG) & 0xFF;	/*loop until outbuf status shows */
	while (datab & KBD_OBF) {	/*output buffer empty            */
		datab = inb(KBD_DATA_REG);	/* flush input buffer */
		datab = inb(KBD_STATUS_REG) & 0xFF;
		/* check outbuf status */
	}
	fl_wait(500000);	/* Let Vpp settle for a .5 sec */
	return (1);

}

/*+
 * ===================================================
 * = deactivate - tap kbd controller to not send vpp to flash=
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     kbd controller must be touched to deactivate vpp on
 *     flash parts
 *  
 * FORM OF CALL:
 *  
 *     deactivate();
 *  
 *
 * RETURNS:
 *
 *      none
 *
 * ARGUMENTS:
 *
 *	none
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	This routine writes to the kbd controller to deactivate
 *	Vpp to the flash roms.  The reason this has to happen is
 *	that the kbd controller is a 12 volt device, and in PC
 *	world, the extra output port on the controller is used
 *	to drive 12 volts to the flash.   once deactivated, only
 *	read operations can be done to the flash.
 *
 * CALLS:
 *
 *	inb - console isa bus device input routine
 *	outb - console isa bus device output routine
 *	printk - console supplied output rouine
 *	kbd_write - writes a command to the kbd controller
 *
 *
 * CONSTANTS:
 *  
 *      KBD_STATUS_REG       0x64
 *      KBD_CNTL_REG         0x64
 *	KBD_DATA_REG	     0x60
 *	KBD_OBF		     0x01
 *	KBD_OUTPORT	     0xD1
 *	KBD_VPP_OFF	     0xFF
 *     
 *
 * LOCAL VARIABLES:
 *  
 *     datab - a longword containing a input byte from the kbd controller
 *     	
 *
 * ALGORITHM:
 *
 *    -Connect to the KBD output port
 *    -deactivate the line that is connected to flash Vpp
 *    -wait for the driver to finish operation by checking the OBF
 *    [@tbs@]...
 *
--*/
void deactivate_vpp()
{
	unsigned int datab;

	kbd_write(KBD_CNTL_REG, KBD_OUTPORT_OFF);
	/* Set pointer to KBD output port */
	datab = inb(KBD_STATUS_REG);	/* wait for output buffer to settle */
	while (datab & KBD_OBF) {
		datab = inb(KBD_DATA_REG);	/* flush input */
		datab = inb(KBD_STATUS_REG);	/* get obf stat */
	}

	printk("Deactivating programming\n");

	kbd_write(KBD_DATA_REG, KBD_VPP_OFF);
	/* turn off vpp at kbd output   */
	/* port */

	datab = inb(KBD_STATUS_REG);	/* wait for OBF to clear */
	while (datab & KBD_OBF) {
		datab = inb(KBD_DATA_REG);	/*flush input */
		datab = inb(KBD_STATUS_REG);	/*GET obf status */
	}

}

/* -----------------------------------------------------------------------------------------------------------------------*/
#define USEC_DELAY 0x10

void fl_wait(int usecs)
{
	volatile int i, j;

	for (i = j = 0; i < USEC_DELAY * usecs; i++)
		j += i;
}

/*+
 * ===================================================
 * = flash_read - read a data byte from the flash    =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     This routine allows us to read data bytes
 *     from the flash. This is the lowest level
 *     routine here, save the outport itself, and contains
 *     the AXPPCI33 toggle the chip sels etc algorithm
 *  
 * FORM OF CALL:
 *  
 *     data = flash_read(rom_port,offset)
 *  
 *
 * RETURNS:
 *
 *	data - a longword whose byte 0 contains a data byte from the flash
 *
 * ARGUMENTS:
 *
  *	rom_port - converts into the chip sel
 *	
 *	offset - address off of flash address 0
 *       
 *       
 *
 * GLOBALS:
 *  
 *     none
 *
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *    This routine takes the two parameters and forms the
 *    longword that needs to be written to the flash roms via
 *    1.d000.0200 (see the tutorial in the module header).  It follows
 *    the prescribed algorithm for flipping OpEn and BBoff up and down and
 *    holding the address on the bus for the necessary length of time.
 *
 * RETURN CODES:
 *
 *	none
 *
 * CALLS:
 *
 *	outb - console support routine to do the write to ISA space
 *	inb	 - console support routine to do the read from ISA space
 *
 * CONSTANTS:
 *  
 *     WARTBUS_ADDR - address 1.d000.0200
 *
 * LOCAL VARIABLES:
 *  
 *     port_data - a longword which is put together to be output
 *
 * ALGORITHM:
 *
 *     Reads on the other hand follow the following sequence (once	    
 *     the flash is put into READ mode by writing out the read command):    
 *									    
 *        write address			       FF000000			    
 *	  write addr w/OutEn and w/BfOff       FF600000			    
 *	  wait a short while						    
 *	  read port and the byte should be at bits 31:24		    
 *									    
 *	  All of the above writes/read were to 1.D000.0200		    
 *
--*/



unsigned int flash_read(unsigned int rom_port,
			/*chip select */
			unsigned int offset)
{				/*flash offset from 0 */
	unsigned int data;
	unsigned long address = (unsigned long) FLASHBASE;

	address =
	    address + (rom_port * (FLASH_SIZE_256K * 8)) + (offset * 8);

	data = (unsigned int) (*((unsigned long *) address) & 0xff);

	mb();
	return (data);

}

/*+
 * ===================================================
 * = write_command - write a command byte to the flash=
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     This routine allows us to write specific 
 *     commands to the flash. This is the lowest level
 *     routine here, save the outport itself, and contains
 *     the AXPPCI33 toggle the chip sels etc algorithm
 *  
 * FORM OF CALL:
 *  
 *     write_command(rom_port,offset,command)
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *
 *	rom_port - converts into the chop selects
 *	
 *	offset - address off of flash address 0
 *       
 *	cmd - command to write
 *       
 *
 * GLOBALS:
 *  
 *     none
 *
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *    This routine takes the three parameters and forms the
 *    longword that needs to be written to the flash roms via
 *    1.d000.0200 (see the tutorial in the module header).  It follows
 *    the prescribed algorithm for flipping WrEn up and down and
 *    holding the data on the bus for the necessary length of time.
 *
 * RETURN CODES:
 *
 *	none
 *
 *
 * CALLS:
 *
 *	outportb - console support routine to do the write to ISA space
 *	inb	 - console support routine to do the read from ISA space
 *
 * CONSTANTS:
 *  
 *     FLASHBASE - address 1.2000.0000
 *
 * LOCAL VARIABLES:
 *  
 *     port_command - a longword which is put together to be output
 *
 * ALGORITHM:
 *
 *    To write a command out the following sequence must be followed	    
 *    (using reset to flash 0 as an example):				    
 *									    
 *        write address and command          FF000000			    
 *	  write addr and command w/WrEn      FF200000			    
 *	  wait a short while						    
 *	  write address and command (noWrEn) FF000000			    
 *									     
 * 	  All of the above write were to 1.D000.0200			    
 *
--*/
void write_command(unsigned int rom_port,
		   /*chip select */
		   unsigned int offset,	/*offset from flash base of 0 */
		   unsigned int cmd)
{				/*command byte for flash */

	unsigned long address = (unsigned long) FLASHBASE;

	address =
	    address + (rom_port * (FLASH_SIZE_256K * 8)) + (offset * 8);

#ifdef DEBUG_FLASH
	printk("write_command: writing cmd 0x%x to address 0x%lx\n",
	       cmd, address);
	fl_wait(1000000);
#endif
	*((unsigned long *) address) = (unsigned long) (cmd & 0xff);
	mb();
}


void flash_reset(unsigned int rom_port)
{
#ifdef DEBUG_FLASH
	printk("flash_reset: writing reset cmd to rom_port %d\n",
	       rom_port);
	fl_wait(1000000);
#endif
	write_command(rom_port, 0, RESET_CMD);
	/*start each new operation with reset */
	fl_wait(1000);		/* this waits are specified   */
	/* in the chip spec */
#ifdef DEBUG_FLASH
	printk("flash_reset: writing reset cmd again to rom_port %d\n",
	       rom_port);
	fl_wait(1000000);
#endif
	write_command(rom_port, 0, RESET_CMD);
	fl_wait(1000);
}

void get_flash_id(int rom_port, int *mfg_id, int *dev_id)
{
	*mfg_id = *dev_id = 0xff;

	if (!activate_vpp())
		return;

#ifdef DEBUG_FLASH
	printk("Vpp Activated\n");
	fl_wait(1000000);
#endif

	flash_reset(rom_port);	/*reset the device */

#ifdef DEBUG_FLASH
	printk("Flash reset\n");
	fl_wait(1000000);
#endif

	write_command(rom_port, MFG_ID_OFFSET, ID_ROM);
	/*send the id command */
	fl_wait(10000);
	*mfg_id = flash_read(rom_port, MFG_ID_OFFSET);
	/* now get the mfg id */
	*dev_id = flash_read(rom_port, DEV_ID_OFFSET);
	/*now get the device id */
#ifdef DEBUG_FLASH
	printk("Flash mfr ID = 0x%x, device ID = 0x%x\n", *mfg_id,
	       *dev_id);
	fl_wait(1000000);
#endif
	deactivate_vpp();
#ifdef DEBUG_FLASH
	printk("Vpp Deactivated\n");
	fl_wait(1000000);
#endif
}

#ifdef SERIAL_IO
void flash_dump(unsigned int rom_port)
{
	unsigned int done;
	unsigned int flash_data;
	unsigned int offset;
	unsigned int flash_size;
	unsigned int location;
	unsigned int part;


/*    flash_size = rom_port?FLASH_SIZE_512K:FLASH_SIZE_128K;  */

	flash_size = FLASH_SIZE_512K;

	if (!activate_vpp())
		return;

	flash_reset(rom_port);
	flash_reset(rom_port + 1);
	write_command(rom_port, 0, READ_CMD);
	write_command((rom_port + 1), 0, READ_CMD);

	for (offset = 0; offset < 512; offset++) {
		if (!(offset % 4096)) {
			printk("%05x ", flash_size - offset);
		}
		done = FALSE;

		if (offset >= FLASH_SIZE_256K) {	/* 2 256K parts make 1 rom port */
			part = (rom_port + 1);
			location = offset - FLASH_SIZE_256K;
		} else {
			part = rom_port;
			location = offset;
		}
		flash_data = flash_read(part, location);
		printk("%x ", flash_data & 0xff);
	}
	deactivate_vpp();

	printk("%05x ", flash_size - offset);
}

#endif


/*+
 * ===================================================
 * = flash_write - write a data byte to the flash    =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     This routine allows us to write data bytes
 *     to the flash. This is the lowest level
 *     routine here, save the outport itself, and contains
 *     the AXPPCI33 toggle the chip sels etc algorithm
 *  
 * FORM OF CALL:
 *  
 *     flash_write(rom_port,offset,data)
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *
 *	rom_port - converts into the chip sel
 *	
 *	offset - address off of flash address 0
 *       
 *	data - data byte to write
 *       
 *
 * GLOBALS:
 *  
 *     none
 *
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *    This routine takes the three parameters and forms the
 *    longword that needs to be written to the flash roms via
 *    1.d000.0200 (see the tutorial in the module header).  It follows
 *    the prescribed algorithm for flipping WrEn up and down and
 *    holding the data on the bus for the necessary length of time.
 *
 * RETURN CODES:
 *
 *	none
 *
 *
 * CALLS:
 *
 *	outportb - console support routine to do the write to ISA space
 *	inportb	 - console support routine to do the read from ISA space
 *
 * CONSTANTS:
 *  
 *     WARTBUS_ADDR - address 1.d000.0200
 *
 * LOCAL VARIABLES:
 *  
 *     port_command - a longword which is put together to be output
 *
 * ALGORITHM:
 *
 *    To write a data byte out the following sequence must be followed	    
 *    (using reset to flash 0 as an example):				    
 *									    
 *        write address and data             FF000000			    
 *	  write addr and data w/WrEn	     FF200000			    
 *	  wait a short while						    
 *	  write address and dat (noWrEn)    FF000000			    
 *									     
 * 	  All of the above write were to 1.D000.0200			    
 *
--*/

void flash_write(unsigned int rom_port,
		 /*chip select */
		 unsigned int offset,	/*flash offset from 0 */
		 unsigned int data)
{				/*data byte to write */

	unsigned long address = (unsigned long) FLASHBASE;

	write_command(rom_port, offset, WRITE_CMD);

	address =
	    address + (rom_port * (FLASH_SIZE_256K * 8)) + (offset * 8);

	*((unsigned long *) address) = (unsigned long) (data & 0xff);
	mb();
}



/*+
 * ===================================================
 * = flash_verify - verify roms against new image       =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     compare update the rom with the image in memory.
 *  
 * FORM OF CALL:
 *  
 *     flash_verify(rom_port);
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *	rom_port - flash part to be updated
 *	        
 * FUNCTIONAL DESCRIPTION:
 *
 *      starting from location 0 in feprom, compare to image at
 *	bootadr.  write out an error message if there is a miscompare
 *
 * CALLS:
 *
 *	flash_reset - sends a reset command to the flash part rom_port
 *	
 *	write_command - sends a command byte out to the flash part rom_port
 *	
 *	flash_read - read a byte back from the flash part rom_port
 *	
 *	printf - console supplied output routine
 *	
 * CONSTANTS:
 *  
 *     FLASH_SIZE_512K - # of bytes in a 256K flash
 *     FLASH_SIZE_128K - # of bytes in a 128K flash
 *     READ_CMD - flash read command byte
 *
 * LOCAL VARIABLES:
 *
 *     done - the write took
 *     rom_data - data byte from image that goes to rom  
 *     flash_data - data read back from flash for verify
 *     ffm - local copy of file ptr 
 *     offset - rom location to be blasted 
 *     flash_size - set to either 128k or 256k depending on rom_port
 *
 * ALGORITHM:
 *
 *    -determine the flash part size
 *    -loop from location (0) to location(size-1)
 *	    -read location AND compare to corresponding image location
 *	    -if error, print a message
 *      
 *	  
 *
--*/

void flash_verify(unsigned int rom_port)
{
	unsigned int done;
	unsigned int rom_data;
	unsigned int flash_data;
	register unsigned char *ffm = (unsigned char *) bootadr;
	unsigned int offset;
	unsigned int flash_size;
	unsigned int location;
	unsigned int part;


/*    flash_size = rom_port?FLASH_SIZE_512K:FLASH_SIZE_128K;  */

	flash_size = FLASH_SIZE_512K;

	flash_reset(rom_port);
	flash_reset(rom_port + 1);
	write_command(rom_port, 0, READ_CMD);
	write_command((rom_port + 1), 0, READ_CMD);

	for (offset = 0; offset < flash_size; offset++) {
		rom_data = (unsigned int) ffm[offset];
		/*get next byte from  image */
		if (!(offset % 4096)) {
			printk("%05x ", flash_size - offset);
		}
		done = FALSE;

		if (offset >= FLASH_SIZE_256K) {	/* 2 256K parts make 1 rom port */
			part = rom_port + 1;
			location = offset - FLASH_SIZE_256K;
		} else {
			part = rom_port;
			location = offset;
		}
		flash_data = flash_read(part, location);
		if ((flash_data & 0xFF) != rom_data) {
			printk("blst vrfy err adr: %x exp: %x act: %x \n",
			       offset, rom_data, flash_data);
		}
	}
	printk("%05x ", flash_size - offset);
}

/*+
 * ===================================================
 * = flash_update - flash the roms w/new image       =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     update the rom with the new image.  This routine
 *     assumes that the roms have been precharged and
 *     erased.
 *  
 * FORM OF CALL:
 *  
 *     flash_update(rom_port);
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *	rom_port - flash part to be updated
 *	        
 * FUNCTIONAL DESCRIPTION:
 *
 *      assuming at this point that the roms have been precharged and
 *	erased, this routine will attempt to update the rom. if any location
 *	is stuck, 25 retries is attempted...NOTA BENE, if a whole series
 *	of locations are bad, this will take a VERY VERY LONG TIME
 *
 * CALLS:
 *
 *	flash_reset - sends a reset command to the flash part rom_port
 *	
 *	flash_write - sends a data byte out to the flash part rom_port
 *	
 *	write_command - sends a command byte out to the flash part rom_port
 *	
 *	flash_read - read a byte back from the flash part rom_port
 *	
 *	printf - console supplied output routine
 *	
 * CONSTANTS:
 *  
 *     FLASH_SIZE_512K - # of bytes in a 256K flash
 *     FLASH_SIZE_128K - # of bytes in a 128K flash
 *     WRITE_VERIFY_CMD - flash write verity command byte
 *
 * LOCAL VARIABLES:
 *
 *     plscnt - retry count on failed writes
 *     done - the write took
 *     rom_data - data byte from image that goes to rom  
 *     flash_data - data read back from flash for verify
 *     ffm - local copy of file ptr 
 *     offset - rom location to be blasted 
 *     flash_size - set to either 128k or 256k depending on rom_port
 *
 * ALGORITHM:
 *
 *    -determine the flash part size
 *    -loop from location (0) to location(size-1)
 *      -while (location <> 0) and (retries<25)
 *	 do
 *	    -send WRITE command to rom_port
 *	    -write data byte to present location
 *	    -send WRITE_VERIFY command to end write sequence
 *	    -read location to verify if a zero is there
 *      
 *	  
 *
--*/

void flash_update(unsigned int rom_port)
{
	int plscnt;		/* write retry count */
	int done, error;	/* a flag tells us if the write worked
				 */
	unsigned int rom_data;	/* data byte from image that goes to
				 * rom  */
	unsigned int flash_data;	/* data read back from flash for
					 * verify*/
	register unsigned char *ffm = (unsigned char *) bootadr;
	/*local copy of file ptr */
	unsigned int offset;	/*rom location to be blasted */
	unsigned int flash_size;	/*either 128k or 256k */
	unsigned int part;
	unsigned int location;

/*    flash_size = rom_port?FLASH_SIZE_512K:FLASH_SIZE_128K;  */

	flash_size = FLASH_SIZE_512K;

	flash_reset(2);
	flash_reset(3);
	error = FALSE;

	for (offset = 0; offset < flash_size; offset++) {

		plscnt = 0;	/* Clear retry count */

		rom_data = (unsigned int) ffm[offset];
		/*get next byte from image */
		if (!(offset % 4096)) {
			printk("%05x ", flash_size - offset);
		}
		done = FALSE;
		if (offset >= FLASH_SIZE_256K) {	/* 2 256K parts make 1 rom port */
			part = (3);
			location = offset - FLASH_SIZE_256K;
		} else {
			part = (2);
			location = offset;
		}
		while (!done) {
			flash_write(part, location, rom_data);
			/*write the data */
			fl_wait(10);
			write_command(part, location, WRITE_VERIFY_CMD);
			/*end the write  */
			/*sequence with  */
			/*verify */
			fl_wait(10);
			flash_data = flash_read(part, location);
			/*check the data */
			if ((flash_data & 0xFF) != rom_data) {
				if (plscnt++ <= 25) {	/* try writing and reading 25 times *//* and if it doesn't work quit */
					printk
					    ("Write err - rom: %x offs: %x exp: %x act: %x ret: %x\n",
					     part, offset, rom_data,
					     flash_data, plscnt);
				} else {
					printk
					    ("**************************************************\n");
					printk
					    ("Location shagged, giving up the ghost on this one.\n");
					printk
					    ("**************************************************\n");
					done = TRUE;
					error = TRUE;
				}
			} else
				done = TRUE;	/* we are outta here... */
		}
	}
	printk("%05x ", flash_size - offset);
}


/*+
 * ===================================================
 * = flash_prep - precharge the flash roms =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     Zero-ing out the whole rom, to precharge so that
 *     an erase can follow.
 *  
 * FORM OF CALL:
 *  
 *     flash_prep(rom_port);
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *	rom_port - the flash part to be precharged
 *       
 * FUNCTIONAL DESCRIPTION:
 *
 *	Flash roms need to be precharged (all locations set to zero)
 *	before a rom can be erased (in preparation for blasting). This
 *	routine will attempt to zero all locations in the rom.
 *	NOTA BENE: if for any reason a location is unwritable, this
 *	routine will HANG....why? because what is the sense of going on
 *	if the hardware is busted.
 *
 * CALLS:
 *
 *	flash_reset - sends a reset command to the flash part rom_port
 *	
 *	flash_write - sends a data byte out to the flash part rom_port
 *	
 *	write_command - sends a command byte out to the flash part rom_port
 *	
 *	flash_read - read a byte back from the flash part rom_port
 *	
 *	printf - console supplied output routine
 *	
 * CONSTANTS:
 *  
 *     FLASH_SIZE_512K - # of bytes in a 256K flash
 *     FLASH_SIZE_128K - # of bytes in a 128K flash
 *     WRITE_VERIFY_CMD - flash write verity command byte
 *
 * LOCAL VARIABLES:
 *
 *     plscnt - retry count on failed writes
 *     done   - the write took
 *     i      - counter for locations in the flash
 *     data   - data read back from the flash
 *     flash_size - set to either 128k or 256k depending on rom_port
 *
 * ALGORITHM:
 *
 *    -determine the flash part size
 *    -loop from location (size-1) to location(0)
 *      -while (location <> 0) and (retries<25)
 *	 do
 *	    -send WRITE command to rom_port
 *	    -write 0 to precharge the present location
 *	    -send WRITE_VERIFY command to end write sequence
 *	    -read location to verify if a zero is there
 *      
 *	  
 *
--*/

void flash_prep(unsigned int rom_port)
{
	register int plscnt;	/*retry count on failed writes */
	register int done;	/* the write took */
	register int i;		/* counter for locations in the flash
				 */
	unsigned int data;	/* data read back from the flash */
	unsigned int flash_size;	/*set to either 128k or 256k depending 
					 * on rom_port*/
	unsigned int part;
/*    flash_size = rom_port?FLASH_SIZE_512K:FLASH_SIZE_128K;  */

	flash_size = FLASH_SIZE_256K;
	flash_reset(rom_port);
	flash_reset(rom_port + 1);	/* you have to reset the pair. */


	for (part = rom_port; part < (rom_port + 2); part++) {

		for (i = flash_size - 1; i >= 0; i--) {
			plscnt = 0;
			done = FALSE;
			if (!(i % 4096)) {	/*print a dot every 4k locations */
				printk("%05x ", i);
			}
			while (!done) {
				flash_write(part, i, 0);
				/*write a zero to loc. i */
				fl_wait(10);	/*wait the prescribed time */
				write_command(part, i, WRITE_VERIFY_CMD);
				/*send the verify    */
				/*command to end sequence */
				fl_wait(10);	/*wait again */
				data = flash_read(part, i);
				/*get the data to verify */
				if ((data & 0xFF) != 0) {	/*try the same location up to 25
   * times *//*if it doesn't seem to be working */
					if (plscnt++ == 25) {
						printk
						    ("\nerror cannot write address: %x exp: %x act: %x \n",
						     i, 0, data);
					}
					printk
					    ("Prep err - rom: %x offs: %x exp: %x act: %x ret: %x\n",
					     part, i, 0, (data & 0xFF),
					     plscnt);
				} else {
					done = TRUE;	/* we are outta here... */
				}
			}

		}

	}

}

/*+
 * ===================================================
 * = verify - verify rom has been set to one byte value   =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     compare each location against a single value.
 *  
 * FORM OF CALL:
 *  
 *     verify(rom_port,value);
 *  
 *
 * RETURNS:
 *
 *	1 - all locations match value
 *	0 - a mismatch occured
 *
 * ARGUMENTS:
 *
 *	rom_port - flash part to be updated
 *	value - byte to compare every location against
 *	        
 *
 * CALLS:
 *
 *	flash_read - read a byte back from the flash part rom_port
 *	
 *	printk - console supplied output routine
 *	
 * CONSTANTS:
 *  
 *     FLASH_SIZE_512K - # of bytes in a 256K flash
 *     FLASH_SIZE_128K - # of bytes in a 128K flash
 *
 * LOCAL VARIABLES:
 *
 *     address - rom location to be checked
 *     flash_size - set to either 128k or 256k depending on rom_port
 *
 * ALGORITHM:
 *
 *    -determine the flash part size
 *    -loop from location (0) to location(size-1)
 *	    -read location to verify "value"
 *	    -if value = location continue
 *	    -else return 0
 *    -when done return 1          
 *	  
 *
--*/

int verify(unsigned int rom_port,	/*chip sel for the rom to be checked */
	   char value)
{				/*value to test against */
	register unsigned int address;	/*loop counter */
	unsigned int flash_size;	/*either 128k or 256k */
	unsigned int location;
	unsigned int part;

/*    flash_size = rom_port?FLASH_SIZE_512K:FLASH_SIZE_128K;  */

	flash_size = FLASH_SIZE_512K;

	value &= 0xff;		/*make sure there is not sign extension
				 */

	for (address = 0; address < flash_size; address++) {
		if (address >= FLASH_SIZE_256K) {	/* 2 256K parts make 1 rom port */
			part = (rom_port + 1);
			location = address - FLASH_SIZE_256K;
		} else {
			part = rom_port;
			location = address;
		}

		if ((flash_read(part, location) & 0xFF) !=
		    (unsigned int) value) {
			return 0;	/*if there is a mismatch, return 0 */
		}
	}
	return 1;		/* success, return1 */
}


/*+
 * ===================================================
 * = flash_erase - erase the flash roms =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     putting FF in the whole rom, to prepare for blasting.
 *  
 * FORM OF CALL:
 *  
 *     flash_erase(rom_port);
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *	rom_port - the flash part to be erased
 *       
 * FUNCTIONAL DESCRIPTION:
 *
 *	Flash roms need to be erased (all locations set to FF)
 *	before a rom can be blasted.
 *	NOTA BENE: if for any reason a location is unwritable, this
 *	routine will HANG....why? because what is the sense of going on
 *	if the hardware is busted.
 *
 * CALLS:
 *
 *	flash_reset - sends a reset command to the flash part rom_port
 *	
 *	write_command - sends a command byte out to the flash part rom_port
 *	
 *	flash_read - read a byte back from the flash part rom_port
 *	
 *	printk - console supplied output routine
 *	
 * CONSTANTS:
 *  
 *     FLASH_SIZE_512K - # of bytes in a 256K flash
 *     FLASH_SIZE_128K - # of bytes in a 128K flash
 *     WRITE_VERIFY_CMD - flash write verity command byte
 *
 * LOCAL VARIABLES:
 *
 *     i - a counter for locations in the rom 
 *     retry - a counter for the number of erase retries
 *     done - a flag to let us know when the erase worked
 *     data - a place to keep the data read from location i
 *     flash_size - either 128k or 256k
 *
 * ALGORITHM:
 *    -reset the part
 *    -determine the flash part size
 *    -loop from location (0) to location(size-1)
 *      -while (location <> FF) and (retries<3000)
 *	 do
 *         -send ERASE_VERIFY command
 *	   -read location
 *	   -if location <>FF
 *	      -send ERASE command (to set up) to rom_port
 *	      -send ERASE command again to rom_port
 *
--*/
int flash_erase(unsigned int rom_port)
{
	register int i;		/* a counter for locations in the rom
				 */
	register int retry;	/* a counter for the number of erase
				 * retries*/
	register int done;	/* a flag to let us know when the erase
				 * worked*/
	unsigned int data;	/* a place to keep the data read from
				 * *i */
	unsigned int flash_size;	/* either 128k or 256k                 
					 *       */
	unsigned int part;

	flash_reset(rom_port);
	flash_reset(rom_port + 1);

	flash_size = FLASH_SIZE_256K;
	/*pick the right size for the max
	 * location */

	for (part = rom_port; part < (rom_port + 2); part++) {

		for (i = 0; i < flash_size; i++) {
			retry = 0;
			done = 0;
			if (!(i % 4096)) {	/* every 4k locations, print a dot */
				printk("%05x ", flash_size - i);
			}
			while ((retry < 3000) && (!done)) {
				write_command(part, i, ERASE_VERIFY_CMD);
				/*EVcmd ends the erase sequence */
				fl_wait(10);	/* wait some time here */
				data = flash_read(part, i);
				/*check the location */
				if ((data) != 0xFF) {
					write_command(part, i, ERASE_CMD);
					/* make sure its erased */
					write_command(part, i, ERASE_CMD);
					fl_wait(10000);	/* intel prescribed wait */
					retry++;
				} else {
					done = 1;
				}
			}
			if (retry == 3000) {	/*after 3000 attempts, no erase */
				printk
				    ("cannot erase ROM addr =%x data =%x\n",
				     i, data);
				return 1;
			}
		}

	}
	return 0;
}


/*+
 * ===================================================
 * = flash_program - overall flashing algorithm      =
 * ===================================================
 *
 * OVERVIEW:
 *  
 *     This routine calls all the necessary routines to precharge
 *     erase, blast and verify the flash. 
 *     [@tbs@]...
 *  
 * FORM OF CALL:
 *  
 *     BlastFlash();
 *  
 *
 * RETURNS:
 *
 *	none
 *
 * ARGUMENTS:
 *
 *	rom_port - the flash part to do all the operation to
 *
 *
 *
 * FUNCTIONAL DESCRIPTION:
 *
 *	This routine is the overall algorithm to flash the roms.
 *	it checks to see if the flash is prep'ed and if not does so,
 *	tries to erase and then blast.  A description of what needs
 *	to be done to flash the intel flash parts in in the intel
 *	memory products spec.
 *
 *
 * CALLS:
 *
 *	verify - checks to see if the part is filled with the specified byte
 *	flash_prep - precharges the rom to zeros
 *	flash_erase - erases all rom locations so that FF's are left over
 *	flash_update- places the new code in the flash
 *	flash verify- compares what's in the rom after blasting with memory
 *	flash_reset - resets the flash parts between operations
 *	printk - console supplied output routine
 *
 *
 * CONSTANTS:
 *  
 *     READ_CMD - sets the rom part into read mode
 *
 *
--*/

void flash_program(unsigned int rom_port)
{

	if (!activate_vpp())
		return;

	flash_reset(rom_port);

	write_command(rom_port, 0, READ_CMD);
	/* now setup for reads */
	fl_wait(10000);

	if (verify(rom_port, 0xff)) {	/* if the rom is filled with FF, then
					   * just blast */
	} else {		/* if the rom has something in it... */

		printk("\nzeroing\n");
		if (!verify(rom_port, 0)) {	/*...check to see if it has been
						   * precharged... */
			flash_prep(rom_port);	/*...if not precharged, write 0's to
						 * all locations...*/
		}
		printk("\nerasing\n");	/*...now erase the roms */
		if (flash_erase(rom_port)) {
			printk("unable to erase ROM\n");
			return;
		}
	}
	printk("\nblasting\n");
	flash_update(rom_port);	/* if you get here, you are about to
				 * flash...*/
	printk("\nverifying\n");
	flash_verify(rom_port);
	printk("\n done !\n");

	deactivate_vpp();
}

/* ----------------------------------------------------------------------------------------*/
static void noname_program(void)
{
	int device, vendor;
	int i, found;


	/* give the user a chance to quit.. */
	if (!yes_no("Do you really want to do this (y/N)? ")) {
		return;
	}

	found = FALSE;

	/* get the flash id */
	for (i = 2; i < 4; i++) {
		get_flash_id(i, &vendor, &device);
		if ((vendor == 0x89) && (device == 0xBD)) {
			printk("Intel 27f020 flash device\n");
			found = TRUE;
		} else if ((vendor == 0x01) && (device == 0x2a)) {
			printk("AMD flash device\n");
			found = TRUE;
		} else {
			found = FALSE;
		}
	}

	if (!found) {
		printk("ERROR: this flash is not Intel 27f020\n");
		return;
	}
#ifdef DEBUG_FLASH
	printk("Preparing flash image\n");
	fl_wait(1000000);
#endif
	prepare_flash_image();
#ifdef DEBUG_FLASH
	printk("Flashing program\n");
	fl_wait(1000000);
#endif
	flash_program(2);
	printk("Done. Now boot your system\n");
	do;
	while (1);
}

static int noname_init(void)
{
	return 1;
}

flash_t Flash = {
	"Noname Board",

	noname_init,
	noname_program,

	NULL,
	NULL,
	NULL,
	NULL,
};


#endif
