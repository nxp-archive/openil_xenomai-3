/**\file
 * \brief Scheduler interface header.
 * \author Philippe Gerum
 *
 * Copyright (C) 2008 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * \ingroup sched
 */

#ifndef _COBALT_KERNEL_SCHED_H
#define _COBALT_KERNEL_SCHED_H

/*! \addtogroup sched
 *@{*/

#include <linux/percpu.h>
#include <cobalt/kernel/thread.h>
#include <cobalt/kernel/schedqueue.h>
#include <cobalt/kernel/sched-tp.h>
#include <cobalt/kernel/sched-weak.h>
#include <cobalt/kernel/sched-sporadic.h>
#include <cobalt/kernel/vfile.h>

/* Sched status flags */
#define XNRESCHED	0x10000000	/* Needs rescheduling */
#define XNINSW		0x20000000	/* In context switch */
#define XNINTCK		0x40000000	/* In master tick handler context */

/* Sched local flags */
#define XNHTICK		0x00008000	/* Host tick pending  */
#define XNINIRQ		0x00004000	/* In IRQ handling context */
#define XNHDEFER	0x00002000	/* Host tick deferred */
#define XNINLOCK	0x00001000	/* Scheduler locked */

struct xnsched_rt {
	xnsched_queue_t runnable;	/*!< Runnable thread queue. */
};

/*!
 * \brief Scheduling information structure.
 */

typedef struct xnsched {

	unsigned long status;		/*!< Scheduler specific status bitmask. */
	unsigned long lflags;		/*!< Scheduler specific local flags bitmask. */
	struct xnthread *curr;		/*!< Current thread. */
#ifdef CONFIG_SMP
	int cpu;
	cpumask_t resched;		/*!< Mask of CPUs needing rescheduling. */
#endif
	struct xnsched_rt rt;		/*!< Context of built-in real-time class. */
#ifdef CONFIG_XENO_OPT_SCHED_WEAK
	struct xnsched_weak weak;	/*!< Context of weak scheduling class. */
#endif
#ifdef CONFIG_XENO_OPT_SCHED_TP
	struct xnsched_tp tp;		/*!< Context of TP class. */
#endif
#ifdef CONFIG_XENO_OPT_SCHED_SPORADIC
	struct xnsched_sporadic pss;	/*!< Context of sporadic scheduling class. */
#endif
	volatile unsigned inesting;	/*!< Interrupt nesting level. */
	struct xntimer htimer;		/*!< Host timer. */
	struct xnthread *zombie;
	struct xnthread rootcb;		/*!< Root thread control block. */
#ifdef CONFIG_XENO_HW_UNLOCKED_SWITCH
	struct xnthread *last;
#endif
#ifdef CONFIG_XENO_HW_FPU
	struct xnthread *fpuholder;	/*!< Thread owning the current FPU context. */
#endif
#ifdef CONFIG_XENO_OPT_WATCHDOG
	struct xntimer wdtimer;		/*!< Watchdog timer object. */
	int wdcount;			/*!< Watchdog tick count. */
#endif
#ifdef CONFIG_XENO_OPT_STATS
	xnticks_t last_account_switch;	/*!< Last account switch date (ticks). */
	xnstat_exectime_t *current_account;	/*!< Currently active account */
#endif
} xnsched_t;

DECLARE_PER_CPU(struct xnsched, nksched);

union xnsched_policy_param;

struct xnsched_class {

	void (*sched_init)(struct xnsched *sched);
	void (*sched_enqueue)(struct xnthread *thread);
	void (*sched_dequeue)(struct xnthread *thread);
	void (*sched_requeue)(struct xnthread *thread);
	struct xnthread *(*sched_pick)(struct xnsched *sched);
	void (*sched_tick)(struct xnthread *thread);
	void (*sched_rotate)(struct xnsched *sched,
			     const union xnsched_policy_param *p);
	void (*sched_migrate)(struct xnthread *thread,
			      struct xnsched *sched);
	void (*sched_setparam)(struct xnthread *thread,
			       const union xnsched_policy_param *p);
	void (*sched_getparam)(struct xnthread *thread,
			       union xnsched_policy_param *p);
	void (*sched_trackprio)(struct xnthread *thread,
				const union xnsched_policy_param *p);
	int (*sched_declare)(struct xnthread *thread,
			     const union xnsched_policy_param *p);
	void (*sched_forget)(struct xnthread *thread);
#ifdef CONFIG_XENO_OPT_VFILE
	int (*sched_init_vfile)(struct xnsched_class *schedclass,
				struct xnvfile_directory *vfroot);
	void (*sched_cleanup_vfile)(struct xnsched_class *schedclass);
#endif
	int nthreads;
	struct xnsched_class *next;
	int weight;
	const char *name;
};

#define XNSCHED_CLASS_WEIGHT(n)		(n * XNSCHED_CLASS_MAX_PRIO)

