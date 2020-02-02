#ifndef x86_fpu_h
#define x86_fpu_h

/* these have to be defined, whether 8087 support compiled in or not. */

extern void x86op_esc_coprocess_d8(SysEnv * m);
extern void x86op_esc_coprocess_d9(SysEnv * m);
extern void x86op_esc_coprocess_da(SysEnv * m);
extern void x86op_esc_coprocess_db(SysEnv * m);
extern void x86op_esc_coprocess_dc(SysEnv * m);
extern void x86op_esc_coprocess_dd(SysEnv * m);
extern void x86op_esc_coprocess_de(SysEnv * m);
extern void x86op_esc_coprocess_df(SysEnv * m);

#endif				/* x86_fpu_h */
