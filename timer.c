/*****************************************************************************

       Copyright © 1995, 1996 Digital Equipment Corporation,
                       Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, provided  
that the copyright notice and this permission notice appear in all copies  
of software and supporting documentation, and that the name of Digital not  
be used in advertising or publicity pertaining to distribution of the software 
without specific, written prior permission. Digital grants this permission 
provided that you prominently mark, as not part of the original, any 
modifications made to this software or documentation.

Digital Equipment Corporation disclaims all warranties and/or guarantees  
with regard to this software, including all implied warranties of fitness for 
a particular purpose and merchantability, and makes no representations 
regarding the use of, or the results of the use of, the software and 
documentation in terms of correctness, accuracy, reliability, currentness or
otherwise; and you rely on the software, documentation and results solely at 
your own risk. 

******************************************************************************/
#include <linux/types.h>
#include <linux/config.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/mc146818rtc.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include "milo.h"
#include "uart.h"

#define RTC_IRQ    8
#if defined(CONFIG_RTC) && !defined(CONFIG_RTC_LIGHT)
#define TIMER_IRQ  0		/* timer is the pit */
#else
#define TIMER_IRQ  RTC_IRQ	/* timer is the rtc */
#endif

volatile struct timeval xtime;	/* The current time */

struct timer_struct timer_table[32];

unsigned long timer_active;

static struct timer_list timer_head =
    { &timer_head, &timer_head, ~0, 0, NULL };

struct tq_struct tq_last = {
	&tq_last, 0, 0, 0
};

DECLARE_TASK_QUEUE(tq_timer);
DECLARE_TASK_QUEUE(tq_immediate);
DECLARE_TASK_QUEUE(tq_scheduler);

unsigned long volatile jiffies = 0;

void timer_interrupt(int irq, void *dev, struct pt_regs *regs);
void do_timer(struct pt_regs *regs);

/* External things */
extern int milo_suppress_printk;
extern int cons_puts(int unit, char *buf);

#if defined(CONFIG_RTC) && !defined(CONFIG_RTC_LIGHT)
void rtc_init_pit(void)
{
	unsigned char control;

	/* Turn off RTC interrupts before /dev/rtc is initialized */
	control = CMOS_READ(RTC_CONTROL);
	control &= ~(RTC_PIE | RTC_AIE | RTC_UIE);
	CMOS_WRITE(control, RTC_CONTROL);
	(void) CMOS_READ(RTC_INTR_FLAGS);

	request_region(0x40, 0x20, "timer");	/* reserve pit */

	/* Setup interval timer.  */
	outb(0x34, 0x43);	/* binary, mode 2, LSB/MSB, ch 0 */
	outb(LATCH & 0xff, 0x40);	/* LSB */
	outb(LATCH >> 8, 0x40);	/* MSB */

	outb(0xb6, 0x43);	/* pit counter 2: speaker */
	outb(0x31, 0x42);
	outb(0x13, 0x42);
}
#endif

void
generic_init_pit (void)
{
        unsigned char x;

        /* Reset periodic interrupt frequency.  */
        x = CMOS_READ(RTC_FREQ_SELECT) & 0x3f;
        if (x != 0x26 && x != 0x19 && x != 0x06) {
                printk("Setting RTC_FREQ to 1024 Hz (%x)\n", x);
                CMOS_WRITE(0x26, RTC_FREQ_SELECT);
        }

        /* Turn on periodic interrupts.  */
        x = CMOS_READ(RTC_CONTROL);
        if (!(x & RTC_PIE)) {
                printk("Turning on RTC interrupts.\n");
                x |= RTC_PIE;
                x &= ~(RTC_AIE | RTC_UIE);
                CMOS_WRITE(x, RTC_CONTROL);
        }
        (void) CMOS_READ(RTC_INTR_FLAGS);

        request_region(RTC_PORT(0), 0x10, "timer"); /* reserve rtc */

        outb(0x36, 0x43);       /* pit counter 0: system timer */
        outb(0x00, 0x40);
        outb(0x00, 0x40);

        outb(0xb6, 0x43);       /* pit counter 2: speaker */
        outb(0x31, 0x42);
        outb(0x13, 0x42);
}

#ifdef DEBUG_TIMER
/*****************************************************************************
 * debug probing routines                                                    *
 *****************************************************************************/
