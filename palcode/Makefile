#
# build MILO tools
#

PLATFORMS	= avanti eb164 eb64 eb64p eb64p_p1 eb66 eb66_p1 eb66p lx164 \
		  miata mikasa noname p2k pc164 sx164 takara

all:
	set -e; for i in $(PLATFORMS); do $(MAKE) -C $$i; done

install:
	set -e; for i in $(PLATFORMS); do $(MAKE) -C $$i install; done

clean:
	set -e; for i in $(PLATFORMS); do $(MAKE) -C $$i clean; done
