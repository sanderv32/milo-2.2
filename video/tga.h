/* TGA hardware description (minimal) */
/*
 * Offsets within Memory Space
 */
#define TGA_ROM_OFFSET                  0x0000000
#define TGA_REGS_OFFSET                 0x0100000
#define TGA_8PLANE_FB_OFFSET            0x0200000
#define TGA_24PLANE_FB_OFFSET           0x0804000
#define TGA_24PLUSZ_FB_OFFSET           0x1004000

#define TGA_PLANEMASK_REG               0x0028
#define TGA_MODE_REG                    0x0030
#define TGA_RASTEROP_REG                0x0034
#define TGA_DEEP_REG                    0x0050
#define TGA_PIXELMASK_REG               0x005c
#define TGA_CURSOR_BASE_REG             0x0060
#define TGA_HORIZ_REG                   0x0064
#define TGA_VERT_REG                    0x0068
#define TGA_BASE_ADDR_REG               0x006c
#define TGA_VALID_REG                   0x0070
#define TGA_CURSOR_XY_REG               0x0074
#define TGA_INTR_STAT_REG               0x007c
#define TGA_RAMDAC_SETUP_REG            0x00c0
#define TGA_BLOCK_COLOR0_REG            0x0140
#define TGA_BLOCK_COLOR1_REG            0x0144
#define TGA_CLOCK_REG                   0x01e8
#define TGA_RAMDAC_REG                  0x01f0
#define TGA_CMD_STAT_REG                0x01f8

#define TGA_HORIZ_ODD                   0x80000000
#define TGA_HORIZ_POLARITY              0x40000000
#define TGA_HORIZ_ACT_MSB               0x30000000
#define TGA_HORIZ_BP                    0x0fe00000
#define TGA_HORIZ_SYNC                  0x001fc000
#define TGA_HORIZ_FP                    0x00007c00
#define TGA_HORIZ_ACT_LSB               0x000001ff

#define TGA_VERT_SE                     0x80000000
#define TGA_VERT_POLARITY               0x40000000
#define TGA_VERT_RESERVED               0x30000000
#define TGA_VERT_BP                     0x0fc00000
#define TGA_VERT_SYNC                   0x003f0000
#define TGA_VERT_FP                     0x0000f800
#define TGA_VERT_ACTIVE                 0x000007ff

/*
 * useful defines for managing the BT485 on the 8-plane TGA
 */
#define BT485_READ_BIT                  0x01
#define BT485_WRITE_BIT                 0x00

#define BT485_ADDR_PAL_WRITE            0x00
#define BT485_DATA_PAL                  0x02
#define BT485_PIXEL_MASK                0x04
#define BT485_ADDR_PAL_READ             0x06
#define BT485_ADDR_CUR_WRITE            0x08
#define BT485_DATA_CUR                  0x0a
#define BT485_CMD_0                     0x0c
#define BT485_ADDR_CUR_READ             0x0e
#define BT485_CMD_1                     0x10
#define BT485_CMD_2                     0x12
#define BT485_STATUS                    0x14
#define BT485_CMD_3                     0x14
#define BT485_CUR_RAM                   0x16
#define BT485_CUR_LOW_X                 0x18
#define BT485_CUR_HIGH_X                0x1a
#define BT485_CUR_LOW_Y                 0x1c
#define BT485_CUR_HIGH_Y                0x1e

/*
 * useful defines for managing the BT463 on the 24-plane TGAs
 */
#define BT463_ADDR_LO           0x0
#define BT463_ADDR_HI           0x1
#define BT463_REG_ACC           0x2
#define BT463_PALETTE           0x3

#define BT463_CUR_CLR_0         0x0100
#define BT463_CUR_CLR_1         0x0101

#define BT463_CMD_REG_0         0x0201
#define BT463_CMD_REG_1         0x0202
#define BT463_CMD_REG_2         0x0203

#define BT463_READ_MASK_0       0x0205
#define BT463_READ_MASK_1       0x0206
#define BT463_READ_MASK_2       0x0207
#define BT463_READ_MASK_3       0x0208

#define BT463_BLINK_MASK_0      0x0209
#define BT463_BLINK_MASK_1      0x020a
#define BT463_BLINK_MASK_2      0x020b
#define BT463_BLINK_MASK_3      0x020c

#define BT463_WINDOW_TYPE_BASE  0x0300

/*
 * built-in font management constants
 *
 * NOTE: the built-in font is 8x16, and the video resolution
 * is 640x480 @ 60Hz.
 * This means we could put 30 rows of text on the screen (480/16).
 * However, we wish to make then TGA look just like a VGA, as the
 * default, so, we pad the character to 8x18, and leave some scan
 * lines at the bottom unused.
 */
#define TGA_F_WIDTH 8
#define TGA_F_HEIGHT 16
#define TGA_F_HEIGHT_PADDED 18

