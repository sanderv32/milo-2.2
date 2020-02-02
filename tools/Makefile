#
# build MILO tools
#

TOOLS	= common arc boot list data sysgen  makerom list strip

all:
	set -e; for i in $(TOOLS); do $(MAKE) -C $$i KSRC=$(KSRC); done

install:
	set -e; for i in $(TOOLS); do $(MAKE) -C $$i install; done

clean:
	set -e; for i in $(TOOLS); do $(MAKE) -C $$i clean; done
