ifeq (../../.config,$(wildcard ../../.config))
include ../../.config
include $(KSRC)/.config
endif

#DEFINES	= -D__KERNEL__ -D__linux__
#INCLUDES = -I. -I/usr/include
# -I. -I.. -I$(KSRC)/include -I$(KSRC)
#CFLAGS	= -O2 -Wall $(DEFINES) $(INCLUDES)

CFLAGS    =  -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing -mcpu=ev5 \
             -pipe -mno-fp-regs -ffixed-8 -Wa,-m21164a -D__linux__
# -DDEBUG 

LDFLAGS	= 
LOBJS	= 	x86_decode.o x86_ops.o x86_prim_ops.o x86_fpu.o \
		x86_debug.o x86_bios.o

X86LIB	= libx86.a
LOADLIBES = $(X86LIB)

all: $(X86LIB)

$(X86LIB): $(X86LIB)($(LOBJS))
	
clean:
	rm -f *.a *.o *~
