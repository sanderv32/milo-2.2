# 
# $Id: Makefile,v 1.1.1.1 1995/08/01 17:33:53 paradis Exp $
#
# Revision History:
#
# $Log: Makefile,v $
# Revision 1.1.1.1  1995/08/01  17:33:53  paradis
# Linux 1.3.10 with NoName and miniloader updates
#
# Revision 1.2  1995/07/26  17:24:28  paradis
# Now either builds PAL from source or unpacks a pre-built PAL based on
# configuration option
#
# Revision 1.1  1995/05/22  09:30:47  rusling
# PALcode sources for the EB64+ system (21064 + APECS).
#
# Revision 1.2  1995/03/28  09:23:10  rusling
# minor updates.
#
# Revision 1.1  1995/03/09  10:17:08  rusling
# .
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

ifndef HAVE_NO_SOURCES
include ../../.config
endif

ifdef MINI_BUILD_PALCODE_FROM_SOURCES

AS		= as
LD		= ld
CPP		= /lib/cpp -lang-c++ $(CPPFLAGS)
STRIP		= quickstrip
CSTRIP		= ../../tools/strip/cstrip
OBJSTRIP 	= $(KSRC)/arch/alpha/boot/tools/objstrip
ECHO 		= echo

# Intermediate files:
#
#   This block should not change.
#

IFILES	= $(SFILES:.S=.i)

# Object files:
#
#   This block should not change.
#

.SUFFIXES:
.SUFFIXES: .S .i

.S.i:
	$(CPP) $(DEFINES) $< > $*.i

osfpal.nh: osfpal
	strip -R .mdebug osfpal
	$(OBJSTRIP) -v osfpal $@

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
	rm -f osfpal.i platform.i

else

# For some reason, we can't seem to build this from source.  For now,
# just use a pre-packaged version...

osfpal.nh:	osfpal.nh.uu
	uudecode osfpal.nh.uu

clean:
	rm -f osfpal.nh

endif
