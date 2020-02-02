#ifndef x86_fpu_regs_h
#define x86_fpu_regs_h

#ifdef X86_FPU_SUPPORT

/* Basic 8087 register can hold any of the following values: */

union x86_fpu_reg_u {
	s8 tenbytes[10];
	double dval;
	float fval;
	s16 sval;
	s32 lval;
};

struct x86_fpu_reg {
	union x86_fpu_reg_u reg;
	char tag;
};


/*
 * Since we are not going to worry about the problems of aliasing
 * registers, every time a register is modified, its result type is
 * set in the tag fields for that register.  If some operation
 * attempts to access the type in a way inconsistent with its current
 * storage format, then we flag the operation.  If common, we'll
 * attempt the conversion.
 */

#define  X86_FPU_VALID		0x80
#define  X86_FPU_REGTYP(r)	((r) & 0x7F)

#define  X86_FPU_WORD		0x0
#define  X86_FPU_SHORT		0x1
#define  X86_FPU_LONG		0x2
#define  X86_FPU_FLOAT		0x3
#define  X86_FPU_DOUBLE		0x4
#define  X86_FPU_LDBL		0x5
#define  X86_FPU_BSD		0x6

#define  X86_FPU_STKTOP  0

struct x86_fpu_registers {
	struct x86_fpu_reg x86_fpu_stack[8];
	int x86_fpu_flags;
	int x86_fpu_config;	/* rounding modes, etc. */
	short x86_fpu_tos, x86_fpu_bos;
};

/* 
 * There are two versions of the following macro.
 *
 * One version is for opcode D9, for which there are more than 32
 * instructions encoded in the second byte of the opcode.
 *
 * The other version, deals with all the other 7 i87 opcodes, for
 * which there are only 32 strings needed to describe the
 * instructions.
 */

#endif				/* X86_FPU_SUPPORT */

#ifdef DEBUG
# define DECODE_PRINTINSTR32(m,t,mod,rh,rl)	\
    DECODE_PRINTF(m,t[(mod<<3)+(rh)]);
# define DECODE_PRINTINSTR256(m,t,mod,rh,rl)	\
    DECODE_PRINTF(m,t[(mod<<6)+(rh<<3)+(rl)]);
#else
# define DECODE_PRINTINSTR32(m,t,mod,rh,rl)
# define DECODE_PRINTINSTR256(m,t,mod,rh,rl)
#endif

#endif				/* x86_fpu_regs_h */
