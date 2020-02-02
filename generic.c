/*
 *  generic.c
 *  provides machine vector to all stages of milo loading. 
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 *  Stefan Reinauer, <stepan@suse.de>
 *
 */

#include <asm/system.h>
#include "milo.h"

#ifdef MINI_ALPHA_ALCOR
extern
#endif
	struct alpha_machine_vector alcor_mv;

#ifdef MINI_ALPHA_XL
extern
#endif
	struct alpha_machine_vector xl_mv;

#ifdef MINI_ALPHA_BOOK1
extern
#endif
	struct alpha_machine_vector alphabook1_mv;

#ifdef MINI_ALPHA_AVANTI
extern 
#endif
	struct alpha_machine_vector avanti_mv;

#ifdef MINI_ALPHA_CABRIOLET
extern 
#endif
	struct alpha_machine_vector cabriolet_mv;

#ifdef MINI_ALPHA_EB164
extern
#endif
	struct alpha_machine_vector eb164_mv;

#ifdef MINI_ALPHA_EB64P
extern
#endif
	struct alpha_machine_vector eb64p_mv;

#ifdef MINI_ALPHA_EB66
extern
#endif
	struct alpha_machine_vector eb66_mv;

#ifdef MINI_ALPHA_EB66P
extern
#endif
	struct alpha_machine_vector eb66p_mv;

#ifdef MINI_ALPHA_LX164
extern
#endif
	struct alpha_machine_vector lx164_mv;

#ifdef MINI_ALPHA_MIATA
extern
#endif
	struct alpha_machine_vector miata_mv;
#ifdef MINI_ALPHA_MIKASA
extern
#endif
	struct alpha_machine_vector mikasa_mv;
#ifdef MINI_ALPHA_NONAME
extern
#endif
	struct alpha_machine_vector noname_mv;
#ifdef MINI_ALPHA_PC164
extern
#endif
	struct alpha_machine_vector pc164_mv;

#ifdef MINI_ALPHA_P2K
extern
#endif
	struct alpha_machine_vector p2k_mv;

#ifdef MINI_ALPHA_RUFFIAN
extern
#endif
	struct alpha_machine_vector ruffian_mv;

#ifdef MINI_ALPHA_SX164
extern
#endif
	struct alpha_machine_vector sx164_mv;

#ifdef MINI_ALPHA_TAKARA
extern
#endif
	struct alpha_machine_vector takara_mv;

struct alpha_machine_vector alpha_mv;

void init_machvec(void);

void init_machvec(void)
{
#ifdef MINI_ALPHA_ALCOR
	alpha_mv = alcor_mv;
#endif
#ifdef MINI_ALPHA_XL
	alpha_mv = xl_mv;
#endif
#ifdef MINI_ALPHA_BOOK1
	alpha_mv = alphabook1_mv;
#endif
#ifdef MINI_ALPHA_AVANTI
	alpha_mv = avanti_mv;
#endif
#ifdef MINI_ALPHA_CABRIOLET
	alpha_mv = cabriolet_mv;
#endif
#ifdef MINI_ALPHA_EB164
	alpha_mv = eb164_mv;
#endif
#ifdef MINI_ALPHA_EB64P
	alpha_mv = eb64p_mv;
#endif
#ifdef MINI_ALPHA_EB66
	alpha_mv = eb66_mv;
#endif
#ifdef MINI_ALPHA_EB66P
	alpha_mv = eb66p_mv;
#endif
#ifdef MINI_ALPHA_LX164
	alpha_mv = lx164_mv;
#endif
#ifdef MINI_ALPHA_MIATA
	alpha_mv = miata_mv;
#endif
#ifdef MINI_ALPHA_MIKASA
	alpha_mv = mikasa_mv;
#endif
#ifdef MINI_ALPHA_NONAME
	alpha_mv = noname_mv;
#endif
#ifdef MINI_ALPHA_PC164
	alpha_mv = pc164_mv;
#endif
#ifdef MINI_ALPHA_P2K
	alpha_mv = p2k_mv;
#endif
#ifdef MINI_ALPHA_RUFFIAN
	alpha_mv = ruffian_mv;
#endif
#ifdef MINI_ALPHA_SX164
	alpha_mv = sx164_mv;
#endif
#ifdef MINI_ALPHA_TAKARA
	alpha_mv = takara_mv;
#endif

}

