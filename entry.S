/*
 * alpha/entry.S
 *
 * kernel entry-points
 * based on entry.S from Kernel 2.2.x
 *
 * Stefan Reinauer, <stepan@suse.de>
 */

#include <alpha/regdef.h>
#include <asm/system.h>
#include <linux/config.h>
#include "autoconf.h"

#define SIGCHLD 20

/*
 * These offsets must match with alpha_mv in <asm/machvec.h>.
 */
#define HAE_CACHE	0
#define HAE_REG		8


/*
 * stack offsets
 */
#define SP_OFF		184

#define SWITCH_STACK_SIZE 320

/*
 * task structure offsets
 */
#define TASK_STATE		0
#define TASK_FLAGS		8
#define TASK_SIGPENDING		16
#define TASK_ADDR_LIMIT		24	
#define TASK_EXEC_DOMAIN	32
#define TASK_NEED_RESCHED	40

/*
 * task flags (must match include/linux/sched.h):
 */
#define	PF_PTRACED	0x00000010

/*
 * This defines the normal kernel pt-regs layout.
 *
 * regs 9-15 preserved by C code
 * regs 16-18 saved by PAL-code
 * regs 29-30 saved and set up by PAL-code
 * JRP - Save regs 16-18 in a special area of the stack, so that
 * the palcode-provided values are available to the signal handler.
 */

#define SAVE_ALL			\
	subq	$30,184,$30;		\
	stq	$0,0($30);		\
	stq	$1,8($30);		\
	stq	$2,16($30);		\
	stq	$3,24($30);		\
	stq	$4,32($30);		\
	stq	$28,144($30);		\
	lda	$2,alpha_mv;		\
	stq	$5,40($30);		\
	stq	$6,48($30);		\
	stq	$7,56($30);		\
	stq	$8,64($30);		\
	stq	$19,72($30);		\
	stq	$20,80($30);		\
	stq	$21,88($30);		\
	ldq	$2,HAE_CACHE($2);	\
	stq	$22,96($30);		\
	stq	$23,104($30);		\
	stq	$24,112($30);		\
	stq	$25,120($30);		\
	stq	$26,128($30);		\
	stq	$27,136($30);		\
	stq	$2,152($30);		\
	stq	$16,160($30);		\
	stq	$17,168($30);		\
	stq	$18,176($30)

#define RESTORE_ALL			\
	lda	$19,alpha_mv;		\
	ldq	$0,0($30);		\
	ldq	$1,8($30);		\
	ldq	$2,16($30);		\
	ldq	$3,24($30);		\
	ldq	$20,152($30);		\
	ldq	$21,HAE_CACHE($19);	\
	ldq	$4,32($30);		\
	ldq	$5,40($30);		\
	ldq	$6,48($30);		\
	ldq	$7,56($30);		\
	subq	$20,$21,$20;		\
	ldq	$8,64($30);		\
	beq	$20,99f;		\
	ldq	$20,HAE_REG($19);	\
	addq	$31,7,$16;		\
	call_pal PAL_swpipl;		\
	stq	$21,HAE_CACHE($19);	\
	stq	$21,0($20);		\
	mov	$0,$16;			\
	call_pal PAL_swpipl;		\
	ldq	$0,0($30);		\
	ldq	$1,8($30);		\
99:;					\
	ldq	$19,72($30);		\
	ldq	$20,80($30);		\
	ldq	$21,88($30);		\
	ldq	$22,96($30);		\
	ldq	$23,104($30);		\
	ldq	$24,112($30);		\
	ldq	$25,120($30);		\
	ldq	$26,128($30);		\
	ldq	$27,136($30);		\
	ldq	$28,144($30);		\
	addq	$30,184,$30

.text
.set noat
#if defined(__linux__) && !defined(__ELF__)
  .set singlegp
#endif

.align 3
.globl	entInt
.ent	entInt
entInt:
	SAVE_ALL
	lda	$8,0x3fff
	lda	$26,ret_from_sys_call
	bic	$30,$8,$8
	jsr	$31,do_entInt
.end entInt

.align 3
.globl	entMM
.ent	entMM
entMM:
	SAVE_ALL
