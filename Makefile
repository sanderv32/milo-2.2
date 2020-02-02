#
# Linux Alpha Mini Loader Makefile.  Build this against the kernel.
#
# 		MILO - the Mini Loader for Linux Kernel 2.2
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# This Makefile is made by david.rusling@reo.mts.dec.com with
# model from Linux original system Makefile.
# Rewritten for Version 2.2 by Stefan Reinauer, <stepan@suse.de>
#
# MILO 2.2 needs a Linux/ELF system to be built.
# Old a.out/ecoff targets have been dropped.
#

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
          else if [ -x /bin/bash ]; then echo /bin/bash; \
          else echo sh; fi ; fi)

#
# Make "config" the default target if there is no configuration file.
#

ifeq (.config,$(wildcard .config))
  include .config
  ifeq ($(KSRC)/.config,$(wildcard $(KSRC)/.config))
    include $(KSRC)/.config
    ifeq (autoconf.h,$(wildcard autoconf.h))
      ALL	= subdirs milo
    else
      ALL	= oldconfig subdirs milo
    endif
  else
      ALL	= nokernel
  endif
else
  ALL	= config
endif

#
# Now, what tools should we be using?
# 

AS     = as
LD     = ld
CC     = gcc
CPP    = $(CC) -E
NM     = nm
STRIP  = strip
OBJSTRIP = $(KSRC)/arch/alpha/boot/tools/objstrip

#
# Set up some of our own linkflags, libraries and so on.
#

LINKFLAGS =  -non_shared -N 
CFLAGS	  =	-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer \
		-fno-strict-aliasing -mcpu=ev5 -pipe -mno-fp-regs \
		-ffixed-8 -Wa,-mev6 -D__KERNEL__ -D__linux__ \
		-I$(KSRC)/include

LIBS	  =   $(KSRC)/lib/lib.a $(KSRC)/arch/alpha/lib/lib.a

#
#  Where do things go in memory?  Be careful with this as you need to have 
#  things like relocate.S agree with these numbers.
#

PALCODE_AT =     0xfffffc0000200000
STUB_AT	=	 0xfffffc0000210000
LOADER_AT =      0xfffffc0000d00000
FMU_AT =	 0xfffffc0000310000

DECOMP_PALBASE = 0x300000
DECOMP_PARAMS =	 0x300B00
DECOMP_BASE =	 0x300F00	# Must always be DECOMP_PARAMS + 1024

#
#  ...and how big are they?
#

PALCODE_SIZE =	0x10000
LOADER_SIZE =	0x20000

FULLLINKFLAGS := $(LINKFLAGS) -Ttext $(LOADER_AT) --defsym milo_global_flags=$(STUB_AT)-8
STUBLINKFLAGS := $(LINKFLAGS) -Ttext $(STUB_AT) --defsym milo_global_flags=$(STUB_AT)-8
FMULINKFLAGS  := $(LINKFLAGS) -Ttext $(FMU_AT) --defsym milo_global_flags=$(STUB_AT)-8
MILOLINKFLAGS := $(LINKFLAGS) -Ttext $(DECOMP_BASE)

#
# Local libraries that Milo needs
#

FSLIB  		= fs-milo.a
VIDEOLIB	= video-milo.a 
ifdef MINI_FREE_BIOSEMU
VIDEOLIB	:= $(VIDEOLIB) video/x86/libx86.a
endif

MINI_LIBS	:= fs/$(FSLIB) video/$(VIDEOLIB)

#
#  The kernel config utility tells us what sort of system it is.  Here we derive
#  some more information from this, like where's the PALcode?  Some of these control 
#  the things that get built later on.
#

KPATH	:= $(KSRC)/arch/alpha/kernel

#
# AlphaBook1 is a NONAME.
#

ifdef MINI_ALPHA_NONAME
MACHINE_TYPE 	=  -DDC21066
PALCODE_DIR  	=  palcode/noname
SYSTEM_SPECIFIC =  -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) mboot milo.dd fmu.gz
ASFLAGS   	=  -m21066
PLATFORM_OBJECTS   := $(KPATH)/sys_sio.o $(KPATH)/core_lca.o
# unwanted:
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_apecs.o
endif

ifdef MINI_ALPHA_P2K
MACHINE_TYPE 	=  -DDC21066
PALCODE_DIR  	=  palcode/p2k
SYSTEM_SPECIFIC = 
ASFLAGS   	=  -m21066
PLATFORM_OBJECTS   := $(KPATH)/sys_sio.o $(KPATH)/core_apecs.o
# unwanted:
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o
endif

