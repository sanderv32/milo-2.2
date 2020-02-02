#ifndef x86_decode_h
#define x86_decode_h

#include "sysenv.h"
#include "host.h"

#include "x86_regs.h"

/* Instruction Decoding Stuff */

#define FETCH_DECODE_MODRM(m,mod,rh,rl)	fetch_decode_modrm(m,&mod,&rh,&rl)
#define DECODE_RM_BYTE_REGISTER(m,r)	decode_rm_byte_register(m,r)
#define DECODE_RM_WORD_REGISTER(m,r)	decode_rm_word_register(m,r)
#define DECODE_RM_LONG_REGISTER(m,r)	decode_rm_long_register(m,r)
#define DECODE_CLEAR_SEGOVR(m)		m->x86.mode &= ~SYSMODE_CLRMASK

extern void x86_intr_raise(SysEnv * m, u8 type);

extern void x86_exec(SysEnv * m);
extern void halt_sys(SysEnv * m);

extern void fetch_decode_modrm(SysEnv * m, u16 * mod, u16 * regh,
			       u16 * regl);
extern u8 *decode_rm_byte_register(SysEnv * m, int reg);
extern u16 *decode_rm_word_register(SysEnv * m, int reg);
extern u32 *decode_rm_long_register(SysEnv * m, int reg);
extern u16 *decode_rm_seg_register(SysEnv * m, int reg);
extern u8 fetch_byte_imm(SysEnv * m);
extern u16 fetch_word_imm(SysEnv * m);
extern u32 fetch_long_imm(SysEnv * m);
extern u16 decode_rm00_address(SysEnv * m, int rm);
extern u16 decode_rm01_address(SysEnv * m, int rm);
extern u16 decode_rm10_address(SysEnv * m, int rm);
extern u8 fetch_data_byte(SysEnv * m, u16 offset);
extern u8 fetch_data_byte_abs(SysEnv * m, u16 segment, u16 offset);
extern u16 fetch_data_word(SysEnv * m, u16 offset);
extern u16 fetch_data_word_abs(SysEnv * m, u16 segment, u16 offset);
extern u32 fetch_data_long(SysEnv * m, u16 offset);
extern u32 fetch_data_long_abs(SysEnv * m, u16 segment, u16 offset);
extern void store_data_byte(SysEnv * m, u16 offset, u8 val);
extern void store_data_byte_abs(SysEnv * m, u16 segment, u16 offset,
				u8 val);
extern void store_data_word(SysEnv * m, u16 offset, u16 val);
extern void store_data_word_abs(SysEnv * m, u16 segment, u16 offset,
				u16 val);
extern void store_data_long(SysEnv * m, u16 offset, u32 val);
extern void store_data_long_abs(SysEnv * m, u16 segment, u16 offset,
				u32 val);

#endif				/* x86_decode_h */
