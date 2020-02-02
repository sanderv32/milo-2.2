# 
# $Id: Makefile,v 1.1.1.1 1995/08/01 17:33:54 paradis Exp $
#
# Revision History:
#
# $Log: Makefile,v $
# Revision 1.1.1.1  1995/08/01  17:33:54  paradis
# Linux 1.3.10 with NoName and miniloader updates
#
# Revision 1.2  1995/07/26  17:24:32  paradis
# Now either builds PAL from source or unpacks a pre-built PAL based on
# configuration option
#
# Revision 1.1  1995/05/22  09:29:00  rusling
# PALcode for the rev 1 EB64+.
#
# Revision 1.2  1995/03/28  09:20:03  rusling
# minor fixups.
#
# Revision 1.1  1995/03/09  10:17:32  rusling
# eb64+ pass 1 makefile.
#
# Revision 2.2  1994/06/16  15:29:48  samberg
# Add cserve.h
#
# Revision 2.1  1994/04/01  21:55:51  ericr
# 1-APR-1994 V2 OSF/1 PALcode
#
# Revision 1.6  1994/03/18  19:11:10  ericr
# Changed TOOLDIR environment variable to EB_TOOLBOX
# Added dependency list
#
# Revision 1.5  1994/03/11  12:46:25  ericr
# Removed hwrpb targets
#
# Revision 1.4  1994/03/09  18:42:11  ericr
# Added KDEBUG to list of default defines
#
# Revision 1.3  1994/03/08  20:27:10  ericr
# Added KDEBUG define.
#
# Revision 1.2  1994/03/08  00:20:08  ericr
# Removed MMGMT define.
#
# Revision 1.1  1994/02/28  18:23:46  ericr
# Initial revision
#
#

ifdef MINI_BUILD_PALCODE_FROM_SOURCES

AS		= alpha-dec-linux-as
LD		= alpha-dec-linux-ld
CPP		= cpp $(CPPFLAGS)
STRIP		= quickstrip
CSTRIP		= ../../tools/cstrip
ECHO 		= echo

#
# Define NPHALT if you want a non-privileged HALT function
# (useful for debugging), otherwise, only the privileged 
# HALT function is available.  NPHALT SHOULD NOT BE INCLUDED
# IN ANY PRODUCTION RELEASE OF PALCODE!
#
# Define KDEBUG if you want a special unprivileged CALL_PAL 
# breakpoint trap handler for remote kernel debugging.
#
# Define CONSOLE_ENTRY to change the sys$enter_console
# transfer address.  Default CONSOLE_ENTRY value is 0x10000.
#

DEFINES 	= -DCONSOLE_ENTRY=0x10000 -DDC21064 -DEB64P -DAPECS_PASS1
LDFLAGS		= -e pal_reset -T ../osfpal.ld -N
ASFLAGS   	= -m21064

# Source files:
#
#   This is the only block in which the list of source files should change.
#
#	SFILES - assembler source files
#	HFILES - header files
#

SFILES	= osfpal.s \
	  platform.s

HFILES	= dc21064.h \
	  osf.h \
	  macros.h \
	  impure.h \
	  platform.h

# Intermediate files:
#
#   This block should not change.
#

IFILES	= $(SFILES:.s=.i)

# Object files:
#
#   This block should not change.
#

.SUFFIXES:
.SUFFIXES: .s .i

.s.i:
	$(CPP) $(DEFINES) $< > $*.i

osfpal.nh: osfpal
	$(CSTRIP) -a osfpal $@

osfpal: osfpal.o 
	$(LD) $(LDFLAGS) -o $@ osfpal.o

osfpal.o: osfpal.i platform.i
	$(AS) $(ASFLAGS) -o $@ osfpal.i platform.i

pvc: osfpal.lis osfpal.nh osfpal.ent osfpal.map
	(export PVC_PAL PVC_ENTRY PVC_MAP;	\
	 PVC_PAL=osfpal.nh;			\
	 PVC_ENTRY=osfpal.ent;			\
	 PVC_MAP=osfpal.map;			\
	 $(PVC);)

osfpal.lis: osfpal
	$(DIS) osfpal > $@

osfpal.map: osfpal
	$(DIS) -m osfpal > $@
	
depend:
	@cat < /dev/null > makedep
	@(for i in $(SFILES); do echo $$i; \
	    $(MAKEDEP) $(MYDEFINES) $$i | 					\
		awk '{ if ($$1 != prev) {if (rec != "") print rec; 		\
		    rec = $$0; prev = $$1; } 					\
		    else { if (length(rec $$2) > 78) { print rec; rec = $$0; } 	\
		    else rec = rec " " $$2 } }		 			\
		    END { print rec }' | sed 's/\.o/\.i/'			\
		    >> makedep; done)
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' > eddep
	@echo '$$r makedep' >> eddep
	@echo 'w' >> eddep
	@cp Makefile Makefile.bak
	@ed - Makefile < eddep
	@rm -f eddep makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

clean:
	rm -f core osfpal.o $(IFILES)
	rm -f osfpal.lis osfpal.nh osfpal.map osfpal

else

# For some reason, we can't seem to build this from source.  For now,
# just use a pre-packaged version...

osfpal.nh:	osfpal.nh.uu
	uudecode osfpal.nh.uu

clean:
	rm -f osfpal.nh


endif

# DO NOT DELETE THIS LINE
osfpal.i:	osfpal.s ./dc21064.h ./osf.h ./macros.h ./impure.h
platform.i:	platform.s ./dc21064.h ./osf.h ./macros.h ./platform.h ./impure.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