ifdef MINI_ALPHA_CABRIOLET
MACHINE_TYPE	=  -DDC21064
PALCODE_DIR	=  palcode/eb64p
SYSTEM_SPECIFIC	=  -DI28F008SA -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) mboot  milo.dd milo.rom fmu.gz
ASFLAGS  	=  -m21064
PLATFORM_OBJECTS   := $(KPATH)/sys_cabriolet.o $(KPATH)/core_apecs.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o kernel/core_pyxis.o kernel/core_cia.o
endif

ifdef MINI_ALPHA_EB164
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR 	=  palcode/eb164
SYSTEM_SPECIFIC =  -DI28F008SA -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) mboot  milo.dd milo.rom fmu.gz
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_cabriolet.o $(KPATH)/core_cia.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o kernel/core_pyxis.o kernel/core_apecs.o
endif

ifdef MINI_ALPHA_PC164
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR 	=  palcode/pc164
SYSTEM_SPECIFIC	=  -DI28F008SA -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) mboot  milo.dd milo.rom fmu.gz
ASFLAGS   	= -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_cabriolet.o $(KPATH)/core_cia.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o kernel/core_pyxis.o kernel/core_apecs.o
endif

ifdef MINI_ALPHA_LX164
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR 	=  palcode/lx164
SYSTEM_SPECIFIC =  -DI28F008SA -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) mboot  milo.dd milo.rom fmu.gz
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_cabriolet.o $(KPATH)/core_pyxis.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o kernel/core_cia.o kernel/core_apecs.o
endif

ifdef MINI_ALPHA_SX164
MACHINE_TYPE	=  -DDC21164 -DDC21164PC
PALCODE_DIR 	=  palcode/sx164
SYSTEM_SPECIFIC =  -DI28F008SA #-DMINI_NVRAM
ALL		:= $(ALL) mboot  milo.dd milo.rom fmu.gz
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_sx164.o $(KPATH)/core_pyxis.o
endif

ifdef MINI_ALPHA_RUFFIAN
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR 	=  palcode/pc164
SYSTEM_SPECIFIC = 
ALL		:= $(ALL) mboot
ASFLAGS		= -m21164a
PLATFORM_OBJECTS   := $(KPATH)/sys_ruffian.o $(KPATH)/core_pyxis.o
endif

ifdef MINI_ALPHA_TAKARA
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR	=  palcode/takara
SYSTEM_SPECIFIC	=  -DI28F008SA #-DMINI_NVRAM
ALL		:= $(ALL) mboot milo.dd milo.rom fmu.gz
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_takara.o $(KPATH)/core_cia.o
endif

ifdef MINI_ALPHA_ALCOR
MACHINE_TYPE 	=  -DDC21164
PALCODE_DIR 	=  palcode/eb164
SYSTEM_SPECIFIC = 
ALL		:= $(ALL) mboot  milo.dd
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_alcor.o $(KPATH)/core_cia.o
endif

ifdef MINI_ALPHA_XLT
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR	=  palcode/eb164
SYSTEM_SPECIFIC	= 
ALL		:= $(ALL) mboot  milo.dd
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_alcor.o $(KPATH)/core_cia.o
endif

ifdef MINI_ALPHA_MIATA
MACHINE_TYPE	=  -DDC21164
PALCODE_DIR 	=  palcode/miata
SYSTEM_SPECIFIC = 
ALL		:= $(ALL) mboot  milo.dd
ASFLAGS   	=  -m21164
PLATFORM_OBJECTS   := $(KPATH)/sys_miata.o $(KPATH)/core_pyxis.o
endif

ifdef MINI_ALPHA_EB66
MACHINE_TYPE	=  -DDC21066 
PALCODE_DIR	=  palcode/eb66
SYSTEM_SPECIFIC = 
ASFLAGS   	=  -m21066
PLATFORM_OBJECTS   := $(KPATH)/sys_eb64p.o $(KPATH)/core_lca.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_apecs.o 
endif

ifdef MINI_ALPHA_EB64P
MACHINE_TYPE	=  -DDC21064
PALCODE_DIR	=  palcode/eb64p
SYSTEM_SPECIFIC	= 
ASFLAGS		=  -m21064
ALL		:= $(ALL)
PLATFORM_OBJECTS   := $(KPATH)/sys_eb64p.o $(KPATH)/core_apecs.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o 
endif