/* save $9 - $15 so the inline exception code can manipulate them.  */
	subq	$30,56,$30
	stq	$9,0($30)
	stq	$10,8($30)
	stq	$11,16($30)
	stq	$12,24($30)
	stq	$13,32($30)
	stq	$14,40($30)
	stq	$15,48($30)
	addq	$30,56,$19
/* handle the fault */
	lda	$8,0x3fff
	bic	$30,$8,$8
	jsr	$26,do_page_fault
/* reload the registers after the exception code played.  */
	ldq	$9,0($30)
	ldq	$10,8($30)
	ldq	$11,16($30)
	ldq	$12,24($30)
	ldq	$13,32($30)
	ldq	$14,40($30)
	ldq	$15,48($30)
	addq	$30,56,$30
/* finish up the syscall as normal.  */
	br	ret_from_sys_call
.end entMM

#if 0
.align 3
.globl	entArith
.ent	entArith
entArith:
	SAVE_ALL
	lda	$8,0x3fff
	lda	$26,ret_from_sys_call
	bic	$30,$8,$8
	jsr	$31,do_entArith
.end entArith
#endif
#if 0
.align 3
.globl	entIF
.ent	entIF
entIF:
	SAVE_ALL
	lda	$8,0x3fff
	lda	$26,ret_from_sys_call
	bic	$30,$8,$8
	jsr	$31,do_entIF
.end entIF
#endif

/*
 * Fork() is one of the special system calls: it needs to
 * save the callee-saved regs so that the regs can be found
 * for the new process.. We save them in the "context switch"
 * stack format (see arch/alpha/kernel/process.c).
 *
 * Also, for the kernel fork, we need to fake the system call
 * stack buildup, as we can't do system calls from kernel space.
 */
.align 3
.ent	kernel_clone
kernel_clone:
	.frame $30, 0, $26
	.prologue 0
	subq	$30,6*8,$30
	stq	$31,0($30)
	stq	$26,8($30)
	stq	$29,16($30)
	stq	$16,24($30)
	stq	$17,32($30)
	stq	$18,40($30)
	bis	$31,2,$0	/* Register v0: syscall nr for fork() */
	SAVE_ALL
	bsr	$26,sys_clone
	stq	$0,0($30)
	br	ret_from_sys_call
.end	kernel_clone

/*
 * __kernel_thread(clone_flags, fn, arg)
 */
.align 3
.globl	__kernel_thread
.ent	__kernel_thread
__kernel_thread:
	ldgp	$29,0($27)	/* we can be called from a module */
	.frame $30, 4*8, $26
	subq	$30,4*8,$30
	stq	$10,16($30)
	stq	$9,8($30)
	stq	$26,0($30)
	.prologue 1
	mov	$17,$9		/* save fn */		
	mov	$18,$10		/* save arg */
	bsr	$26,kernel_clone
	bne	$20,1f		/* $20 is non-zero in child */
	ldq	$26,0($30)
	ldq	$9,8($30)
	ldq	$10,16($30)
	addq	$30,4*8,$30
	ret	$31,($26),1
/* this is in child: look out as we don't have any stack here.. */
1:	mov	$9,$27		/* get fn */
	lda	$8,0x3fff
	mov	$10,$16		/* get arg */
	bic	$30,$8,$8	/* get current */
	jsr	$26,($27)
	ldgp	$29,0($26)
	mov	$0,$16
	mov	$31,$26
	jsr	$31,sys_exit
.end	__kernel_thread

/*
 * __kernel_execve(path, argv, envp, regs)
 */
.align 3
.globl	__kernel_execve
.ent	__kernel_execve
__kernel_execve:
	ldgp	$29,0($27)	/* we can be called from modules.  */
	subq	$30,16,$30
	.frame	$30,16,$26,0
	stq	$26,0($30)
	stq	$19,8($30)
	.prologue 1
	jsr	$26,do_execve
	bne	$0,1f		/* error! */
	ldq	$30,8($30)
	br	$31,ret_from_sys_call
1:	ldq	$26,0($30)
	addq	$30,16,$30
	ret
.end	__kernel_execve