/* Placeholder for current thread priority */
#define XNSCHED_RUNPRIO   0x80000000

#ifdef CONFIG_SMP
#define xnsched_cpu(__sched__)	((__sched__)->cpu)
#else /* !CONFIG_SMP */
#define xnsched_cpu(__sched__)	({ (void)__sched__; 0; })
#endif /* CONFIG_SMP */

/* Test resched flag of given sched. */
static inline int xnsched_resched_p(struct xnsched *sched)
{
	return sched->status & XNRESCHED;
}

/* Set self resched flag for the given scheduler. */
#define xnsched_set_self_resched(__sched__) do {		\
  XENO_BUGON(NUCLEUS, __sched__ != xnpod_current_sched());	\
  (__sched__)->status |= XNRESCHED;				\
} while (0)

/* Set resched flag for the given scheduler. */
#ifdef CONFIG_SMP
#define xnsched_set_resched(__sched__) do {				\
  xnsched_t *current_sched = xnpod_current_sched();			\
  if (current_sched == (__sched__))					\
      current_sched->status |= XNRESCHED;				\
  else if (!xnsched_resched_p(__sched__)) {				\
      cpu_set(xnsched_cpu(__sched__), current_sched->resched);		\
      (__sched__)->status |= XNRESCHED;					\
      current_sched->status |= XNRESCHED;				\
  }									\
} while (0)
#else /* !CONFIG_SMP */
#define xnsched_set_resched	xnsched_set_self_resched
#endif /* !CONFIG_SMP */

void xnsched_zombie_hooks(struct xnthread *thread);

void __xnsched_finalize_zombie(struct xnsched *sched);

static inline void xnsched_finalize_zombie(struct xnsched *sched)
{
	if (sched->zombie)
		__xnsched_finalize_zombie(sched);
}

#ifdef CONFIG_XENO_HW_UNLOCKED_SWITCH

struct xnsched *xnsched_finish_unlocked_switch(struct xnsched *sched);

#define xnsched_resched_after_unlocked_switch() xnpod_schedule()

static inline
int xnsched_maybe_resched_after_unlocked_switch(struct xnsched *sched)
{
	return sched->status & XNRESCHED;
}

#else /* !CONFIG_XENO_HW_UNLOCKED_SWITCH */

#ifdef CONFIG_SMP
#define xnsched_finish_unlocked_switch(__sched__)	\
	({ XENO_BUGON(NUCLEUS, !hard_irqs_disabled());	\
		xnpod_current_sched(); })
#else /* !CONFIG_SMP */
#define xnsched_finish_unlocked_switch(__sched__)	\
	({ XENO_BUGON(NUCLEUS, !hard_irqs_disabled());	\
		(__sched__); })
#endif /* !CONFIG_SMP */

#define xnsched_resched_after_unlocked_switch()		do { } while(0)

#define xnsched_maybe_resched_after_unlocked_switch(sched)	\
	({ (void)(sched); 0; })

#endif /* !CONFIG_XENO_HW_UNLOCKED_SWITCH */

#ifdef CONFIG_XENO_OPT_WATCHDOG
static inline void xnsched_reset_watchdog(struct xnsched *sched)
{
	sched->wdcount = 0;
}
#else /* !CONFIG_XENO_OPT_WATCHDOG */
static inline void xnsched_reset_watchdog(struct xnsched *sched)
{
}
#endif /* CONFIG_XENO_OPT_WATCHDOG */

#include <cobalt/kernel/sched-idle.h>
#include <cobalt/kernel/sched-rt.h>

int xnsched_init_proc(void);

void xnsched_cleanup_proc(void);

void xnsched_register_classes(void);

void xnsched_init(struct xnsched *sched, int cpu);

void xnsched_destroy(struct xnsched *sched);

struct xnthread *xnsched_pick_next(struct xnsched *sched);

void xnsched_putback(struct xnthread *thread);

int xnsched_set_policy(struct xnthread *thread,
		       struct xnsched_class *sched_class,
		       const union xnsched_policy_param *p);

void xnsched_track_policy(struct xnthread *thread,
			  struct xnthread *target);

void xnsched_migrate(struct xnthread *thread,
		     struct xnsched *sched);

void xnsched_migrate_passive(struct xnthread *thread,
			     struct xnsched *sched);

/*!
 * \fn void xnsched_rotate(struct xnsched *sched, struct xnsched_class *sched_class, const union xnsched_policy_param *sched_param)
 * \brief Rotate a scheduler runqueue.
 *
 * The specified scheduling class is requested to rotate its runqueue
 * for the given scheduler. Rotation is performed according to the
 * scheduling parameter specified by @a sched_param.
 *
 * @note The nucleus supports round-robin scheduling for the members
 * of the RT class.
 *
 * @param sched The per-CPU scheduler hosting the target scheduling
 * class.
 *
 * @param sched_class The scheduling class which should rotate its
 * runqueue.
 *
 * @param sched_param The scheduling parameter providing rotation
 * information to the specified scheduling class.
 *
 * Environments:
 *
 * This service should be called from:
 *
 * - Kernel-based task
 * - Interrupt service routine
 * - User-space task (primary mode only)
 *
 * Rescheduling: never.
 */