#
# Alpha XL is Avanti, too!
#

ifdef MINI_ALPHA_AVANTI
MACHINE_TYPE	=  -DDC21064
PALCODE_DIR 	=  palcode/avanti
SYSTEM_SPECIFIC =  
ASFLAGS   	=  -m21064
ALL		:= $(ALL) mboot  milo.dd
PLATFORM_OBJECTS   := $(KPATH)/sys_sio.o $(KPATH)/core_apecs.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_lca.o 
endif

ifdef MINI_ALPHA_MIKASA
MACHINE_TYPE 	=  -DDC21064
PALCODE_DIR	=  palcode/mikasa
SYSTEM_SPECIFIC = 
ASFLAGS   	=  -m21064
ALL		:= $(ALL) mboot  milo.dd
PLATFORM_OBJECTS  := $(KPATH)/sys_mikasa.o  $(KPATH)/core_apecs.o
#unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_cia.o 
endif

ifdef MINI_ALPHA_EB66P
MACHINE_TYPE	=  -DDC21066
PALCODE_DIR	=  palcode/eb66p
BUILD_FLASH	=  1
SYSTEM_SPECIFIC =  -DI28F008SA -DMINI_NVRAM
SPEC_OBJECTS	=  nvram.o
ALL		:= $(ALL) fmu.gz
ASFLAGS   	=  -m21066
PLATFORM_OBJECTS  := $(KPATH)/sys_cabriolet.o $(KPATH)/core_lca.o
# unwanted
PLATFORM_OBJECTS   := $(PLATFORM_OBJECTS) kernel/core_cia.o kernel/core_pyxis.o kernel/core_apecs.o
endif

#
#  Set up the compile and build time constants that we need.
#

WHERE	=	-DPALCODE_AT=$(PALCODE_AT) \
		-DLOADER_AT=$(LOADER_AT) \
		-DSTUB_AT=$(STUB_AT) \
		-DDECOMP_PALBASE=$(DECOMP_PALBASE) \
		-DDECOMP_PARAMS=$(DECOMP_PARAMS) \
		-DDECOMP_BASE=$(DECOMP_BASE)

SIZES	=	-DPALCODE_SIZE=$(PALCODE_SIZE) \
		-DLOADER_SIZE=$(LOADER_SIZE) 

DEFINES	= $(WHERE) $(SIZES) \
	  $(MACHINE_TYPE) \
	  $(SYSTEM_SPECIFIC)

INCLUDES = -I. -Ifs -Ivideo  -I$(PALCODE_DIR)

