struct vga {
    unsigned long video_base;	/* bus address */

    int col;
    int row;
    int nrows;
    int ncols;
};

extern struct vga vga;

void vga_putchar(unsigned char c);
