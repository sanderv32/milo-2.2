#ifndef sysenv_h
#define sysenv_h

#include "x86_regs.h"

typedef struct {
	unsigned long mem_base;
	unsigned long mem_size;
	unsigned long busmem_base;
	X86Regs x86;
} SysEnv;

extern u8 sys_rdb(SysEnv * m, u32 addr);
extern u16 sys_rdw(SysEnv * m, u32 addr);
extern u32 sys_rdl(SysEnv * m, u32 addr);
extern void sys_wrb(SysEnv * m, u32 addr, u8 val);
extern void sys_wrw(SysEnv * m, u32 addr, u16 val);
extern void sys_wrl(SysEnv * m, u32 addr, u32 val);

extern u8 sys_inb(SysEnv * m, u32 port);
extern u16 sys_inw(SysEnv * m, u32 port);
extern u32 sys_inl(SysEnv * m, u32 port);
extern void sys_outb(SysEnv * m, u32 port, u8 val);
extern void sys_outw(SysEnv * m, u32 port, u16 val);
extern void sys_outl(SysEnv * m, u32 port, u32 val);

extern void sys_init(SysEnv * m);

#endif				/* sysenv_h */