.c.s:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -S -o $*.s $<
.s.o:
	$(AS) $(ASFLAGS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $*.o $<
.S.s:
	$(CC) -D__ASSEMBLY__ -traditional -E -o $*.s $<
.S.o:
	$(CC) -D__ASSEMBLY__ $(DEFINES) $(INCLUDES) -mcpu=ev56 -Wa,-m21164a  \
		-traditional -E -o $*.s_p $<
	$(AS) $(ASFLAGS) -traditional -o $*.o $*.s_p || rm $*.s_p
	rm -f $*.s_p

# head.o MUST be the first entry in OBJECTS !!
OBJECTS	:=	head.o entry.o version.o boot.o hwrpb.o cpu.o milo.o \
		memory.o env.o flash/28f008sa.o timer.o devices.o kernel.o \
		zip/misc.o zip/inflate.o zip/unzip.o generic.o $(SPEC_OBJECTS)

COMMON_OBJECTS	= kbd.o printf.o lib.o support.o
ifdef MINI_SERIAL_ECHO
COMMON_OBJECTS	:= $(COMMON_OBJECTS) uart.o
endif

KERNEL_OBJECTS := $(KSRC)/kernel/softirq.o $(KSRC)/arch/alpha/kernel/irq.o

FLASH_OBJECTS = head.o flash/flash_main.o flash/flash.o \
		flash/noname.o flash/eb.o flash/28f008sa.o generic.o dumbflash.o

STUB_OBJECTS =	head.o stub.o uart.o printf.o support.o \
		dumbirq.o zip/inflate.o zip/unzip.o generic.o

RELOCATE_OBJECTS = head.o relocate.o uart.o printf.o dumbirq.o generic.o

# **********************************************************
#
# Platform dependant local files.
#

ifdef MINI_ALPHA_PC164
OBJECTS		:= $(OBJECTS) smc.o
FLASH_OBJECTS	:= $(FLASH_OBJECTS) smc.o
STUB_OBJECTS	:= $(STUB_OBJECTS) smc.o
RELOCATE_OBJECTS := $(RELOCATE_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_CABRIOLET
OBJECTS		:= $(OBJECTS) smc.o
FLASH_OBJECTS	:= $(FLASH_OBJECTS) smc.o
STUB_OBJECTS	:= $(STUB_OBJECTS) smc.o
RELOCATE_OBJECTS := $(RELOCATE_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_EB66P
OBJECTS		:= $(OBJECTS) smc.o
FLASH_OBJECTS	:= $(FLASH_OBJECTS) smc.o
STUB_OBJECTS	:= $(STUB_OBJECTS) smc.o
RELOCATE_OBJECTS := $(RELOCATE_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_EB164
OBJECTS		:= $(OBJECTS) smc.o
FLASH_OBJECTS	:= $(FLASH_OBJECTS) smc.o
STUB_OBJECTS	:= $(STUB_OBJECTS) smc.o
RELOCATE_OBJECTS := $(RELOCATE_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_LX164
KERNEL_OBJECTS	:= $(KSRC)/arch/alpha/kernel/smc37c93x.o $(KERNEL_OBJECTS)
FLASH_OBJECTS	:= $(FLASH_OBJECTS) smc.o
STUB_OBJECTS	:= $(STUB_OBJECTS) smc.o
RELOCATE_OBJECTS := $(RELOCATE_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_MIATA
PLATFORM_OBJECTS	:= $(KSRC)/arch/alpha/kernel/smc37c669.o $(PLATFORM_OBJECTS)
endif

ifdef MINI_ALPHA_SX164
PLATFORM_OBJECTS	:= $(KSRC)/arch/alpha/kernel/smc37c669.o $(PLATFORM_OBJECTS)
RELOCATE_OBJECTS	:= $(RELOCATE_OBJECTS) smc.o
STUB_OBJECTS		:= $(STUB_OBJECTS) smc.o
#FLASH_OBJECTS		:= $(FLASH_OBJECTS) smc.o
endif

ifdef MINI_ALPHA_MIKASA
OBJECTS		:= $(OBJECTS) pceb.o
endif

ifdef MINI_ALPHA_ALCOR
OBJECTS		:= $(OBJECTS) pceb.o
endif

# ************************************************************************************
#
#  What objects/libraries that we need from the Linux kernel depends on what 
#  we're building.

ifdef CONFIG_PCI

KERNEL_OBJECTS := $(KERNEL_OBJECTS)  \
		  $(KPATH)/bios32.o \
		  $(KSRC)/drivers/pci/pci.o \
		  $(KSRC)/drivers/pci/compat.o

FLASH_OBJECTS := $(FLASH_OBJECTS)  \
		  $(KPATH)/bios32.o \
		  $(KSRC)/drivers/pci/pci.o \
		  $(KSRC)/drivers/pci/compat.o

ifdef CONFIG_PCI_OLD_PROC
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/pci/oldproc.o
endif

ifdef CONFIG_PCI_QUIRKS
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/pci/quirks.o
endif

endif

ifdef CONFIG_SCSI
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/kernel/resource.o
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/block/block.a
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/scsi/scsi.a
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/cdrom/cdrom.a
endif

ifdef CONFIG_BLOCK_DEV_FD
ifndef CONFIG_SCSI
KERNEL_OBJECTS := $(KERNEL_OBJECTS) $(KSRC)/drivers/block/block.a
endif
endif

ifdef CONFIG_PROC_FS
OBJECTS	:=	$(OBJECTS) proc.o
endif

# ***********************************************************************

#
#  The set of targets for this directory depends on what system we're 
#  building the miniloader for.
#
all:	$(ALL)
	@echo
	@echo Now build a rom, burn some flash or load an image!

SUBDIRS	:= $(PALCODE_DIR) fs video

dummy:	
	

subdirs: dummy
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i KSRC=$(KSRC); done

config:	dummy
	$(CONFIG_SHELL) Configure config.in

oldconfig: 
	$(CONFIG_SHELL) Configure -d config.in


video/x86/libx86.a:
	$(MAKE) -C video/x86
	
# ----------------------------------------------------------------------------
#	milo (a relocatable image that can be loaded by _anything_!
# ----------------------------------------------------------------------------

micropal.i:	micropal.S
	$(CPP) $(DEFINES) -D__ASSEMBLY__ $(INCLUDES) $< > micropal.i

micropal.o:	micropal.i
	$(AS) $(ASFLAGS) -o micropal.o micropal.i

micropal:	micropal.o
	$(LD) -e Start $(LINKFLAGS) -o $@ micropal.o

micropal.nh:	micropal $(OBJSTRIP)
	strip micropal
	$(OBJSTRIP) micropal micropal.nh

# ----------------------------------------------------------------------------
#	The miniloader itself (milo.full)
# ----------------------------------------------------------------------------

milo.full.exe: version.h $(OBJECTS) $(COMMON_OBJECTS) $(KERNEL_OBJECTS) $(PLATFORM_OBJETCS) $(MINI_LIBS) $(LIBS)

	$(LD) $(FULLLINKFLAGS) \
		$(OBJECTS) $(COMMON_OBJECTS) \
		$(KERNEL_OBJECTS) $(PLATFORM_OBJECTS) \
		$(MINI_LIBS) $(LIBS)  \
		-o milo.full.exe || \
		(rm -f milo.full.exe && exit 1)
	$(NM) milo.full.exe | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > milo.map

milo.full.stripped: milo.full.exe
	cp -f milo.full.exe milo.full.stripped
	$(STRIP) milo.full.stripped

milo.full:	milo.full.stripped $(OBJSTRIP)
	$(OBJSTRIP) milo.full.stripped milo.full

# ----------------------------------------------------------------------------
#	gunzip stub code (milo.compressed)
# ----------------------------------------------------------------------------

milo.full.s:	tools/bin/data milo.full
	rm -f milo.full.gz
	cp -f milo.full tmp
	gzip -9 tmp
	mv tmp.gz milo.full.gz
	tools/bin/data -v milo.full.gz milo.full.s

milo.full.o:	milo.full.s

milo.compressed.exe: $(STUB_OBJECTS) $(PLATFORM_OBJECTS) $(LIBS) milo.full.o
	$(LD) $(STUBLINKFLAGS) $(STUB_OBJECTS) $(PLATFORM_OBJECTS) $(LIBS) milo.full.o \
		-o milo.compressed.exe || \
		(rm -f milo.compressed.exe && exit 1)
	$(NM) milo.compressed.exe | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > stub.map

milo.compressed.nh:	milo.compressed.exe $(OBJSTRIP)
	cp -f milo.compressed.exe milo.compressed.stripped
	$(STRIP) milo.compressed.stripped
	$(OBJSTRIP) milo.compressed.stripped milo.compressed.nh

milo.compressed: $(PALCODE_DIR)/osfpal.nh milo.compressed.nh tools/bin/sysgen 
	@tools/bin/sysgen \
		-s -e$(PALCODE_AT) $(PALCODE_DIR)/osfpal.nh \
		-s -e$(STUB_AT) milo.compressed.nh \
		-o milo.compressed

# ----------------------------------------------------------------------------
# Create final milo
# ----------------------------------------------------------------------------

milo.compressed.s:	tools/bin/data milo.compressed
	tools/bin/data -v milo.compressed milo.compressed.s

milo.compressed.o:	milo.compressed.s

milo.exe: $(RELOCATE_OBJECTS) $(PLATFORM_OBJECTS) $(LIBS) milo.compressed.o
	$(LD) $(MILOLINKFLAGS) $(RELOCATE_OBJECTS) $(PLATFORM_OBJECTS) $(LIBS) milo.compressed.o \
		-o milo.exe || \
		(rm -f milo.exe && exit 1)
	$(NM) milo.exe | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > relocate.map
	strip $@

milo.nh:	milo.exe $(OBJSTRIP)
	$(OBJSTRIP) milo.exe milo.nh

parblock.nh: 
	echo -n "MILO Init command=" > parblock.nh

milo:	micropal.nh parblock.nh milo.nh tools/bin/sysgen tools/bin/miloctl
	@tools/bin/sysgen \
		-s -e$(DECOMP_PALBASE) micropal.nh \
		-s -e$(DECOMP_PARAMS) parblock.nh \
		-s -e$(DECOMP_BASE) milo.nh \
		-o milo
#	tools/bin/miloctl -v milo -S -V
	tools/bin/miloctl -v milo +S +V

# ----------------------------------------------------------------------------


# ----------------------------------------------------------------------------
#	boot block loadable milo.
# ----------------------------------------------------------------------------

mboot:	milo $(OBJSTRIP)
	$(OBJSTRIP) -p milo mboot >/dev/null

milo.dd: mboot milo
	cat mboot milo > milo.dd

# ----------------------------------------------------------------------------
#	rom images
# ----------------------------------------------------------------------------

milo.rom:	milo tools/bin/makerom
	tools/bin/makerom -v -c -l200000 -i7 -s"MILO" milo -o milo.rom

# ----------------------------------------------------------------------------
#	flash update tool
# ----------------------------------------------------------------------------

ifdef MINI_ALPHA_NONAME
flash/flash_image.s:	tools/bin/data milo 
	tools/bin/data -v milo flash/flash_image.s
else
flash/flash_image.s:	tools/bin/data milo.rom
	tools/bin/data -v milo.rom flash/flash_image.s
endif

flash/flash_image.o:	flash/flash_image.s

fmu.gz:	fmu
	cp -f fmu tmp
	gzip -9 tmp
	mv tmp.gz fmu.gz

fmu: $(FLASH_OBJECTS) $(COMMON_OBJECTS) $(MINI_LIBS) $(LIBS) $(FMU_OBJECTS) flash/flash_image.o
	$(LD) $(FMULINKFLAGS) \
		$(FLASH_OBJECTS) $(COMMON_OBJECTS) $(PLATFORM_OBJECTS) flash/flash_image.o \
		$(MINI_LIBS) $(LIBS) \
		-o fmu || \
(rm -f fmu && exit 1)
	$(NM) fmu | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > fmu.map
	$(STRIP) fmu

# ----------------------------------------------------------------------------
#	The tools that we need to build the miniloader.
# ----------------------------------------------------------------------------

TOOLS=	tools/bin/data  tools/bin/sysgen \
	tools/bin/clist tools/bin/makerom \
	tools/bin/miloctl

tools:	$(TOOLS)
	(cd tools ; make install)

tools/bin/clist:	tools/list/clist.c tools/common/commlib.a
	(cd tools/list ; make install)

tools/bin/sysgen:	tools/sysgen/sysgen.c
	(cd tools/sysgen ; make install)

tools/bin/data:	tools/data/data.c
	(cd tools/data ; make install)

tools/bin/makerom:	tools/makerom/makerom.c
	(cd tools/makerom ; make install)

tools/bin/miloctl:	tools/boot/miloctl.c
	(cd tools/boot; make install)

tools/common/commlib.a:	tools/common/c_32_64.c tools/common/disassm.c tools/common/romhead.c
	(cd tools/common ; make all) 

$(KSRC)/arch/alpha/boot/tools/objstrip:	$(KSRC)/arch/alpha/boot/tools/objstrip.c
	(cd $(KSRC)/arch/alpha/boot/tools ; $(CC) objstrip.c -o objstrip

nokernel: dummy
	@echo "Sorry, you need to configure and compile your kernel first.\n"


# ----------------------------------------------------------------------------
# Clean up ...
# ----------------------------------------------------------------------------

clean:
	rm -f $(ALL) *.nh *.map *.exe *.o flash/*.o flash/flash_image.s *.gz \
		mboot *~ milo.exe milo.full* milo.dd milo.rom milo.compressed* \
		*.stripped *.i fmu micropal autoconf.h version.h .config.old \
		zip/*.o kernel/*.o

	$(MAKE) -C fs clean
	$(MAKE) -C video clean
	$(MAKE) -C tools clean
	$(MAKE) -C palcode clean

# ----------------------------------------------------------------------------
# Version number, build date, ...
# ----------------------------------------------------------------------------

version.h: dummy
	@echo "#define MILO_RELEASE	\"`bash -c 'pwd -P' | sed -e 's/.*milo-//'`\"" > t
	@echo "#define MILO_BUILD_TIME	`date +%Y%m%d%H%M`" >> t
	@echo "#define MILO_BUILD_DATE	\"`date`\"" >> t
	@echo "#define MILO_BUILD_BY	\"`whoami`\"" >> t
	@echo "#define MILO_COMPILER	\"`$(CC) -v 2>&1 | tail -1`\"" >> t
	@mv -f t $@

version.o: version.c version.h
