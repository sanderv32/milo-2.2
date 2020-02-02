#ifndef x86_bios_h
#define x86_bios_h

#include "sysenv.h"

#define BIOS_SEG	0xfff0

extern void (*bios_intr_tab[]) (SysEnv * m, int intno);

extern void x86_bios_init(SysEnv * m);

#endif				/* x86_bios_h */
