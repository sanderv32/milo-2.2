# 
# $Id: Makefile,v 1.1.1.1 1995/08/01 17:33:53 paradis Exp $
#
# Revision History:
#
# $Log: Makefile,v $
# Revision 1.1.1.1  1995/08/01  17:33:53  paradis
# Linux 1.3.10 with NoName and miniloader updates
#
# Revision 1.2  1995/07/26  17:24:07  paradis
# Now either builds PAL from source or unpacks a pre-built PAL based on
# configuration option
#
# Revision 1.1  1995/05/22  09:28:11  rusling
# Makefile for the EB64 PALcode.
#
# Revision 1.2  1995/03/28  09:19:44  rusling
# Minor fixups.
#
# Revision 1.1  1995/03/09  10:16:42  rusling
# eb64
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

HAVE_NO_SOURCES = 1

include ../Rules.make

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

DEFINES 	= -DCONSOLE_ENTRY=0x210000 -DDC21064 -DEB64
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
	  cserve.h \
	  platform.h

# DO NOT DELETE THIS LINE
