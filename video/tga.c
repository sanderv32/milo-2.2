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
 *
 */
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kd.h>
#include <linux/pci.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>

#include "milo.h"
#include "video.h"

#include "tga.h"

int tga_console_find(void);
static void tga_putchar(unsigned char c);

struct bootvideo tgavideo = {
	"TGA",
	&tga_console_find,
	&tga_putchar
};

static unsigned int tga_mem_base;
static unsigned long tga_fb_base;
static unsigned long tga_regs_base;
static unsigned int tga_bpp, tga_fb_width, tga_fb_height, tga_fb_stride;

static int tga_type;

static void tga_init_video(void);
static void tga_clear_screen(void);
static int tga_blitc(unsigned int charattr, int row, int col);

static void tga_set_cursor(int row, int col);

static int col;
static int row;
static int nrows;
static int ncols;
static int cursor_xoffset, cursor_yoffset;

static void video_scrollscreen(void)
{
	int j, k;
	unsigned int value, value1, value2, value3, value4;
	unsigned int *dst, *src;
	int scroll_inc = (tga_type == 0) ? 1 : 3;

	for (row = scroll_inc; row < nrows; row++) {
		for (col = 0; col < ncols; col++) {
			src = (unsigned int *) ((unsigned long) tga_fb_base
						+
						(row * tga_fb_width *
						 TGA_F_HEIGHT_PADDED) +
						(col * TGA_F_WIDTH *
						 tga_bpp));
			dst =
			    (unsigned int *) ((unsigned long) tga_fb_base +
					      ((row - scroll_inc) *
					       tga_fb_width *
					       TGA_F_HEIGHT_PADDED) +
					      (col * TGA_F_WIDTH *
					       tga_bpp));

			for (j = 0; j < TGA_F_HEIGHT_PADDED; j++) {
				if (tga_type == 0) {
					value = readl(src);
					writel(value, dst);
					value = readl(src + 1);
					writel(value, (dst + 1));
				} else {
					for (k = 0; k < TGA_F_WIDTH;
					     k += 4) {
						value1 = readl(src + k);
						value2 =
						    readl(src + k + 1);
						value3 =
						    readl(src + k + 2);
						value4 =
						    readl(src + k + 3);
						writel(value1, dst + k);
						writel(value2,
						       dst + k + 1);
						writel(value3,
						       dst + k + 2);
						writel(value4,
						       dst + k + 3);
					}
				}
				dst += tga_fb_stride;
				src += tga_fb_stride;
			}
		}
	}

	/* Clear the last row(s) by writing all spaces to it. */
	value = (unsigned short int) ' ';
	value = value | (0x07 << 8);

	for (k = nrows - scroll_inc; k < nrows; k++) {
		for (j = 0; j < ncols; j++) {
			tga_blitc(value, k, j);
		}
	}

	/* Position cursor at bottom of cleared row */
	row = (nrows - scroll_inc);
	col = 0;
	tga_set_cursor(row, col);
}


/*
 *-----------------------------------------------------------------------
 * tga_putchar --
 *      Put a character out to screen memory.
 *-----------------------------------------------------------------------
 */
static void tga_putchar(unsigned char c)
{
	unsigned short int outword;

	if (c == '\n') {

		/* just go onto the next row */
		row++;
		col = 0;
	} else if (c == '\r') {

		/* Go to beginning of current row */
		col = 0;
	} else if (c == '\b') {

		/* Backspace: go back one col unless we're already at beginning of
		 * row... */
		if (col > 0)
			col--;
	} else {

		/* build the character/attribute word */
		outword = (unsigned short int) c;
		outword = outword | (0x07 << 8);

		/* output it to the correct location */
		tga_blitc(outword, row, col);

		/* move on where to write the next character */
		col++;
	}

	/* check for rows and cols overflowing */
	if (col > ncols - 1) {
		col = 0;
		row++;
	}
	if (row > nrows - 1) {
		video_scrollscreen();
	}

	/* Put the cursor at the right spot... */
	tga_set_cursor(row, col);
}

