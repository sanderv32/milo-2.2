/* kernel.c
 *
 * Dummy and minimal functions used by linux device drivers
 * and startup code.
 * 
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Stefan Reinauer, <stepan@suse.de>
 */

#include <linux/config.h>
#include <linux/malloc.h>
#include "milo.h"

#ifdef CONFIG_ALPHA_GENERIC
int alpha_using_srm=0, alpha_use_srm_setup=0;
#endif

/*****************************************************
 *     Shutdown functions                            *
 *****************************************************/

void do_exit(long code)
{
#ifdef DEBUG_KERNEL
	printk("do_exit()\n");
#endif
	halt();
}

asmlinkage int sys_exit(int error_code)
{
	do_exit((error_code & 0xff) << 8);
}

void generic_kill_arch(int mode, char *restart_cmd)
{
#ifdef DEBUG_KERNEL
	printk("generic_kill_arch()\n");
#endif
	halt();
}

/*****************************************************
 *     Memory management dummy functions             *
 *****************************************************/

void exit_mm(struct task_struct *tsk)
{
#ifdef DEBUG_KERNEL
	printk("exit_mm()\n");
#endif
}

/*****************************************************
 *     process handling                              *
 *****************************************************/

int do_execve(char *filename, char **argv, char **envp,
	      struct pt_regs *regs)
{
#ifdef DEBUG_KERNEL
	printk("do_execve()\n");
#endif
	return -E2BIG;
}

int alpha_clone(unsigned long clone_flags, unsigned long usp,
		struct switch_stack *swstack)
{
	if (!usp)
		usp = rdusp();
	return -ENOMEM;
}

asmlinkage int
do_signal(sigset_t * oldset, struct pt_regs *regs, struct switch_stack *sw,
	  unsigned long r0, unsigned long r19)
{
#ifdef DEBUG_KERNEL
	printk("do_signal();\n");
#endif
	return 0;
}


/*****************************************************
 *     Other stuff                                   *
 *****************************************************/

int __down_interruptible(struct semaphore *sem)
{
#ifdef DEBUG_KERNEL
	printk(" __down_interruptible()\n");
#endif
	return 0;
}

int permission(struct inode * inode,int mask)
{
#ifdef DEBUG_KERNEL
	printk(" permission()\n");
#endif
	return 0;
}

int invalidate_inodes(struct super_block * sb)
{
#ifdef DEBUG_KERNEL
	printk("invalidate_inodes()\n");
#endif
	return 0;
}

void es1888_init(void)
{
}

void console_map_init(void)
{
}

/* ******************************************************************** */

/* Page faults should be easy since we use 1-to-1 virt2phys mapping */

asmlinkage void
do_page_fault(unsigned long address, unsigned long mmcsr,
	      long cause, struct pt_regs *regs)
{
#ifdef DEBUG_KERNEL
#if 0
	printk("Unable to handle kernel paging request at "
	       "virtual address %016lx\n", address);
#endif
#endif
}

#if 0
asmlinkage void
do_entIF(unsigned long type, unsigned long a1,
	 unsigned long a2, unsigned long a3, unsigned long a4,
	 unsigned long a5, struct pt_regs regs)
{
#ifdef DEBUG_KERNEL
	printk("Oops: You are lucky, got an instruction fault.\n");
#endif
}
#endif