void timer_probe(void)
{
	unsigned long flags;
	unsigned long volatile ticks;

	/* let some ticks happen */
	printk("Checking for interval timers ticking\n");

	/* get jiffies, drop ipl and wait a bit */
	save_flags(flags);
	ticks = jiffies;
	sti();
	udelay(100);
	restore_flags(flags);

	/* did jiffies change? */
	if (ticks != jiffies)
		printk("Timers are ticking\n");
	else {
		printk("Timers not ticking\n");
	}
}
#endif

/*****************************************************************************
 * initialise timers                                                         *
 *****************************************************************************/
void init_timers(void)
{
	unsigned long flags;

	/* init the system time to null */
	xtime.tv_sec = 0;
	xtime.tv_usec = 0;

	/*
	 * Initialize Interval Timers:
	 */
	outb(0x54, 0x43);	/* counter 1: refresh timer */
	outb(0x18, 0x41);

	if ( strncmp(alpha_mv.vector_name, "Ruffian", 7) == 0 ) {
		outb(0x36, 0x43);	/* counter 0: system timer */
		outb(0x8d, 0x40);	/* Ruffian uses intel timer */
		outb(0x04, 0x40);	/* for the system tick.    */
	} else {
		/* We have no Ruffian */
		outb(0x36, 0x43);	/* counter 0: system timer */
		outb(0x00, 0x40);
		outb(0x00, 0x40);
	}

	outb(0xb6, 0x43);	/* counter 2: speaker */
	outb(0x31, 0x42);
	outb(0x13, 0x42);

	/* we must try to guarantee 1024 ticks/sec from the RTC */
	save_flags(flags);
	cli();

	if ((CMOS_READ(RTC_FREQ_SELECT) & 0x3f) != 0x26) {
		printk("init_timers: setting RTC frequency to 1024 Hz\n");
		CMOS_WRITE(0x26, RTC_FREQ_SELECT);
	}

	restore_flags(flags);

#ifdef DEBUG_TIMER
	timer_probe();
#endif
}

int init_timerirq(void)
{
	void (*irq_handler) (int, void *, struct pt_regs *);
	irq_handler = timer_interrupt;
	if (request_irq(TIMER_IRQ, irq_handler, 0, "timer", NULL)) {
		printk("Could not allocate IRQ handler for timer.\n");
		return -1;
	}
	return 0;
}

/*****************************************************************************
 * Timer routines                                                            *
 *****************************************************************************/

void add_timer(struct timer_list *timer)
{
	unsigned long flags;
	struct timer_list *p;

	p = &timer_head;
	save_flags(flags);
	cli();

	do {
		p = p->next;
	} while (timer->expires > p->expires);

	timer->next = p;
	timer->prev = p->prev;
	p->prev = timer;
	timer->prev->next = timer;

	restore_flags(flags);
}

int del_timer(struct timer_list *timer)
{
	unsigned long flags;
	save_flags(flags);

	cli();
	if (timer->next) {
		timer->next->prev = timer->prev;
		timer->prev->next = timer->next;
		timer->next = timer->prev = NULL;
		restore_flags(flags);
		return 1;
	}
	restore_flags(flags);
	return 0;
}

void timer_interrupt(int irq, void *dev, struct pt_regs *regs)
{
	do_timer(regs);
}

void do_timer(struct pt_regs *regs)
{
	struct timer_list *timer;
	unsigned long flags;

	save_flags(flags);
	cli();

	if (xtime.tv_usec >= 1000000) {
		xtime.tv_usec -= 1000000;
		xtime.tv_sec++;
	}

	jiffies++;

	/*
	 * If printk () messages are suppressed, show we're still alive while
	 * setting up devices.
	 */

	if (milo_suppress_printk && jiffies % HZ == 1)
		cons_puts(0, ".");

	while ((timer = timer_head.next) != &timer_head
	       && timer->expires <= jiffies) {
		void (*fn) (unsigned long) = timer->function;
		unsigned long data = timer->data;

		timer->next->prev = timer->prev;
		timer->prev->next = timer->next;
		timer->next = timer->prev = NULL;
		fn(data);
	}

	/* now the tqueue stuff */
	if (tq_timer != &tq_last) {
#ifdef DEBUG_TIMER
		printk("do_timer: calling run_task_queue @ 0x%p\n",
		       &tq_timer);
#endif
		run_task_queue(&tq_timer);
	}

	restore_flags(flags);
}

void it_real_fn(unsigned long thing)
{
	printk("it_real_fn(): called\n");
}

void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;

	save_flags(flags);
	cli();
	*tv = xtime;
	restore_flags(flags);
}

