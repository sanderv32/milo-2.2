/*****************************************************************************

       Copyright © 1993, 1994 Digital Equipment Corporation,
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
 * Original author: Ken Curewitz
 * date: 7-aug-95
 */

#include <linux/config.h>
#include <linux/delay.h>

#include "smc.h"

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/malloc.h>
#include <linux/mm.h>

#include <asm/hwrpb.h>
#include <asm/io.h>

unsigned int static SMCUltraBase;

static inline unsigned long SMCConfigState(unsigned long baseAddr);
static inline unsigned long SMCdetectUltraIO(void);
static inline void SMCrunState(unsigned long baseAddr);
static inline void SMCEnableCOM1(unsigned int baseAddr);
static inline void SMCEnableCOM2(unsigned int baseAddr);
static inline void SMCEnableIDE1(unsigned int baseAddr);
static inline void SMCDisableIDE(unsigned int baseAddr, unsigned int ide_id);
static inline void SMCEnableRTC(unsigned int baseAddr);
static inline void SMCEnableKeyboard(unsigned int baseAddr);
static inline void SMCEnableFDC(unsigned int baseAddr);
inline void SMCReportDeviceStatus(unsigned int baseAddr);
inline void disable_onboard(void);
inline void enable_offboard(void);
inline void SMCInit(void);


static inline unsigned long SMCConfigState(unsigned long baseAddr)
{
	int devId;
	int devRev;
	int foundDev;
	unsigned long configPort;
	unsigned long indexPort;
	unsigned long dataPort;

	foundDev = FALSE;

	configPort = indexPort = baseAddr;
	dataPort = (unsigned int) (configPort + 1);

	outb(CONFIG_ON_KEY, configPort);
	outb(CONFIG_ON_KEY, configPort);
	outb(DEVICE_ID, indexPort);
	devId = inb(dataPort);
	if (devId == VALID_DEVICE_ID) {
		outb(DEVICE_REV, indexPort);	/* check for revision */
		devRev = inb(dataPort);
#ifdef DEBUG_SMC
		printk("SMC devRev=%x\n", devRev);
#endif				/* DEBUG_SMC */
	} else {
		udelay(100);
		baseAddr = 0;
	}
#ifdef DEBUG_SMC
	printk("SMC baseAddr=%x, devId=%x, configPort=%x\n",
	       baseAddr, devId, configPort);
#endif				/* DEBUG_SMC */
	return (baseAddr);
}

static inline unsigned long SMCdetectUltraIO()
{
	unsigned long baseAddr;

	baseAddr = 0x3f0;
	if ((baseAddr = SMCConfigState(baseAddr)) == 0x3f0) {
		return baseAddr;
	}
	baseAddr = 0x370;
	if ((baseAddr = SMCConfigState(baseAddr)) == 0x370) {
		return baseAddr;
	}
	return (unsigned long) 0;
}

static inline void SMCrunState(unsigned long baseAddr)
{
	outb(CONFIG_OFF_KEY, baseAddr);
}

/*
 *
 */
