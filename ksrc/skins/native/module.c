/**
 * @file
 * This file is part of the Xenomai project.
 *
 * @note Copyright (C) 2004 Philippe Gerum <rpm@xenomai.org> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*!
 * \defgroup native Native Xenomai API.
 *
 * The native Xenomai programming interface available to real-time
 * applications. This API is built over the abstract RTOS core
 * implemented by the Xenomai nucleus.
 *
 */

#include <xenomai/nucleus/pod.h>
#ifdef __KERNEL__
#include <xenomai/native/syscall.h>
#endif /* __KERNEL__ */
#include <xenomai/native/task.h>
#include <xenomai/native/timer.h>
#include <xenomai/native/registry.h>
#include <xenomai/native/sem.h>
#include <xenomai/native/event.h>
#include <xenomai/native/mutex.h>
#include <xenomai/native/cond.h>
#include <xenomai/native/pipe.h>
#include <xenomai/native/queue.h>
#include <xenomai/native/heap.h>
#include <xenomai/native/alarm.h>
#include <xenomai/native/intr.h>

MODULE_DESCRIPTION("Native skin");
MODULE_AUTHOR("rpm@xenomai.org");
MODULE_LICENSE("GPL");

#if !defined(__KERNEL__) || !defined(CONFIG_XENO_OPT_PERVASIVE)
static xnpod_t __xeno_pod;
#endif /* !__KERNEL__ && CONFIG_XENO_OPT_PERVASIVE) */

static void xeno_shutdown (int xtype)

{
#ifdef CONFIG_XENO_OPT_NATIVE_INTR
    __native_intr_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_INTR */

#ifdef CONFIG_XENO_OPT_NATIVE_ALARM
    __native_alarm_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_ALARM */

#ifdef CONFIG_XENO_OPT_NATIVE_HEAP
    __native_heap_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_HEAP */

#ifdef CONFIG_XENO_OPT_NATIVE_QUEUE
    __native_queue_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_QUEUE */

#ifdef CONFIG_XENO_OPT_NATIVE_PIPE
    __native_pipe_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_PIPE */

#ifdef CONFIG_XENO_OPT_NATIVE_COND
    __native_cond_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_COND */

#ifdef CONFIG_XENO_OPT_NATIVE_MUTEX
    __native_mutex_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_MUTEX */

#ifdef CONFIG_XENO_OPT_NATIVE_EVENT
    __native_event_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_EVENT */

#ifdef CONFIG_XENO_OPT_NATIVE_SEM
    __native_sem_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_SEM */

    __native_task_pkg_cleanup();

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
    __native_syscall_cleanup();
    xncore_detach();
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */

#ifdef CONFIG_XENO_OPT_NATIVE_REGISTRY
    __native_registry_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_REGISTRY */

    xnpod_shutdown(xtype);
}

int SKIN_INIT(native)

