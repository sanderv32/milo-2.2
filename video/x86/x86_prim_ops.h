#ifndef x86_prim_ops_h
#define x86_prim_ops_h

#include "host.h"
#include "sysenv.h"

extern u32 x86_parity_tab[8];

#define PARITY(x)	((x86_parity_tab[(x) / 32] >> ((x) % 32)) & 1)

extern void push_word(SysEnv * m, u16 w);
extern void push_long(SysEnv * m, u32 w);
extern u16 pop_word(SysEnv * m);
extern u32 pop_long(SysEnv * m);
extern u16 mem_access_word(SysEnv * m, int addr);
extern u16 aad_word(SysEnv * m, u16 d);
extern u16 aam_word(SysEnv * m, u8 d);
extern u8 adc_byte(SysEnv * m, u8 d, u8 s);
extern u16 adc_word(SysEnv * m, u16 d, u16 s);
extern u8 add_byte(SysEnv * m, u8 d, u8 s);
extern u16 add_word(SysEnv * m, u16 d, u16 s);
extern u8 and_byte(SysEnv * m, u8 d, u8 s);
extern u16 and_word(SysEnv * m, u16 d, u16 s);
extern u8 cmp_byte(SysEnv * m, u8 d, u8 s);
extern u16 cmp_word(SysEnv * m, u16 d, u16 s);
extern u32 cmp_long(SysEnv * m, u32 d, u32 s);
extern u8 dec_byte(SysEnv * m, u8 d);
extern u16 dec_word(SysEnv * m, u16 d);
extern u8 inc_byte(SysEnv * m, u8 d);
extern u16 inc_word(SysEnv * m, u16 d);
extern u8 or_byte(SysEnv * m, u8 d, u8 s);
extern u16 or_word(SysEnv * m, u16 d, u16 s);
extern u8 neg_byte(SysEnv * m, u8 s);
extern u16 neg_word(SysEnv * m, u16 s);
extern u8 not_byte(SysEnv * m, u8 s);
extern u16 not_word(SysEnv * m, u16 s);
extern u8 rcl_byte(SysEnv * m, u8 d, u8 s);
extern u16 rcl_word(SysEnv * m, u16 d, u16 s);
extern u32 rcl_long(SysEnv * m, u32 d, u32 s);
extern u8 rcr_byte(SysEnv * m, u8 d, u8 s);
extern u16 rcr_word(SysEnv * m, u16 d, u16 s);
extern u32 rcr_long(SysEnv * m, u32 d, u32 s);
extern u8 rol_byte(SysEnv * m, u8 d, u8 s);
extern u16 rol_word(SysEnv * m, u16 d, u16 s);
extern u32 rol_long(SysEnv * m, u32 d, u32 s);
extern u8 ror_byte(SysEnv * m, u8 d, u8 s);
extern u16 ror_word(SysEnv * m, u16 d, u16 s);
extern u32 ror_long(SysEnv * m, u32 d, u32 s);
extern u8 shl_byte(SysEnv * m, u8 d, u8 s);
extern u16 shl_word(SysEnv * m, u16 d, u16 s);
extern u32 shl_long(SysEnv * m, u32 d, u32 s);
extern u8 shr_byte(SysEnv * m, u8 d, u8 s);
extern u16 shr_word(SysEnv * m, u16 d, u16 s);
extern u32 shr_long(SysEnv * m, u32 d, u32 s);
extern u8 sar_byte(SysEnv * m, u8 d, u8 s);
extern u16 sar_word(SysEnv * m, u16 d, u16 s);
extern u32 sar_long(SysEnv * m, u32 d, u32 s);
extern u8 sbb_byte(SysEnv * m, u8 d, u8 s);
extern u16 sbb_word(SysEnv * m, u16 d, u16 s);
extern u8 sub_byte(SysEnv * m, u8 d, u8 s);
extern u16 sub_word(SysEnv * m, u16 d, u16 s);
extern void test_byte(SysEnv * m, u8 d, u8 s);
extern void test_word(SysEnv * m, u16 d, u16 s);
extern u8 xor_byte(SysEnv * m, u8 d, u8 s);
extern u16 xor_word(SysEnv * m, u16 d, u16 s);
extern void imul_byte(SysEnv * m, u8 s);
extern void imul_word(SysEnv * m, u16 s);
extern void mul_byte(SysEnv * m, u8 s);
extern void mul_word(SysEnv * m, u16 s);
extern void idiv_byte(SysEnv * m, u8 s);
extern void idiv_word(SysEnv * m, u16 s);
extern void div_byte(SysEnv * m, u8 s);
extern void div_word(SysEnv * m, u16 s);
extern void ins(SysEnv * m, int size);
extern void outs(SysEnv * m, int size);

#endif				/* x86_prim_ops_h */
