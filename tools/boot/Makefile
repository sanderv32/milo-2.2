CC	= gcc
CFLG	= -O2 -pipe -Wall $(CFLAGS)
LDFLAGS	= -s
SBINDIR	= ../bin

all:	miloctl srmbootfat srmbootraw

miloctl: miloctl.c ../../flags.h
	$(CC) $(CFLG) $(LDFLAGS) -o miloctl miloctl.c

srmbootfat: srmbootfat.c msdos.h
	$(CC) $(CFLG) $(LDFLAGS) -o srmbootfat srmbootfat.c

srmbootraw: srmbootraw.c
	$(CC) $(CFLG) $(LDFLAGS) -o srmbootraw srmbootraw.c

install: all
	cp -fp miloctl srmbootfat srmbootraw $(SBINDIR)

clean:
	rm -f srmbootfat miloctl srmbootraw
	rm -f ../bin/srmbootfat ../bin/miloctl ../bin/srmbootraw