{
    int err;

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
    /* The native skin is stacked over the shared Xenomai pod. */
    err = xncore_attach();
#else /* !(__KERNEL__ && CONFIG_XENO_OPT_PERVASIVE) */
    /* The native skin is standalone, there is no priority level to
       reserve for interrupt servers in user-space, since there is no
       user-space support in the first place. */
    err = xnpod_init(&__xeno_pod,T_LOPRIO,T_HIPRIO,0);
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */

    if (err)
	goto fail;

#ifdef CONFIG_XENO_OPT_NATIVE_REGISTRY
    err = __native_registry_pkg_init();

    if (err)
	goto fail;
#endif /* CONFIG_XENO_OPT_NATIVE_REGISTRY */

    err = __native_task_pkg_init();

    if (err)
	goto cleanup_registry;

#ifdef CONFIG_XENO_OPT_NATIVE_SEM
    err = __native_sem_pkg_init();

    if (err)
	goto cleanup_task;
#endif /* CONFIG_XENO_OPT_NATIVE_SEM */

#ifdef CONFIG_XENO_OPT_NATIVE_EVENT
    err = __native_event_pkg_init();

    if (err)
	goto cleanup_sem;
#endif /* CONFIG_XENO_OPT_NATIVE_EVENT */

#ifdef CONFIG_XENO_OPT_NATIVE_MUTEX
    err = __native_mutex_pkg_init();

    if (err)
	goto cleanup_event;
#endif /* CONFIG_XENO_OPT_NATIVE_MUTEX */

#ifdef CONFIG_XENO_OPT_NATIVE_COND
    err = __native_cond_pkg_init();

    if (err)
	goto cleanup_mutex;
#endif /* CONFIG_XENO_OPT_NATIVE_MUTEX */

#ifdef CONFIG_XENO_OPT_NATIVE_PIPE
    err = __native_pipe_pkg_init();

    if (err)
	goto cleanup_cond;
#endif /* CONFIG_XENO_OPT_NATIVE_PIPE */

#ifdef CONFIG_XENO_OPT_NATIVE_QUEUE
    err = __native_queue_pkg_init();

    if (err)
	goto cleanup_pipe;
#endif /* CONFIG_XENO_OPT_NATIVE_QUEUE */

#ifdef CONFIG_XENO_OPT_NATIVE_HEAP
    err = __native_heap_pkg_init();

    if (err)
	goto cleanup_queue;
#endif /* CONFIG_XENO_OPT_NATIVE_HEAP */

#ifdef CONFIG_XENO_OPT_NATIVE_ALARM
    err = __native_alarm_pkg_init();

    if (err)
	goto cleanup_heap;
#endif /* CONFIG_XENO_OPT_NATIVE_HEAP */

#ifdef CONFIG_XENO_OPT_NATIVE_INTR
    err = __native_intr_pkg_init();

    if (err)
	goto cleanup_alarm;
#endif /* CONFIG_XENO_OPT_NATIVE_INTR */

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
    err = __native_syscall_init();

    if (err)
	goto cleanup_intr;
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */
    
    xnprintf("starting native API services.\n");

    return 0;	/* SUCCESS. */

#if defined(__KERNEL__) && defined(CONFIG_XENO_OPT_PERVASIVE)
 cleanup_intr:
#endif /* __KERNEL__ && CONFIG_XENO_OPT_PERVASIVE */

#ifdef CONFIG_XENO_OPT_NATIVE_INTR
    __native_intr_pkg_cleanup();

 cleanup_alarm:
#endif /* CONFIG_XENO_OPT_NATIVE_INTR */

#ifdef CONFIG_XENO_OPT_NATIVE_ALARM
    __native_alarm_pkg_cleanup();

 cleanup_heap:
#endif /* CONFIG_XENO_OPT_NATIVE_ALARM */

#ifdef CONFIG_XENO_OPT_NATIVE_HEAP
    __native_heap_pkg_cleanup();

 cleanup_queue:
#endif /* CONFIG_XENO_OPT_NATIVE_HEAP */

#ifdef CONFIG_XENO_OPT_NATIVE_QUEUE
    __native_queue_pkg_cleanup();

 cleanup_pipe:
#endif /* CONFIG_XENO_OPT_NATIVE_QUEUE */

#ifdef CONFIG_XENO_OPT_NATIVE_PIPE
    __native_pipe_pkg_cleanup();

 cleanup_cond:
#endif /* CONFIG_XENO_OPT_NATIVE_PIPE */

#ifdef CONFIG_XENO_OPT_NATIVE_COND
    __native_cond_pkg_cleanup();

 cleanup_mutex:
#endif /* CONFIG_XENO_OPT_NATIVE_COND */

#ifdef CONFIG_XENO_OPT_NATIVE_MUTEX
    __native_mutex_pkg_cleanup();

 cleanup_event:
#endif /* CONFIG_XENO_OPT_NATIVE_MUTEX */

#ifdef CONFIG_XENO_OPT_NATIVE_EVENT
    __native_event_pkg_cleanup();

 cleanup_sem:
#endif /* CONFIG_XENO_OPT_NATIVE_EVENT */

#ifdef CONFIG_XENO_OPT_NATIVE_SEM
    __native_sem_pkg_cleanup();

 cleanup_task:
#endif /* CONFIG_XENO_OPT_NATIVE_SEM */

    __native_task_pkg_cleanup();

 cleanup_registry:

#ifdef CONFIG_XENO_OPT_NATIVE_REGISTRY
    __native_registry_pkg_cleanup();
#endif /* CONFIG_XENO_OPT_NATIVE_REGISTRY */

 fail:

    return err;
}

void SKIN_EXIT(native)

{
    xnprintf("stopping native API services.\n");
    xeno_shutdown(XNPOD_NORMAL_EXIT);
}

module_init(__native_skin_init);
module_exit(__native_skin_exit);