.align 3
.ent	do_switch_stack
do_switch_stack:
	lda	$30,-SWITCH_STACK_SIZE($30)
	stq	$9,0($30)
	stq	$10,8($30)
	stq	$11,16($30)
	stq	$12,24($30)
	stq	$13,32($30)
	stq	$14,40($30)
	stq	$15,48($30)
	stq	$26,56($30)
	stt	$f0,64($30)
	stt	$f1,72($30)
	stt	$f2,80($30)
	stt	$f3,88($30)
	stt	$f4,96($30)
	stt	$f5,104($30)
	stt	$f6,112($30)
	stt	$f7,120($30)
	stt	$f8,128($30)
	stt	$f9,136($30)
	stt	$f10,144($30)
	stt	$f11,152($30)
	stt	$f12,160($30)
	stt	$f13,168($30)
	stt	$f14,176($30)
	stt	$f15,184($30)
	stt	$f16,192($30)
	stt	$f17,200($30)
	stt	$f18,208($30)
	stt	$f19,216($30)
	stt	$f20,224($30)
	stt	$f21,232($30)
	stt	$f22,240($30)
	stt	$f23,248($30)
	stt	$f24,256($30)
	stt	$f25,264($30)
	stt	$f26,272($30)
	stt	$f27,280($30)
	mf_fpcr	$f0		# get fpcr
	stt	$f28,288($30)
	stt	$f29,296($30)
	stt	$f30,304($30)
	stt	$f0,312($30)	# save fpcr in slot of $f31
	ldt	$f0,64($30)	# dont let "do_switch_stack" change fp state.
	ret	$31,($1),1
.end do_switch_stack

.align 3
.ent	undo_switch_stack
undo_switch_stack:
	ldq	$9,0($30)
	ldq	$10,8($30)
	ldq	$11,16($30)
	ldq	$12,24($30)
	ldq	$13,32($30)
	ldq	$14,40($30)
	ldq	$15,48($30)
	ldq	$26,56($30)
	ldt	$f30,312($30)	# get saved fpcr
	ldt	$f0,64($30)
	ldt	$f1,72($30)
	ldt	$f2,80($30)
	ldt	$f3,88($30)
	mt_fpcr	$f30		# install saved fpcr
	ldt	$f4,96($30)
	ldt	$f5,104($30)
	ldt	$f6,112($30)
	ldt	$f7,120($30)
	ldt	$f8,128($30)
	ldt	$f9,136($30)
	ldt	$f10,144($30)
	ldt	$f11,152($30)
	ldt	$f12,160($30)
	ldt	$f13,168($30)
	ldt	$f14,176($30)
	ldt	$f15,184($30)
	ldt	$f16,192($30)
	ldt	$f17,200($30)
	ldt	$f18,208($30)
	ldt	$f19,216($30)
	ldt	$f20,224($30)
	ldt	$f21,232($30)
	ldt	$f22,240($30)
	ldt	$f23,248($30)
	ldt	$f24,256($30)
	ldt	$f25,264($30)
	ldt	$f26,272($30)
	ldt	$f27,280($30)
	ldt	$f28,288($30)
	ldt	$f29,296($30)
	ldt	$f30,304($30)
	lda	$30,SWITCH_STACK_SIZE($30)
	ret	$31,($1),1
.end undo_switch_stack

