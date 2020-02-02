#
# First, where's the linux kernel that we're building against?
#
TOPDIR=..

include $(TOPDIR)/.config
include $(KSRC)/.config

CC = gcc
DEFINES	=
INCLUDES = -I. -I.. 
CFLAGS  =  -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer \
	   -pipe -mno-fp-regs -ffixed-8  -Wa,-mev6 -D__KERNEL__ -D__linux__ \
	   -mcpu=ev5 -fno-strict-aliasing -I$(KSRC)/include

LDFLAGS	= -O

LOBJS	= ext2.o isofs.o fs.o dos.o

FSLIB	= fs-milo.a

all:	$(FSLIB) 

$(FSLIB): $(FSLIB)($(LOBJS))

fs.o: 	fs.c 
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<

ext2.o: ext2.c 
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<

dos.o: dos.c 
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<

isofs.o: isofs.c 
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<

clean:
	rm -f $(FSLIB) ginit dump-bios *.o *~