static inline void xnsched_rotate(struct xnsched *sched,
				  struct xnsched_class *sched_class,
				  const union xnsched_policy_param *sched_param)
{
	sched_class->sched_rotate(sched, sched_param);
}

static inline int xnsched_init_thread(struct xnthread *thread)
{
	int ret = 0;

	xnsched_idle_init_thread(thread);
	xnsched_rt_init_thread(thread);
#ifdef CONFIG_XENO_OPT_SCHED_TP
	ret = xnsched_tp_init_thread(thread);
	if (ret)
		return ret;
#endif /* CONFIG_XENO_OPT_SCHED_TP */
#ifdef CONFIG_XENO_OPT_SCHED_SPORADIC
	ret = xnsched_sporadic_init_thread(thread);
	if (ret)
		return ret;
#endif /* CONFIG_XENO_OPT_SCHED_SPORADIC */
	return ret;
}

static inline int xnsched_root_priority(struct xnsched *sched)
{
	return sched->rootcb.cprio;
}

static inline struct xnsched_class *xnsched_root_class(struct xnsched *sched)
{
	return sched->rootcb.sched_class;
}

static inline void xnsched_tick(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;
	/*
	 * A thread that undergoes round-robin scheduling only
	 * consumes its time slice when it runs within its own
	 * scheduling class, which excludes temporary PIP boosts.
	 */
	if (sched_class != &xnsched_class_idle &&
	    sched_class == thread->base_class &&
	    thread->sched->curr == thread &&
	    xnthread_test_state(thread, XNTHREAD_BLOCK_BITS|XNLOCK|XNRRB) == XNRRB)
		sched_class->sched_tick(thread);
}

#ifdef CONFIG_XENO_OPT_SCHED_CLASSES

static inline void xnsched_enqueue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		sched_class->sched_enqueue(thread);
}

static inline void xnsched_dequeue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		sched_class->sched_dequeue(thread);
}

static inline void xnsched_requeue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		sched_class->sched_requeue(thread);
}

static inline void xnsched_setparam(struct xnthread *thread,
				    const union xnsched_policy_param *p)
{
	thread->sched_class->sched_setparam(thread, p);
	thread->wprio = thread->cprio + thread->sched_class->weight;
}

static inline void xnsched_getparam(struct xnthread *thread,
				    union xnsched_policy_param *p)
{
	thread->sched_class->sched_getparam(thread, p);
}

static inline void xnsched_trackprio(struct xnthread *thread,
				     const union xnsched_policy_param *p)
{
	thread->sched_class->sched_trackprio(thread, p);
	thread->wprio = thread->cprio + thread->sched_class->weight;
}

static inline void xnsched_forget(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->base_class;

	--sched_class->nthreads;

	if (sched_class->sched_forget)
		sched_class->sched_forget(thread);
}

#else /* !CONFIG_XENO_OPT_SCHED_CLASSES */

/*
 * If only the RT and IDLE scheduling classes are compiled in, we can
 * fully inline common helpers for dealing with those.
 */

static inline void xnsched_enqueue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_enqueue(thread);
}

static inline void xnsched_dequeue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_dequeue(thread);
}

static inline void xnsched_requeue(struct xnthread *thread)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_requeue(thread);
}

static inline void xnsched_setparam(struct xnthread *thread,
				    const union xnsched_policy_param *p)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_setparam(thread, p);
	else
		__xnsched_idle_setparam(thread, p);

	thread->wprio = thread->cprio + sched_class->weight;
}

static inline void xnsched_getparam(struct xnthread *thread,
				    union xnsched_policy_param *p)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_getparam(thread, p);
	else
		__xnsched_idle_getparam(thread, p);
}

static inline void xnsched_trackprio(struct xnthread *thread,
				     const union xnsched_policy_param *p)
{
	struct xnsched_class *sched_class = thread->sched_class;

	if (sched_class != &xnsched_class_idle)
		__xnsched_rt_trackprio(thread, p);
	else
		__xnsched_idle_trackprio(thread, p);

	thread->wprio = thread->cprio + sched_class->weight;
}

static inline void xnsched_forget(struct xnthread *thread)
{
	--thread->base_class->nthreads;
	__xnsched_rt_forget(thread);
}

#endif /* !CONFIG_XENO_OPT_SCHED_CLASSES */

/*@}*/

#endif /* !_COBALT_KERNEL_SCHED_H */