static inline void SMCEnableCOM1(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select com1 */
	outb(SER1, dataPort);

	outb(ADDR_LOW, indexPort);
	outb((COM1_BASE & 0xff), dataPort);

	outb(ADDR_HI, indexPort);
	outb(((COM1_BASE >> 8) & 0xff), dataPort);

	outb(INTERRUPT_SEL, indexPort);
	outb((COM1_INTERRUPT), dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

static inline void SMCEnableCOM2(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select com2 */
	outb(SER2, dataPort);

	outb(ADDR_LOW, indexPort);
	outb((COM2_BASE & 0xff), dataPort);

	outb(ADDR_HI, indexPort);
	outb(((COM2_BASE >> 8) & 0xff), dataPort);

	outb(INTERRUPT_SEL, indexPort);
	outb((COM2_INTERRUPT), dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

static inline void SMCEnableIDE1(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select real time clock */
	outb(IDE1, dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

static inline void
SMCDisableIDE(unsigned int baseAddr, unsigned int ide_id)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select real time clock */
	outb((unsigned char) ide_id, dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_OFF, dataPort);
}

static inline void SMCEnableRTC(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select real time clock */
	outb(RTCL, dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

static inline void SMCEnableKeyboard(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select keyboard/mouse */
	outb(KYBD, dataPort);

	outb(0x70, indexPort);	/* primary interrupt */
	outb(0x01, dataPort);

	outb(0x72, indexPort);	/* secondary interrupt */
	outb(0x0c, dataPort);

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

static inline void SMCEnableFDC(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;

	unsigned int oldValue;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(LOGICAL_DEVICE_NUMBER, indexPort);	/* select real time clock */
	outb(FDC, dataPort);

	outb(FDD_MODE_REGISTER, indexPort);
	oldValue = inb(dataPort);

	oldValue |= 0x0E;	/* enable burst mode */
	outb(oldValue, dataPort);

	outb(0x70, indexPort);	/* set up primary interrupt select */
	outb(0x06, dataPort);	/*    to 6 */

	outb(0x72, indexPort);	/* set up dma channel select       */
	outb(0x02, dataPort);	/*    to 2 */

	outb(ACTIVATE, indexPort);
	outb(DEVICE_ON, dataPort);
}

#ifdef DEBUG_SMC
inline void SMCReportDeviceStatus(unsigned int baseAddr)
{
	unsigned int indexPort;
	unsigned int dataPort;
	unsigned int currentControl;
	unsigned int fer;

	indexPort = baseAddr;
	dataPort = (unsigned int) (baseAddr + 1);

	outb(POWER_CONTROL, indexPort);
	currentControl = inb(dataPort);

	if (currentControl & (1 << FDC)) {
		printk("\t+FDC enabled\n");
	} else {
		printk("\t-FDC disabled\n");
	}

	if (currentControl & (1 << IDE1)) {
		printk("\t+IDE1 enabled\n");
	} else {
		printk("\t-IDE1 disabled\n");
	}

	if (currentControl & (1 << IDE2)) {
		printk("\t+IDE2 enabled\n");
	} else {
		printk("\t-IDE2 disabled\n");
	}

	if (currentControl & (1 << PARP)) {
		printk("\t+Parallel port enabled\n");
	} else {
		printk("\t-Parallel port disabled\n");
	}

	if (currentControl & (1 << SER1)) {
		printk("\t+SER1 enabled\n");
	} else {
		printk("\t-SER1 disabled\n");
	}

	if (currentControl & (1 << SER2)) {
		printk("\t+SER2 enabled\n");
	} else {
		printk("\t-SER2 disabled\n");
	}

	outb(0, 0x398);
	fer = inb(0x399) & 0xff;
	printk("onboard super i/o: %x\n", fer);
	printk("\n");
}

/*
 * disable onboard super I/O functions and RTC, KBD, Mouse
 */
inline void disable_onboard()
{
#if 0
	outb(0, 0x3f2);		/* disable old floppy from driving int6,drq  */

	outb(0, 0x398);		/* FER index */
	outb(0x00, 0x399);	/* FER data - disable all functions */
	outb(0x00, 0x399);	/* (need to write twice) */

	outcfgb(0, 19, 0x4d, 0x40);	/* disable keyboard and RTC access thru sio */
#ifdef OFFBOARD_RTC
	outcfgb(0, 19, 0x4e, 0xc0);	/* disable keyboard and RTC access thru sio */
#else
	outcfgb(0, 19, 0x4e, 0xc1);	/* disable keyboard access thru sio */
#endif /* OFFBOARD_RTC */
	outcfgb(0, 19, 0x4f, 0xff);	/* (was 7f) disable all but port 92 */
#endif
}


inline void enable_offboard(void)
{

	outb(0x08, 0x3f2);	/* DMA enable/reset for offboard floppy */
	outb(0x0c, 0x3f2);	/* clear reset                          */

}
inline void SMCInit(void)
{

	SMCUltraBase = SMCdetectUltraIO();
	if (SMCUltraBase != 0) {	/* SMC Super I/O chip was detected. */
		printk("SMC board detected @ base %x\n", SMCUltraBase);
		SMCReportDeviceStatus(SMCUltraBase);
		SMCEnableCOM1(SMCUltraBase);
		SMCEnableCOM2(SMCUltraBase);

		SMCEnableRTC(SMCUltraBase);
		SMCDisableIDE(SMCUltraBase, IDE1);
		SMCDisableIDE(SMCUltraBase, IDE2);
		SMCEnableKeyboard(SMCUltraBase);
		SMCEnableFDC(SMCUltraBase);

		SMCReportDeviceStatus(SMCUltraBase);
#ifdef DISABLE_ONBOARD
		printk("about to disable onboard superIO, etc...\n");
		disable_onboard();
#endif
		printk("SMC enabling...\n");
		SMCrunState(SMCUltraBase);
		printk("SMC enabled...\n");
#ifdef DISABLE_ONBOARD
		enable_offboard();
#endif
	} else {
		printk("No SMC board detected.\n");
	}

}
#else
inline void SMCInit(void)
{

	SMCUltraBase = SMCdetectUltraIO();
	if (SMCUltraBase != 0) {	/* SMC Super I/O chip was detected. */
		SMCEnableCOM1(SMCUltraBase);
		SMCEnableCOM2(SMCUltraBase);
		SMCEnableRTC(SMCUltraBase);
		SMCDisableIDE(SMCUltraBase, IDE1);	/* We'll be using on board IDE */
		SMCDisableIDE(SMCUltraBase, IDE2);
		SMCEnableKeyboard(SMCUltraBase);
		SMCEnableFDC(SMCUltraBase);
		SMCrunState(SMCUltraBase);
	}
}
#endif

void SMCEnableKYBD(unsigned long baseAddr)
{
	SMCEnableKeyboard(baseAddr);
}

int SMC93x_Init(void)
{
	SMCInit();
	return 0;
}