#define PCI_DEVICE_ID_DEC_TGA 0x0004

int tga_console_find(void)
{
	unsigned char pci_bus, pci_devfn;
	int status;

	/* unsigned int temp; */

	/* first, find the TGA among the PCI devices... */
	status =
	    pcibios_find_device(PCI_VENDOR_ID_DEC, PCI_DEVICE_ID_DEC_TGA,
				0, &pci_bus, &pci_devfn);
	if (status == PCIBIOS_DEVICE_NOT_FOUND) {
		/* PANIC!!! */
#ifdef DEBUG
		printk("tga_console_init: TGA not found!!! :-(\n");
#endif
		return FALSE;
	}

	/* read BASE_REG_0 for memory address */
	pcibios_read_config_dword(pci_bus, pci_devfn, PCI_BASE_ADDRESS_0,
				  &tga_mem_base);

	tga_mem_base &= ~15;

#ifdef DEBUG
	printk("tga_console_init: mem_base 0x%x\n", tga_mem_base);
#endif				/* DEBUG */

	tga_type = (readl((unsigned long) tga_mem_base) >> 12) & 0x0f;
	if (tga_type != 0 && tga_type != 1 && tga_type != 3) {
		printk("TGA type (0x%x) unrecognized!\n", tga_type);
		return FALSE;
	}

	tga_init_video();
	tga_clear_screen();
	return TRUE;
}