static unsigned int fb_offset_presets[4] = {
	TGA_8PLANE_FB_OFFSET,
	TGA_24PLANE_FB_OFFSET,
	0xffffffff,
	TGA_24PLUSZ_FB_OFFSET
};

static unsigned int deep_presets[4] = {
	0x00014000,
	0x0001440d,
	0xffffffff,
	0x0001441d
};

static unsigned int rasterop_presets[4] = {
	0x00000003,
	0x00000303,
	0xffffffff,
	0x00000303
};

static unsigned int mode_presets[4] = {
	0x00002000,
	0x00002300,
	0xffffffff,
	0x00002300
};

static unsigned int base_addr_presets[4] = {
	0x00000000,
	0x00000001,
	0xffffffff,
	0x00000001
};

#define TGA_WRITE_REG(v,r) \
        { writel((v), tga_regs_base+(r)); mb(); }

#define TGA_READ_REG(r) readl(tga_regs_base+(r))

#define BT485_WRITE(v,r) \
          TGA_WRITE_REG((r),TGA_RAMDAC_SETUP_REG);              \
          TGA_WRITE_REG(((v)&0xff)|((r)<<8),TGA_RAMDAC_REG);

#define BT463_LOAD_ADDR(a) \
        TGA_WRITE_REG(BT463_ADDR_LO<<2, TGA_RAMDAC_SETUP_REG); \
        TGA_WRITE_REG((BT463_ADDR_LO<<10)|((a)&0xff), TGA_RAMDAC_REG); \
        TGA_WRITE_REG(BT463_ADDR_HI<<2, TGA_RAMDAC_SETUP_REG); \
        TGA_WRITE_REG((BT463_ADDR_HI<<10)|(((a)>>8)&0xff), TGA_RAMDAC_REG);

#define BT463_WRITE(m,a,v) \
        BT463_LOAD_ADDR((a)); \
        TGA_WRITE_REG(((m)<<2),TGA_RAMDAC_SETUP_REG); \
        TGA_WRITE_REG(((m)<<10)|((v)&0xff),TGA_RAMDAC_REG);


static unsigned int
 fontmask_bits[16] = {
	0x00000000,
	0xff000000,
	0x00ff0000,
	0xffff0000,
	0x0000ff00,
	0xff00ff00,
	0x00ffff00,
	0xffffff00,
	0x000000ff,
	0xff0000ff,
	0x00ff00ff,
	0xffff00ff,
	0x0000ffff,
	0xff00ffff,
	0x00ffffff,
	0xffffffff
};

const unsigned char Palette[16][3] = {

	{0x00, 0x00, 0x00},	/* Black */
	{0xAA, 0x00, 0x00},	/* Red */
	{0x00, 0xAA, 0x00},	/* Green */
	{0xAA, 0xAA, 0x00},	/* Yellow */

	{0x00, 0x00, 0xAA},	/* Blue */
	{0xAA, 0x00, 0xAA},	/* Magenta */
	{0x00, 0xAA, 0xAA},	/* Cyan */
	{0xAA, 0xAA, 0xAA},	/* White */

	{0x00, 0x00, 0x00},	/* Hi Black */
	{0xFF, 0x00, 0x00},	/* Hi Red */
	{0x00, 0xFF, 0x00},	/* Hi Green */
	{0xFF, 0xFF, 0x00},	/* Hi Yellow */

	{0x00, 0x00, 0xFF},	/* Hi Blue */
	{0xFF, 0x00, 0xFF},	/* Hi Magenta */
	{0x00, 0xFF, 0xFF},	/* Hi Cyan */
	{0xFF, 0xFF, 0xFF}	/* Hi White */
};

const unsigned long bt485_cursor_source[64] = {
	0x00000000000000ff, 0x00000000000000ff, 0x00000000000000ff,
	    0x00000000000000ff,
	0x00000000000000ff, 0x00000000000000ff, 0x00000000000000ff,
	    0x00000000000000ff,
	0x00000000000000ff, 0x00000000000000ff, 0x00000000000000ff,
	    0x00000000000000ff,
	0x00000000000000ff, 0x00000000000000ff, 0x00000000000000ff,
	    0x00000000000000ff,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const unsigned int bt463_cursor_source[256] = {
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0xffff0000, 0x00000000, 0x00000000, 0x00000000,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* from kernel drivers/char/console.c */
static unsigned char color_table[] = { 0, 4, 2, 6, 1, 5, 3, 7,
	8, 12, 10, 14, 9, 13, 11, 15
};

/* the default colour table, for VGA+ colour systems */
static int default_red[] =
    { 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa,
	0x55, 0xff, 0x55, 0xff, 0x55, 0xff, 0x55, 0xff
};
static int default_grn[] =
    { 0x00, 0x00, 0xaa, 0x55, 0x00, 0x00, 0xaa, 0xaa,
	0x55, 0x55, 0xff, 0xff, 0x55, 0x55, 0xff, 0xff
};
static int default_blu[] =
    { 0x00, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff
};
