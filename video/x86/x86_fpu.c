/* 
 * Copyright (c) 1995 Free Software Foundation.
 *
 * The file is part of x86emu.
 * 
 * x86emu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * x86emu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with x86emu; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  
 */
#include "sysenv.h"

#include "x86_regs.h"
#include "x86_fpu_regs.h"
#include "x86_decode.h"
#include "x86_debug.h"
#include "x86_fpu.h"


/* opcode=0xd8 */
void x86op_esc_coprocess_d8(SysEnv * m)
{
	START_OF_INSTR(m);
	DECODE_PRINTF(m, "ESC D8\n");
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG

static char *x86_fpu_op_d9_tab[] = {
	"FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ",
	    "FSTP\tDWORD PTR ",
	"FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",

	"FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ",
	    "FSTP\tDWORD PTR ",
	"FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",

	"FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ",
	    "FSTP\tDWORD PTR ",
	"FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",
};

static char *x86_fpu_op_d9_tab1[] = {
	"FLD\t", "FLD\t", "FLD\t", "FLD\t",
	"FLD\t", "FLD\t", "FLD\t", "FLD\t",

	"FXCH\t", "FXCH\t", "FXCH\t", "FXCH\t",
	"FXCH\t", "FXCH\t", "FXCH\t", "FXCH\t",

	"FNOP", "ESC_D9", "ESC_D9", "ESC_D9",
	"ESC_D9", "ESC_D9", "ESC_D9", "ESC_D9",

	"FSTP\t", "FSTP\t", "FSTP\t", "FSTP\t",
	"FSTP\t", "FSTP\t", "FSTP\t", "FSTP\t",

	"FCHS", "FABS", "ESC_D9", "ESC_D9",
	"FTST", "FXAM", "ESC_D9", "ESC_D9",

	"FLD1", "FLDL2T", "FLDL2E", "FLDPI",
	"FLDLG2", "FLDLN2", "FLDZ", "ESC_D9",

	"F2XM1", "FYL2X", "FPTAN", "FPATAN",
	"FXTRACT", "ESC_D9", "FDECSTP", "FINCSTP",

	"FPREM", "FYL2XP1", "FSQRT", "ESC_D9",
	"FRNDINT", "FSCALE", "ESC_D9", "ESC_D9",
};

#endif				/* DEBUG */


/* opcode=0xd9 */
void x86op_esc_coprocess_d9(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
	if (mod != 3) {
		DECODE_PRINTINSTR32(m, x86_fpu_op_d9_tab, mod, rh, rl);
	} else {
		DECODE_PRINTF(m, x86_fpu_op_d9_tab1[(rh << 3) + rl]);
	}
#endif

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		if (rh < 4)
			DECODE_PRINTF2(m, "ST(%d)\n", stkelem);
		else
			DECODE_PRINTF(m, "\n");
		break;
	}

#ifdef X86_FPU_PRESENT
	/* execute */
	switch (mod) {
	case 3:
		switch (rh) {
		case 0:
			x86_fpu_R_fld(m, X86_FPU_STKTOP, stkelem);
			break;
		case 1:
			x86_fpu_R_fxch(m, X86_FPU_STKTOP, stkelem);
			break;
		case 2:
			switch (rl) {
			case 0:
				x86_fpu_R_nop(m);
				break;
			default:
				x86_fpu_illegal(m);
				break;
			}
		case 3:
			x86_fpu_R_fstp(m, X86_FPU_STKTOP, stkelem);
			break;
		case 4:
			switch (rl) {
			case 0:
				x86_fpu_R_fchs(m, X86_FPU_STKTOP);
				break;
			case 1:
				x86_fpu_R_fabs(m, X86_FPU_STKTOP);
				break;
			case 4:
				x86_fpu_R_ftst(m, X86_FPU_STKTOP);
				break;
			case 5:
				x86_fpu_R_fxam(m, X86_FPU_STKTOP);
				break;
			default:
				/* 2,3,6,7 */
				x86_fpu_illegal(m);
				break;
			}
			break;

		case 5:
			switch (rl) {
			case 0:
				x86_fpu_R_fld1(m, X86_FPU_STKTOP);
				break;
			case 1:
				x86_fpu_R_fldl2t(m, X86_FPU_STKTOP);
				break;
			case 2:
				x86_fpu_R_fldl2e(m, X86_FPU_STKTOP);
				break;
			case 3:
				x86_fpu_R_fldpi(m, X86_FPU_STKTOP);
				break;
			case 4:
				x86_fpu_R_fldlg2(m, X86_FPU_STKTOP);
				break;
			case 5:
				x86_fpu_R_fldln2(m, X86_FPU_STKTOP);
				break;
			case 6:
				x86_fpu_R_fldz(m, X86_FPU_STKTOP);
				break;
			default:
				/* 7 */
				x86_fpu_illegal(m);
				break;
			}
			break;

		case 6:
			switch (rl) {
			case 0:
				x86_fpu_R_f2xm1(m, X86_FPU_STKTOP);
				break;
			case 1:
				x86_fpu_R_fyl2x(m, X86_FPU_STKTOP);
				break;
			case 2:
				x86_fpu_R_fptan(m, X86_FPU_STKTOP);
				break;
			case 3:
				x86_fpu_R_fpatan(m, X86_FPU_STKTOP);
				break;
			case 4:
				x86_fpu_R_fxtract(m, X86_FPU_STKTOP);
				break;
			case 5:
				x86_fpu_illegal(m);
				break;
			case 6:
				x86_fpu_R_decstp(m);
				break;
			case 7:
				x86_fpu_R_incstp(m);
				break;
			}
			break;

		case 7:
			switch (rl) {
			case 0:
				x86_fpu_R_fprem(m, X86_FPU_STKTOP);
				break;
			case 1:
				x86_fpu_R_fyl2xp1(m, X86_FPU_STKTOP);
				break;
			case 2:
				x86_fpu_R_fsqrt(m, X86_FPU_STKTOP);
				break;
			case 3:
				x86_fpu_illegal(m);
				break;
			case 4:
				x86_fpu_R_frndint(m, X86_FPU_STKTOP);
				break;
			case 5:
				x86_fpu_R_fscale(m, X86_FPU_STKTOP);
				break;
			case 6:
			case 7:
			default:
				x86_fpu_illegal(m);
				break;
			}
			break;

		default:
			switch (rh) {
			case 0:
				x86_fpu_M_fld(m, X86_FPU_FLOAT,
					      destoffset);
				break;
			case 1:
				x86_fpu_illegal(m);
				break;
			case 2:
				x86_fpu_M_fst(m, X86_FPU_FLOAT,
					      destoffset);
				break;
			case 3:
				x86_fpu_M_fstp(m, X86_FPU_FLOAT,
					       destoffset);
				break;
			case 4:
				x86_fpu_M_fldenv(m, X86_FPU_WORD,
						 destoffset);
				break;
			case 5:
				x86_fpu_M_fldcw(m, X86_FPU_WORD,
						destoffset);
				break;
			case 6:
				x86_fpu_M_fstenv(m, X86_FPU_WORD,
						 destoffset);
				break;
			case 7:
				x86_fpu_M_fstcw(m, X86_FPU_WORD,
						destoffset);
				break;
			}
		}
	}
#endif				/* X86_FPU_PRESENT */
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG

char *x86_fpu_op_da_tab[] = {
	"FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
	"FICOMP\tDWORD PTR ",
	"FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
	"FIDIVR\tDWORD PTR ",

	"FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
	"FICOMP\tDWORD PTR ",
	"FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
	"FIDIVR\tDWORD PTR ",

	"FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
	"FICOMP\tDWORD PTR ",
	"FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
	"FIDIVR\tDWORD PTR ",

	"ESC_DA ", "ESC_DA ", "ESC_DA ", "ESC_DA ",
	"ESC_DA	", "ESC_DA ", "ESC_DA	", "ESC_DA ",
};

#endif				/* DEBUG */


/* opcode=0xda */
void x86op_esc_coprocess_da(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

	DECODE_PRINTINSTR32(m, x86_fpu_op_da_tab, mod, rh, rl);

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		DECODE_PRINTF2(m, "\tST(%d),ST\n", stkelem);
		break;
	}
#ifdef X86_FPU_PRESENT
	switch (mod) {
	case 3:
		x86_fpu_illegal(m);
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_iadd(m, X86_FPU_SHORT, destoffset);
			break;
		case 1:
			x86_fpu_M_imul(m, X86_FPU_SHORT, destoffset);
			break;
		case 2:
			x86_fpu_M_icom(m, X86_FPU_SHORT, destoffset);
			break;
		case 3:
			x86_fpu_M_icomp(m, X86_FPU_SHORT, destoffset);
			break;
		case 4:
			x86_fpu_M_isub(m, X86_FPU_SHORT, destoffset);
			break;
		case 5:
			x86_fpu_M_isubr(m, X86_FPU_SHORT, destoffset);
			break;
		case 6:
			x86_fpu_M_idiv(m, X86_FPU_SHORT, destoffset);
			break;
		case 7:
			x86_fpu_M_idivr(m, X86_FPU_SHORT, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}

#ifdef DEBUG

char *x86_fpu_op_db_tab[] = {
	"FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ",
	    "FISTP\tDWORD PTR ",
	"ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",

	"FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ",
	    "FISTP\tDWORD PTR ",
	"ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",

	"FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ",
	    "FISTP\tDWORD PTR ",
	"ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",
};

#endif				/* DEBUG */


/* opcode=0xdb */
void x86op_esc_coprocess_db(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

#ifdef DEBUG
	if (mod != 3) {
		DECODE_PRINTINSTR32(m, x86_fpu_op_db_tab, mod, rh, rl);
	} else if (rh == 4) {	/* === 11 10 0 nnn */
		switch (rl) {
		case 0:
			DECODE_PRINTF(m, "FENI\n");
			break;
		case 1:
			DECODE_PRINTF(m, "FDISI\n");
			break;
		case 2:
			DECODE_PRINTF(m, "FCLEX\n");
			break;
		case 3:
			DECODE_PRINTF(m, "FINIT\n");
			break;
		}
	} else {
		DECODE_PRINTF2(m, "ESC_DB %0x\n",
			       (mod << 6) + (rh << 3) + (rl));
	}
#endif				/* DEBUG */

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		break;
	case 3:		/* register to register */
		break;
	}

#ifdef X86_FPU_PRESENT
	/* execute */
	switch (mod) {
	case 3:
		switch (rh) {
		case 4:
			switch (rl) {
			case 0:
				x86_fpu_R_feni(m);
				break;
			case 1:
				x86_fpu_R_fdisi(m);
				break;
			case 2:
				x86_fpu_R_fclex(m);
				break;
			case 3:
				x86_fpu_R_finit(m);
				break;
			default:
				x86_fpu_illegal(m);
				break;
			}
			break;
		default:
			x86_fpu_illegal(m);
			break;
		}
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_fild(m, X86_FPU_SHORT, destoffset);
			break;
		case 1:
			x86_fpu_illegal(m);
			break;
		case 2:
			x86_fpu_M_fist(m, X86_FPU_SHORT, destoffset);
			break;
		case 3:
			x86_fpu_M_fistp(m, X86_FPU_SHORT, destoffset);
			break;
		case 4:
			x86_fpu_illegal(m);
			break;
		case 5:
			x86_fpu_M_fld(m, X86_FPU_LDBL, destoffset);
			break;
		case 6:
			x86_fpu_illegal(m);
			break;
		case 7:
			x86_fpu_M_fstp(m, X86_FPU_LDBL, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG
char *x86_fpu_op_dc_tab[] = {
	"FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
	"FCOMP\tQWORD PTR ",
	"FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
	"FDIVR\tQWORD PTR ",

	"FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
	"FCOMP\tQWORD PTR ",
	"FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
	"FDIVR\tQWORD PTR ",

	"FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
	"FCOMP\tQWORD PTR ",
	"FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
	"FDIVR\tQWORD PTR ",

	"FADD\t", "FMUL\t", "FCOM\t", "FCOMP\t",
	"FSUBR\t", "FSUB\t", "FDIVR\t", "FDIV\t",
};
#endif				/* DEBUG */


/* opcode=0xdc */
void x86op_esc_coprocess_dc(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

	DECODE_PRINTINSTR32(m, x86_fpu_op_dc_tab, mod, rh, rl);

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		DECODE_PRINTF2(m, "\tST(%d),ST\n", stkelem);
		break;
	}
#ifdef X86_FPU_PRESENT
	/* execute */
	switch (mod) {
	case 3:
		switch (rh) {
		case 0:
			x86_fpu_R_fadd(m, stkelem, X86_FPU_STKTOP);
			break;
		case 1:
			x86_fpu_R_fmul(m, stkelem, X86_FPU_STKTOP);
			break;
		case 2:
			x86_fpu_R_fcom(m, stkelem, X86_FPU_STKTOP);
			break;
		case 3:
			x86_fpu_R_fcomp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 4:
			x86_fpu_R_fsubr(m, stkelem, X86_FPU_STKTOP);
			break;
		case 5:
			x86_fpu_R_fsub(m, stkelem, X86_FPU_STKTOP);
			break;
		case 6:
			x86_fpu_R_fdivr(m, stkelem, X86_FPU_STKTOP);
			break;
		case 7:
			x86_fpu_R_fdiv(m, stkelem, X86_FPU_STKTOP);
			break;
		}
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_fadd(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 1:
			x86_fpu_M_fmul(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 2:
			x86_fpu_M_fcom(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 3:
			x86_fpu_M_fcomp(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 4:
			x86_fpu_M_fsub(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 5:
			x86_fpu_M_fsubr(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 6:
			x86_fpu_M_fdiv(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 7:
			x86_fpu_M_fdivr(m, X86_FPU_DOUBLE, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG

static char *x86_fpu_op_dd_tab[] = {
	"FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ",
	    "FSTP\tQWORD PTR ",
	"FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

	"FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ",
	    "FSTP\tQWORD PTR ",
	"FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

	"FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ",
	    "FSTP\tQWORD PTR ",
	"FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

	"FFREE\t", "FXCH\t", "FST\t", "FSTP\t",
	"ESC_DD\t2C,", "ESC_DD\t2D,", "ESC_DD\t2E,", "ESC_DD\t2F,",
};

#endif				/* DEBUG */


/* opcode=0xdd */
void x86op_esc_coprocess_dd(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

	DECODE_PRINTINSTR32(m, x86_fpu_op_dd_tab, mod, rh, rl);

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		DECODE_PRINTF2(m, "\tST(%d),ST\n", stkelem);
		break;
	}
#ifdef X86_FPU_PRESENT
	switch (mod) {
	case 3:
		switch (rh) {
		case 0:
			x86_fpu_R_ffree(m, stkelem);
			break;
		case 1:
			x86_fpu_R_fxch(m, stkelem);
			break;
		case 2:
			x86_fpu_R_fst(m, stkelem);	/* register version */
			break;
		case 3:
			x86_fpu_R_fstp(m, stkelem);	/* register version */
			break;
		default:
			x86_fpu_illegal(m);
			break;
		}
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_fld(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 1:
			x86_fpu_illegal(m);
			break;
		case 2:
			x86_fpu_M_fst(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 3:
			x86_fpu_M_fstp(m, X86_FPU_DOUBLE, destoffset);
			break;
		case 4:
			x86_fpu_M_frstor(m, X86_FPU_WORD, destoffset);
			break;
		case 5:
			x86_fpu_illegal(m);
			break;
		case 6:
			x86_fpu_M_fsave(m, X86_FPU_WORD, destoffset);
			break;
		case 7:
			x86_fpu_M_fstsw(m, X86_FPU_WORD, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG

static char *x86_fpu_op_de_tab[] = {
	"FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
	"FICOMP\tWORD PTR ",
	"FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
	"FIDIVR\tWORD PTR ",

	"FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
	"FICOMP\tWORD PTR ",
	"FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
	"FIDIVR\tWORD PTR ",

	"FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
	"FICOMP\tWORD PTR ",
	"FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
	"FIDIVR\tWORD PTR ",

	"FADDP\t", "FMULP\t", "FCOMP\t", "FCOMPP\t",
	"FSUBRP\t", "FSUBP\t", "FDIVRP\t", "FDIVP\t",
};

#endif				/* DEBUG */


/* opcode=0xde */
void x86op_esc_coprocess_de(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

	DECODE_PRINTINSTR32(m, x86_fpu_op_de_tab, mod, rh, rl);

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		DECODE_PRINTF2(m, "\tST(%d),ST\n", stkelem);
		break;
	}
#ifdef X86_FPU_PRESENT
	switch (mod) {
	case 3:
		switch (rh) {
		case 0:
			x86_fpu_R_faddp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 1:
			x86_fpu_R_fmulp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 2:
			x86_fpu_R_fcomp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 3:
			if (stkelem == 1)
				x86_fpu_R_fcompp(m, stkelem,
						 X86_FPU_STKTOP);
			else
				x86_fpu_illegal(m);
			break;
		case 4:
			x86_fpu_R_fsubrp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 5:
			x86_fpu_R_fsubp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 6:
			x86_fpu_R_fdivrp(m, stkelem, X86_FPU_STKTOP);
			break;
		case 7:
			x86_fpu_R_fdivp(m, stkelem, X86_FPU_STKTOP);
			break;
		}
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_fiadd(m, X86_FPU_WORD, destoffset);
			break;
		case 1:
			x86_fpu_M_fimul(m, X86_FPU_WORD, destoffset);
			break;
		case 2:
			x86_fpu_M_ficom(m, X86_FPU_WORD, destoffset);
			break;
		case 3:
			x86_fpu_M_ficomp(m, X86_FPU_WORD, destoffset);
			break;
		case 4:
			x86_fpu_M_fisub(m, X86_FPU_WORD, destoffset);
			break;
		case 5:
			x86_fpu_M_fisubr(m, X86_FPU_WORD, destoffset);
			break;
		case 6:
			x86_fpu_M_fidiv(m, X86_FPU_WORD, destoffset);
			break;
		case 7:
			x86_fpu_M_fidivr(m, X86_FPU_WORD, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef DEBUG

static char *x86_fpu_op_df_tab[] = {
	/* mod == 00 */
	"FILD\tWORD PTR ", "ESC_DF\t39\n", "FIST\tWORD PTR ",
	    "FISTP\tWORD PTR ",
	"FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
	"FISTP\tQWORD PTR ",

	/* mod == 01 */
	"FILD\tWORD PTR ", "ESC_DF\t39 ", "FIST\tWORD PTR ",
	    "FISTP\tWORD PTR ",
	"FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
	"FISTP\tQWORD PTR ",

	/* mod == 10 */
	"FILD\tWORD PTR ", "ESC_DF\t39 ", "FIST\tWORD PTR ",
	    "FISTP\tWORD PTR ",
	"FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
	"FISTP\tQWORD PTR ",

	/* mod == 11 */
	"FFREE\t", "FXCH\t", "FST\t", "FSTP\t",
	"ESC_DF\t3C,", "ESC_DF\t3D,", "ESC_DF\t3E,", "ESC_DF\t3F,"
};

#endif				/* DEBUG */


/* opcode=0xdf */
void x86op_esc_coprocess_df(SysEnv * m)
{
	u16 mod, rl, rh;
	u16 destoffset;
	u8 stkelem;

	START_OF_INSTR(m);
	FETCH_DECODE_MODRM(m, mod, rh, rl);

	DECODE_PRINTINSTR32(m, x86_fpu_op_df_tab, mod, rh, rl);

	switch (mod) {
	case 0:
		destoffset = decode_rm00_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 1:
		destoffset = decode_rm01_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 2:
		destoffset = decode_rm10_address(m, rl);
		DECODE_PRINTF(m, "\n");
		break;
	case 3:		/* register to register */
		stkelem = rl;
		DECODE_PRINTF2(m, "\tST(%d)\n", stkelem);
		break;
	}
#ifdef X86_FPU_PRESENT
	switch (mod) {
	case 3:
		switch (rh) {
		case 0:
			x86_fpu_R_ffree(m, stkelem);
			break;
		case 1:
			x86_fpu_R_fxch(m, stkelem);
			break;
		case 2:
			x86_fpu_R_fst(m, stkelem);	/* register version */
			break;
		case 3:
			x86_fpu_R_fstp(m, stkelem);	/* register version */
			break;
		default:
			x86_fpu_illegal(m);
			break;
		}
		break;
	default:
		switch (rh) {
		case 0:
			x86_fpu_M_fild(m, X86_FPU_WORD, destoffset);
			break;
		case 1:
			x86_fpu_illegal(m);
			break;
		case 2:
			x86_fpu_M_fist(m, X86_FPU_WORD, destoffset);
			break;
		case 3:
			x86_fpu_M_fistp(m, X86_FPU_WORD, destoffset);
			break;
		case 4:
			x86_fpu_M_fbld(m, X86_FPU_BSD, destoffset);
			break;
		case 5:
			x86_fpu_M_fild(m, X86_FPU_LONG, destoffset);
			break;
		case 6:
			x86_fpu_M_fbstp(m, X86_FPU_BSD, destoffset);
			break;
		case 7:
			x86_fpu_M_fistp(m, X86_FPU_LONG, destoffset);
			break;
		}
	}
#endif
	DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}
