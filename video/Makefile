#
# First, where's the linux kernel that we're building against?
#
include ../.config
include $(KSRC)/.config

DEFINES	=
CC	= gcc
INCLUDES = -I. -I..  -I$(KSRC)/include -Ix86

CFLAGS  =  -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer \
	   -pipe -mno-fp-regs -ffixed-8  -Wa,-mev6 -D__KERNEL__ -D__linux__ \
	   -mcpu=ev5 -fno-strict-aliasing

LDFLAGS	= -O

ifdef MINI_DIGITAL_BIOSEMU
LOBJS         := decbiosvga.o x86-digital/biosem.o $(LOBJS)
endif

#ifdef MINI_FREE_BIOSEMU
#LOBJS         := freebiosvga.o x86/libx86.a $(LOBJS)
#endif

ifdef MINI_FREE_BIOSEMU
LOBJS         := freebiosvga.o $(LOBJS)
endif

ifdef MINI_VGA_RAW
LOBJS := rawvga1.o $(LOBJS)
endif
ifdef MINI_VGA_RAW2
LOBJS := rawvga2.o $(LOBJS)
endif

ifdef LOBJS
LOBJS := vgaprint.o $(LOBJS)
endif

ifdef MINI_TGA_CONSOLE
LOBJS := tga.o $(LOBJS)
endif

ifdef MINI_EXTERNAL_FONT
LOBJS := extfont.o $(LOBJS)
else
LOBJS := font.o $(LOBJS)
endif

LOBJS	:= video.o $(LOBJS)

VIDEOLIB	= video-milo.a

all:	$(VIDEOLIB)

$(VIDEOLIB): $(VIDEOLIB)($(LOBJS))

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<
.s.o:
	$(AS) $(ASFLAGS) -o $*.o $<

font.tmp: $(MINI_FONT_FILE)
	zcat -f $^ > $@

extfont.o:	extfont.s

extfont.s:	../tools/bin/data font.tmp
	../tools/bin/data -vn -l console_font font.tmp extfont.s

x86/libx86.a:
	make -C x86

x86-digital/biosem.o:
	(cd x86-digital && uudecode biosem.o.uu)

../tools/bin/data:	../tools/data/data.c
	(cd ../tools/data ; make install)

clean:
	rm -f $(VIDEOLIB) extfont.s *.o *~ x86-digital/*.o font.tmp
	$(MAKE) -C x86 clean