void tga_init_video(void)
{
	int i, j, temp;
	unsigned char *cbp;
	unsigned int vhcr, vvcr, xres, yres, right_margin, lower_margin;
	unsigned int hsync_len, vsync_len, left_margin, upper_margin;

	tga_regs_base = ((unsigned long) tga_mem_base + TGA_REGS_OFFSET);
	tga_fb_base =
	    ((unsigned long) tga_mem_base + fb_offset_presets[tga_type]);

	/* decode board configuration */
	vhcr = TGA_READ_REG(TGA_HORIZ_REG);
	vvcr = TGA_READ_REG(TGA_VERT_REG);
	xres =
	    ((vhcr & TGA_HORIZ_ACT_LSB) |
	     ((vhcr & TGA_HORIZ_ACT_MSB) >> 19)) * 4;
	yres = (vvcr & TGA_VERT_ACTIVE);
	right_margin = ((vhcr & TGA_HORIZ_FP) >> 9) * 4;
	lower_margin = ((vvcr & TGA_VERT_FP) >> 11);
	hsync_len = ((vhcr & TGA_HORIZ_SYNC) >> 14) * 4;
	vsync_len = ((vvcr & TGA_VERT_SYNC) >> 16);
	left_margin = ((vhcr & TGA_HORIZ_BP) >> 21) * 4;
	upper_margin = ((vvcr & TGA_VERT_BP) >> 22);

	/* first, disable video timing */
	TGA_WRITE_REG(0x03, TGA_VALID_REG);	/* SCANNING and BLANK */

	/* write the DEEP register */
	while (TGA_READ_REG(TGA_CMD_STAT_REG) & 1)	/* wait for not busy */
		continue;

	mb();
	TGA_WRITE_REG(deep_presets[tga_type], TGA_DEEP_REG);
	while (TGA_READ_REG(TGA_CMD_STAT_REG) & 1)	/* wait for not busy */
		continue;
	mb();

	/* write some more registers */
	TGA_WRITE_REG(rasterop_presets[tga_type], TGA_RASTEROP_REG);
	TGA_WRITE_REG(mode_presets[tga_type], TGA_MODE_REG);
	TGA_WRITE_REG(base_addr_presets[tga_type], TGA_BASE_ADDR_REG);

	TGA_WRITE_REG(0xffffffff, TGA_PLANEMASK_REG);
	TGA_WRITE_REG(0xffffffff, TGA_PIXELMASK_REG);
	TGA_WRITE_REG(0x12345678, TGA_BLOCK_COLOR0_REG);
	TGA_WRITE_REG(0x12345678, TGA_BLOCK_COLOR1_REG);

	if (tga_type == 0) {	/* 8-plane */

		tga_bpp = 1;

		/* init BT485 RAMDAC registers */
		BT485_WRITE(0xa2, BT485_CMD_0);
		BT485_WRITE(0x01, BT485_ADDR_PAL_WRITE);
		BT485_WRITE(0x14, BT485_CMD_3);	/* cursor 64x64 */
		BT485_WRITE(0x40, BT485_CMD_1);
		BT485_WRITE(0x22, BT485_CMD_2);	/* WIN cursor type */
		BT485_WRITE(0xff, BT485_PIXEL_MASK);

		/* fill palette registers */
		BT485_WRITE(0x00, BT485_ADDR_PAL_WRITE);
		TGA_WRITE_REG(BT485_DATA_PAL, TGA_RAMDAC_SETUP_REG);

		for (i = 0; i < 16; i++) {
			j = color_table[i];
			TGA_WRITE_REG(default_red[j] |
				      (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(default_grn[j] |
				      (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(default_blu[j] |
				      (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
		}
		for (i = 0; i < 240 * 3; i += 4) {
			TGA_WRITE_REG(0x55 | (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT485_DATA_PAL << 8),
				      TGA_RAMDAC_REG);
		}

		/* initialize RAMDAC cursor colors */
		BT485_WRITE(0, BT485_ADDR_CUR_WRITE);

		BT485_WRITE(0xaa, BT485_DATA_CUR);	/* overscan WHITE */
		BT485_WRITE(0xaa, BT485_DATA_CUR);	/* overscan WHITE */
		BT485_WRITE(0xaa, BT485_DATA_CUR);	/* overscan WHITE */

		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 1 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 1 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 1 BLACK */

		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 2 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 2 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 2 BLACK */

		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 3 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 3 BLACK */
		BT485_WRITE(0x00, BT485_DATA_CUR);	/* color 3 BLACK */

		/* initialize RAMDAC cursor RAM */
		BT485_WRITE(0x00, BT485_ADDR_PAL_WRITE);
		cbp = (unsigned char *) bt485_cursor_source;
		for (i = 0; i < 512; i++) {
			BT485_WRITE(*cbp++, BT485_CUR_RAM);
		}
		for (i = 0; i < 512; i++) {
			BT485_WRITE(0xff, BT485_CUR_RAM);
		}

	} else {		/* 24-plane or 24plusZ */

		tga_bpp = 4;

		TGA_WRITE_REG(0x01, TGA_VALID_REG);	/* SCANNING */

		/*
		 * init some registers
		 */
		BT463_WRITE(BT463_REG_ACC, BT463_CMD_REG_0, 0x40);
		BT463_WRITE(BT463_REG_ACC, BT463_CMD_REG_1, 0x08);
		BT463_WRITE(BT463_REG_ACC, BT463_CMD_REG_2, 0x40);

		BT463_WRITE(BT463_REG_ACC, BT463_READ_MASK_0, 0xff);
		BT463_WRITE(BT463_REG_ACC, BT463_READ_MASK_1, 0xff);
		BT463_WRITE(BT463_REG_ACC, BT463_READ_MASK_2, 0xff);
		BT463_WRITE(BT463_REG_ACC, BT463_READ_MASK_3, 0x0f);

		BT463_WRITE(BT463_REG_ACC, BT463_BLINK_MASK_0, 0x00);
		BT463_WRITE(BT463_REG_ACC, BT463_BLINK_MASK_1, 0x00);
		BT463_WRITE(BT463_REG_ACC, BT463_BLINK_MASK_2, 0x00);
		BT463_WRITE(BT463_REG_ACC, BT463_BLINK_MASK_3, 0x00);

		/*
		 * fill the palette
		 */
		BT463_LOAD_ADDR(0x0000);
		TGA_WRITE_REG((BT463_PALETTE << 2), TGA_RAMDAC_SETUP_REG);

		for (i = 0; i < 16; i++) {
			j = color_table[i];
			TGA_WRITE_REG(default_red[j] |
				      (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(default_grn[j] |
				      (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(default_blu[j] |
				      (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
		}
		for (i = 0; i < 512 * 3; i += 4) {
			TGA_WRITE_REG(0x55 | (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x00 | (BT463_PALETTE << 10),
				      TGA_RAMDAC_REG);
		}

		/*
		 * fill window type table after start of vertical retrace
		 */
		while (!(TGA_READ_REG(TGA_INTR_STAT_REG) & 0x01))
			continue;
		TGA_WRITE_REG(0x01, TGA_INTR_STAT_REG);
		mb();
		while (!(TGA_READ_REG(TGA_INTR_STAT_REG) & 0x01))
			continue;
		TGA_WRITE_REG(0x01, TGA_INTR_STAT_REG);

		BT463_LOAD_ADDR(BT463_WINDOW_TYPE_BASE);
		TGA_WRITE_REG((BT463_REG_ACC << 2), TGA_RAMDAC_SETUP_REG);

		for (i = 0; i < 16; i++) {
			TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x01 | (BT463_REG_ACC << 10),
				      TGA_RAMDAC_REG);
			TGA_WRITE_REG(0x80 | (BT463_REG_ACC << 10),
				      TGA_RAMDAC_REG);
		}

		/*
		 * init cursor colors
		 */
		BT463_LOAD_ADDR(BT463_CUR_CLR_0);

		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* bg */
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* bg */
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* bg */

		TGA_WRITE_REG(0xff | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* fg */
		TGA_WRITE_REG(0xff | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* fg */
		TGA_WRITE_REG(0xff | (BT463_REG_ACC << 10), TGA_RAMDAC_REG);	/* fg */

		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);

		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);
		TGA_WRITE_REG(0x00 | (BT463_REG_ACC << 10),
			      TGA_RAMDAC_REG);

		/*
		 * finally, init the cursor shape.
		 * this assumes video starts at base,
		 * and base is beyond memory start,
		 * which is on a megabyte boundary.
		 */
		temp = tga_fb_base & ~0x000fffffU;

		for (i = 0; i < 256; i++) {
			writel(bt463_cursor_source[i], temp + i * 4);
		}
		TGA_WRITE_REG(0x3c00, TGA_CURSOR_BASE_REG);
	}

	/* finally, enable video scan & cursor */
	TGA_WRITE_REG(0x05, TGA_VALID_REG);	/* SCANNING and CURSOR */

	/* describe the board */
	tga_fb_width = xres * tga_bpp;
	tga_fb_height = yres;
	tga_fb_stride = tga_fb_width / sizeof(int);
	nrows = yres / TGA_F_HEIGHT_PADDED;
	ncols = xres / TGA_F_WIDTH;
	cursor_xoffset = left_margin + hsync_len;
	cursor_yoffset = upper_margin + vsync_len;
}

static void tga_clear_screen()
{
	register int i, j;
	register unsigned int *dst;

	dst = (unsigned int *) ((unsigned long) tga_fb_base);
	for (i = 0; i < tga_fb_height; i++, dst += tga_fb_stride) {
		for (j = 0; j < tga_fb_stride; j++) {
			writel(0, (dst + j));
		}
	}

	row = col = 0;
}

static void tga_set_cursor(int row, int col)
{
	unsigned long flags;
	unsigned int xt, yt;

	save_flags(flags);
	cli();

	if (tga_type == 0) {	/* 8-plane */

		xt = col * TGA_F_WIDTH + 64;
		yt = row * TGA_F_HEIGHT_PADDED + 64;

		/* make sure it's enabled */
		BT485_WRITE(0x22, BT485_CMD_2);	/* WIN cursor type */

		BT485_WRITE(xt, BT485_CUR_LOW_X);
		BT485_WRITE((xt >> 8), BT485_CUR_HIGH_X);
		BT485_WRITE(yt, BT485_CUR_LOW_Y);
		BT485_WRITE((yt >> 8), BT485_CUR_HIGH_Y);

	} else {

		xt = (col - 1) * TGA_F_WIDTH + cursor_xoffset;
		yt = row * TGA_F_HEIGHT_PADDED + cursor_yoffset;

		TGA_WRITE_REG(0x05, TGA_VALID_REG);	/* SCANNING and CURSOR */
		TGA_WRITE_REG(xt | (yt << 12), TGA_CURSOR_XY_REG);
	}

	restore_flags(flags);
}

/*
 * tga_blitc
 *
 * Renders an ASCII/DEC MTS character at a specified character cell position.
 */

static int tga_blitc(unsigned int charattr, int row, int col)
{
	int c, attrib;
	register unsigned int fgmask, bgmask, data, rowbits;
	register unsigned int *dst;
	register unsigned char *font_row;
	register int i, j, stride;

	c = charattr & 0x00ff;
	attrib = (charattr >> 8) & 0x00ff;

	/*
	 * extract foreground and background indices
	 * NOTE: we always treat blink/underline bits as color for now...
	 */
	fgmask = attrib & 0x0f;
	bgmask = (attrib >> 4) & 0x0f;

	i = (c & 0xff) << 4;	/* NOTE: assumption of 16 bytes per character bitmap */

	/*
	 * calculate destination address
	 */
	dst = (unsigned int *) ((unsigned long) tga_fb_base
				+
				(row * tga_fb_width *
				 TGA_F_HEIGHT_PADDED) +
				(col * TGA_F_WIDTH * tga_bpp));

	font_row = (unsigned char *) &console_font[i];
	stride = tga_fb_stride;

	if (tga_type == 0) {	/* 8-plane */

		fgmask = fgmask << 8 | fgmask;
		fgmask |= fgmask << 16;
		bgmask = bgmask << 8 | bgmask;
		bgmask |= bgmask << 16;

		for (j = 0; j < TGA_F_HEIGHT_PADDED; j++) {
			if (j < TGA_F_HEIGHT) {
				rowbits = font_row[j];
			} else {
				/* dup the last n rows only if char > 0x7f */
				if (c & 0x80)
					rowbits =
					    font_row[j -
						     (TGA_F_HEIGHT_PADDED -
						      TGA_F_HEIGHT)];
				else
					rowbits = 0;
			}
			data = fontmask_bits[(rowbits >> 4) & 0xf];
			data = (data & fgmask) | (~data & bgmask);
			writel(data, dst);
			data = fontmask_bits[rowbits & 0xf];
			data = (data & fgmask) | (~data & bgmask);
			writel(data, (dst + 1));
			dst += stride;
		}
	} else {		/* 24-plane */

		fgmask = (default_red[fgmask] << 16) |
		    (default_grn[fgmask] << 8) |
		    (default_blu[fgmask] << 0);
		bgmask = (default_red[bgmask] << 16) |
		    (default_grn[bgmask] << 8) |
		    (default_blu[bgmask] << 0);

		for (i = 0; i < TGA_F_HEIGHT_PADDED; i++) {
			if (i < TGA_F_HEIGHT) {
				rowbits = font_row[i];
			} else {
				/* dup the last n rows only if char > 0x7f */
				if (c & 0x80)
					rowbits =
					    font_row[i -
						     (TGA_F_HEIGHT_PADDED -
						      TGA_F_HEIGHT)];
				else
					rowbits = 0;
			}
			data = 1 << (TGA_F_WIDTH - 1);
			for (j = 0; j < TGA_F_WIDTH; j++, data >>= 1) {
				if (rowbits & data)
					writel(fgmask, (dst + j));
				else
					writel(bgmask, (dst + j));
			}
			dst += stride;
		}
	}
	return (0);
}

void tga_console_init(void)
{
}