/* this will be accurate for machines rated at 200 BogoMIPS
 * it will be too long for slow machines, and too fast for others,
 * but...  this will be used by the <asm-alpha/delay.h> "udelay" 
 * macro whenever the "calibrate_delay" routine has *NOT* been 
 * called beforehand.
 */
unsigned long loops_per_sec = 100000000UL;

/*****************************************************************************/
/* this is imported directly from init/main.c of the kernel                  */

/* This is the number of bits of precision for the loops_per_second.  Each
   bit takes on average 1.5/HZ seconds.  This (like the original) is a little
   better than 1% */
#define LPS_PREC 8

void calibrate_delay(void)
{
	unsigned long ticks;
	int loopbit;
	int lps_precision = LPS_PREC;

/* this should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
	loops_per_sec = (1UL << 12);

	swpipl(0);

	if (milo_verbose)
		printk("Calibrating delay loop.. ");

	while (loops_per_sec <<= 1) {
		/* wait for "start of" clock tick */
		ticks = jiffies;
		while (ticks == jiffies)
			mb();
		/* nothing */

		/* Go .. */
		ticks = jiffies;
		__delay(loops_per_sec);
		ticks = jiffies - ticks;
		if (ticks)
			break;
	}

/* Do a binary approximation to get loops_per_second set to equal one clock
   (up to lps_precision bits) */
	loops_per_sec >>= 1;
	loopbit = loops_per_sec;
	while (lps_precision-- && (loopbit >>= 1)) {
		loops_per_sec |= loopbit;
		ticks = jiffies;
		while (ticks == jiffies);
		ticks = jiffies;
		__delay(loops_per_sec);
		if (jiffies != ticks)	/* longer than 1 tick */
			loops_per_sec &= ~loopbit;
	}

/* finally, adjust loops per second in terms of seconds instead of clocks */
	loops_per_sec *= HZ;

	/* Round the value and print it */
	if (milo_verbose)
		printk("ok - %lu.%02lu BogoMIPS\n",
		       (loops_per_sec + 2500) / 500000,
		       ((loops_per_sec + 2500) / 5000) % 100);
}



/* Needed by some kernel drivers */

/*
 * Event timer code
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

struct timer_vec {
        int index;
        struct timer_list *vec[TVN_SIZE];
};

struct timer_vec_root {
        int index;
        struct timer_list *vec[TVR_SIZE];
};

static struct timer_vec tv5 = { 0 };
static struct timer_vec tv4 = { 0 };
static struct timer_vec tv3 = { 0 };
static struct timer_vec tv2 = { 0 };
static struct timer_vec_root tv1 = { 0 };
 
static unsigned long timer_jiffies = 0;

static inline void insert_timer(struct timer_list *timer,
                                struct timer_list **vec, int idx)
{
        if ((timer->next = vec[idx]))
                vec[idx]->prev = timer;
        vec[idx] = timer;
        timer->prev = (struct timer_list *)&vec[idx];
}

static inline void internal_add_timer(struct timer_list *timer)
{
        /*
         * must be cli-ed when calling this
         */
        unsigned long expires = timer->expires;
        unsigned long idx = expires - timer_jiffies;

        if (idx < TVR_SIZE) {
                int i = expires & TVR_MASK;
                insert_timer(timer, tv1.vec, i);
        } else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
                int i = (expires >> TVR_BITS) & TVN_MASK;
                insert_timer(timer, tv2.vec, i);
        } else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
                int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
                insert_timer(timer, tv3.vec, i);
        } else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
                int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
                insert_timer(timer, tv4.vec, i);
        } else if ((signed long) idx < 0) {
                /* can happen if you add a timer with expires == jiffies,
                 * or you set a timer to go off in the past
                 */
                insert_timer(timer, tv1.vec, tv1.index);
        } else if (idx <= 0xffffffffUL) {
                int i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
                insert_timer(timer, tv5.vec, i);
        } else {
                /* Can only get here on architectures with 64-bit jiffies */
                timer->next = timer->prev = timer;
        }
}

static inline int detach_timer(struct timer_list *timer)
{
        struct timer_list *prev = timer->prev;
        if (prev) {
                struct timer_list *next = timer->next;
                prev->next = next;
                if (next)
                        next->prev = prev;
                return 1;
        }
        return 0;
}

void mod_timer(struct timer_list *timer, unsigned long expires)
{
        unsigned long flags;

        spin_lock_irqsave(&timerlist_lock, flags);
        timer->expires = expires;
        detach_timer(timer);
        internal_add_timer(timer);
        spin_unlock_irqrestore(&timerlist_lock, flags);
}