.align 3
.globl	entUna
.ent	entUna
entUna:
	lda	$30,-256($30)
	stq	$0,0($30)
	ldq	$0,256($30)	/* get PS */
	stq	$1,8($30)
	stq	$2,16($30)
	stq	$3,24($30)

	stq	$5,40($30)
	stq	$6,48($30)
	stq	$7,56($30)
	stq	$8,64($30)
	stq	$9,72($30)
	stq	$10,80($30)
	stq	$11,88($30)
	stq	$12,96($30)
	stq	$13,104($30)
	stq	$14,112($30)
	stq	$15,120($30)
	/* 16-18 PAL-saved */
	stq	$19,152($30)
	stq	$20,160($30)
	stq	$21,168($30)
	stq	$22,176($30)
	stq	$23,184($30)
	stq	$24,192($30)
	stq	$25,200($30)
	stq	$26,208($30)
	stq	$27,216($30)
	stq	$28,224($30)
	stq	$29,232($30)
	lda	$8,0x3fff
	stq	$31,248($30)
	bic	$30,$8,$8
	jsr	$26,do_entUna
	ldq	$0,0($30)
	ldq	$1,8($30)
	ldq	$2,16($30)
	ldq	$3,24($30)
	ldq	$4,32($30)
	ldq	$5,40($30)
	ldq	$6,48($30)
	ldq	$7,56($30)
	ldq	$8,64($30)
	ldq	$9,72($30)
	ldq	$10,80($30)
	ldq	$11,88($30)
	ldq	$12,96($30)
	ldq	$13,104($30)
	ldq	$14,112($30)
	ldq	$15,120($30)
	/* 16-18 PAL-saved */
	ldq	$19,152($30)
	ldq	$20,160($30)
	ldq	$21,168($30)
	ldq	$22,176($30)
	ldq	$23,184($30)
	ldq	$24,192($30)
	ldq	$25,200($30)
	ldq	$26,208($30)
	ldq	$27,216($30)
	ldq	$28,224($30)
	ldq	$29,232($30)
	lda	$30,256($30)
	call_pal PAL_rti
.end entUna

.align 3
.globl	sys_clone
.ent	sys_clone
sys_clone:
	bsr	$1,do_switch_stack
	/* arg1 and arg2 come from the user */
	mov	$30,$18
	jsr	$26,alpha_clone
	bsr	$1,undo_switch_stack
	ret	$31,($26),1
.end	sys_clone

.align 3
.globl  alpha_switch_to
.ent    alpha_switch_to
alpha_switch_to:
	.prologue 0
	bsr	$1,do_switch_stack
	call_pal PAL_swpctx
	unop
	bsr	$1,undo_switch_stack
	mov	$17,$0
	ret	$31,($26),1
.end alpha_switch_to

.align 3
.ent	ret_from_sys_call
ret_from_sys_call:
	cmovne	$26,0,$19		/* $19 = 0 => non-restartable */
	/* check bottom half interrupts */
	ldq	$3,bh_active
	ldq	$4,bh_mask
	and	$3,$4,$2
	bne	$2,handle_bottom_half
ret_from_handle_bh:
	ldq	$0,SP_OFF($30)
	and	$0,8,$0
	beq	$0,restore_all
ret_from_reschedule:
	ldq	$2,TASK_NEED_RESCHED($8)
	lda	$4,init_task_union
	bne	$2,reschedule
	xor	$4,$8,$4
	ldl	$5,TASK_SIGPENDING($8)
	beq	$4,restore_all
	bne	$5,signal_return
restore_all:
	RESTORE_ALL
	call_pal PAL_rti

	.align 3
handle_bottom_half:
	subq	$30,16,$30
	stq	$19,0($30)	/* save syscall nr */
	stq	$20,8($30)	/* and error indication (a3) */
	jsr	$26,do_bottom_half
	ldq	$19,0($30)
	ldq	$20,8($30)
	addq	$30,16,$30
	br	ret_from_handle_bh

.align 3
signal_return:
	mov	$30,$17
	br	$1,do_switch_stack
	mov	$30,$18
	mov	$31,$16
	jsr	$26,do_signal
	bsr	$1,undo_switch_stack
	br	restore_all

.end ret_from_sys_call

#ifdef __SMP__
        .globl  ret_from_smp_fork
.align 3
.ent ret_from_smp_fork
ret_from_smp_fork:
	lda	$26,ret_from_sys_call
	mov	$17,$16
	jsr	$31,schedule_tail
.end ret_from_smp_fork
#endif /* __SMP__ */

.align 3
.ent reschedule
reschedule:
	subq	$30,16,$30
	stq	$19,0($30)	/* save syscall nr */
	stq	$20,8($30)	/* and error indication (a3) */
	jsr	$26,schedule
	ldq	$19,0($30)
	ldq	$20,8($30)
	addq	$30,16,$30
	br	ret_from_reschedule
.end reschedule

/* additionally we need wrsp */

        .globl  wrsp
        .ent    wrsp
wrsp:
        .option O1
        .frame sp, 0, ra
        bis a0, zero, sp
        ret     zero, (ra)
        .end wrsp
