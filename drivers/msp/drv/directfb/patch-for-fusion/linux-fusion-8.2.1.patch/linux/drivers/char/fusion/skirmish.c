/*
 *	Fusion Kernel Module
 *
 *	(c) Copyright 2002-2003  Convergence GmbH
 *
 *      Written by Denis Oliver Kropp <dok@directfb.org>
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_LINUX_CONFIG_H
#include <linux/config.h>
#endif
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35)
#include <linux/smp_lock.h>
#endif
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include <linux/fusion.h>

#include "fusiondev.h"
#include "fusionee.h"
#include "list.h"
#include "skirmish.h"

#define MAX_PRE_ACQUISITIONS  256

#define FUSION_SKIRMISH_LOG(x...)  do {} while (0)

typedef struct __FUSION_FusionSkirmish FusionSkirmish;

struct __FUSION_FusionSkirmish {
	FusionEntry entry;

	int lock_fid;		/* non-zero if locked */
	int lock_pid;
	int lock_count;

	int lock_total;

	unsigned int notify_count;

	unsigned long lock_time;

	FusionID transfer_to;
	FusionID transfer_from;
	int transfer_from_pid;
	int transfer_count;
	unsigned int transfer_serial;

	FusionID transfer2_to;
	FusionID transfer2_from;
	int transfer2_from_pid;
	int transfer2_count;
	unsigned int transfer2_serial;

#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	int pre_acquis[MAX_PRE_ACQUISITIONS];

	bool outer;
#endif
};

/******************************************************************************/

#ifndef PID_MAX_DEFAULT
#define PID_MAX_DEFAULT PID_MAX
#endif

static unsigned int m_pidlocks[PID_MAX_DEFAULT + 1];	/* FIXME: find cleaner, but still fast method */
static sigset_t m_sigmask;


static void
print_skirmish_internal( FusionSkirmish* skirmish, const char* pHeader )
{
	FusionEntry *entry;
	char p[16];
	struct timeval now;
	static int kaboemcount = 100;

	do_gettimeofday(&now);

	entry = &skirmish->entry;

	if (entry->last_lock.tv_sec) {
		int diff = ((now.tv_sec - entry->last_lock.tv_sec) * 1000 +
                    (now.tv_usec - entry->last_lock.tv_usec) / 1000);

		if (diff < 1000) {
			snprintf(p, 16,"%3d  ms  ", diff);
		} else if (diff < 1000000) {
			snprintf(p,16, "%3d.%d s  ", diff / 1000,
				   (diff % 1000) / 100);
		} else {
			diff = (now.tv_sec - entry->last_lock.tv_sec +
				(now.tv_usec -
				 entry->last_lock.tv_usec) / 1000000);

			snprintf(p,16, "%3d.%d h  ", diff / 3600,
				   (diff % 3600) / 360);
		}
	} else
		snprintf(p,16, "  -.-    ");

	#ifndef HI_ADVCA_FUNCTION_RELEASE
	printk( "%s %s %-3ld.%03ld %d %-18s [1] t:%ld f:%ld fpid:%-4d c:%d s:%d [2] t:%ld f:%ld fpid:%-4d c:%d s:%d - c:%d f:%d p:%-4d w:%d\n",
		   pHeader ? pHeader : " ",
		   p,
		   entry->last_lock.tv_sec,
		   (entry->last_lock.tv_usec)/1000000    ,
		   entry->id,
		   entry->name[0] ? entry->name : "???",

		   skirmish->transfer_to,
		   skirmish->transfer_from,
		   skirmish->transfer_from_pid,
		   skirmish->transfer_count,
		   skirmish->transfer_serial,

		   skirmish->transfer2_to,
		   skirmish->transfer2_from,
		   skirmish->transfer2_from_pid,
		   skirmish->transfer2_count,
		   skirmish->transfer2_serial,

		   skirmish->lock_count,
		   skirmish->lock_fid,
		   skirmish->lock_pid,
		   skirmish->entry.waiters
		   );
	#endif
	if( kaboemcount <= 0 ) {
		#ifndef HI_ADVCA_FUNCTION_RELEASE
		printk( KERN_EMERG "boem !\n" );
		#endif
		kill_pgrp(task_pgrp(current), SIGSEGV, 1);
		while(1) {
			yield();
		}
	}
}

static void
print_skirmish( FusionSkirmish* skirmish )
{
	print_skirmish_internal( skirmish, KERN_EMERG " skirmish" );
}


#ifdef FUSION_BLOCK_SIGNALS
static int skirmish_signal_handler(void *ctx)
{
	if (current->pid <= PID_MAX_DEFAULT && !m_pidlocks[current->pid]) {
		unblock_all_signals();
		return 1;
	}
	#ifndef HI_ADVCA_FUNCTION_RELEASE
	printk(KERN_EMERG "FusionSkirmish: Blocking signal for process %d!\n",
	       current->pid);
	#endif
	return 0;
}
#endif

/******************************************************************************/

static void
fusion_skirmish_print(FusionEntry * entry, void *ctx, struct seq_file *p)
{
	FusionSkirmish *skirmish = (FusionSkirmish *) entry;

#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	int i, n;

	for (i = 0, n = 0; i < MAX_PRE_ACQUISITIONS; i++) {
		if (skirmish->pre_acquis[i]) {
			n++;
		}
	}
	#ifndef HI_ADVCA_FUNCTION_RELEASE
	seq_printf(p, "[%2d]%s", n, skirmish->outer ? "." : " ");
	#endif
	for (i = 0, n = 0; i < MAX_PRE_ACQUISITIONS; i++) {
		if (skirmish->pre_acquis[i]) {
			#ifndef HI_ADVCA_FUNCTION_RELEASE
			seq_printf(p, "%s%02x", n ? "," : "",
				   skirmish->pre_acquis[i] - 1);
			#endif
			n++;
		}
	}
#endif
	#ifndef HI_ADVCA_FUNCTION_RELEASE
	seq_printf(p, "[1] t:%ld, f:%ld, fpid:%d, c:%d s:%d, [2] t:%ld, f:%ld, fpid:%d, c:%d, s:%d",
				skirmish->transfer_to,
				skirmish->transfer_from,
				skirmish->transfer_from_pid,
				skirmish->transfer_count,
				skirmish->transfer_serial,
				skirmish->transfer2_to,
				skirmish->transfer2_from,
				skirmish->transfer2_from_pid,
				skirmish->transfer2_count,
				skirmish->transfer2_serial
				);
	seq_printf(p, ", c:%d, f:0x%08x, p:%d, waiters:%d\n",
				skirmish->lock_count,
				skirmish->lock_fid,
				skirmish->lock_pid,
				skirmish->entry.waiters
				);
	#endif
}

FUSION_ENTRY_CLASS(FusionSkirmish, skirmish, NULL, NULL, fusion_skirmish_print)

/******************************************************************************/
int fusion_skirmish_init(FusionDev * dev)
{
	fusion_entries_init(&dev->skirmish, &skirmish_class, dev);

	fusion_entries_create_proc_entry(dev, "skirmishs", &dev->skirmish);

	sigemptyset(&m_sigmask);
	sigaddset(&m_sigmask, SIGSTOP);
	sigaddset(&m_sigmask, SIGTTIN);
	sigaddset(&m_sigmask, SIGTERM);
	sigaddset(&m_sigmask, SIGINT);

	return 0;
}

void fusion_skirmish_deinit(FusionDev * dev)
{
	remove_proc_entry("skirmishs", dev->proc_dir);

	fusion_entries_deinit(&dev->skirmish);
}

/******************************************************************************/

int fusion_skirmish_new(FusionDev * dev, int *ret_id)
{
	return fusion_entry_create(&dev->skirmish, ret_id, NULL);
}

int fusion_skirmish_prevail(FusionDev * dev, int id, int fusion_id)
{
	int ret;
	FusionSkirmish *skirmish;
#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	FusionSkirmish *s;
	int i;
	bool outer = true;
#endif

	dev->stat.skirmish_prevail_swoop++;

	ret = fusion_skirmish_lock(&dev->skirmish, id, true, &skirmish);
	if (ret)
		return ret;

	if (skirmish->lock_pid == current->pid) {
		skirmish->lock_count++;
		skirmish->lock_total++;
		fusion_skirmish_unlock(skirmish);
		up(&dev->skirmish.lock);
		return 0;
	}
#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	/* look in currently acquired skirmishs for this one being
	   a pre-acquisition, indicating a potential deadlock */
	fusion_list_foreach(s, dev->skirmish.list) {
		if (s->lock_pid != current->pid)
			continue;

		outer = false;

		for (i = 0; i < MAX_PRE_ACQUISITIONS; i++) {
			if (s->pre_acquis[i] == id + 1) {
				#ifndef HI_ADVCA_FUNCTION_RELEASE
				printk(KERN_DEBUG
				       "FusionSkirmish: Potential deadlock "
				       "between locked 0x%x and to be locked 0x%x in world %d!\n",
				       s->entry.id, skirmish->entry.id,
				       dev->index);
				#endif
			}
		}
	}

	if (outer)
		skirmish->outer = true;

	/* remember all previously acquired skirmishs being pre-acquisitions for
	   this one, to detect potential deadlocks due to a lock order twist */
	fusion_list_foreach(s, dev->skirmish.list) {
		int free = -1;

		if (s->lock_pid != current->pid)
			continue;

		for (i = 0; i < MAX_PRE_ACQUISITIONS; i++) {
			if (skirmish->pre_acquis[i]) {
				if (skirmish->pre_acquis[i] == s->entry.id + 1) {
					break;
				}
			} else
				free = i;
		}

		/* not found? */
		if (i == MAX_PRE_ACQUISITIONS) {
			if (free != -1) {
				skirmish->pre_acquis[free] = s->entry.id + 1;
			} else {
				#ifndef HI_ADVCA_FUNCTION_RELEASE
				printk(KERN_DEBUG
				       "FusionSkirmish: Too many pre-acquisitions to remember.\n");

				printk(KERN_DEBUG " [ '%s' ] <- ",
				       skirmish->entry.name);
				#endif
				for (i = 0; i < MAX_PRE_ACQUISITIONS; i++)
				{
					#ifndef HI_ADVCA_FUNCTION_RELEASE
					printk("0x%03x ",
					       skirmish->pre_acquis[i] - 1);
					#endif
				}
				#ifndef HI_ADVCA_FUNCTION_RELEASE
				printk("\n");
				#endif
			}
		}
	}
#endif

	up(&dev->skirmish.lock);


     while (   skirmish->lock_pid
            || (    (skirmish->transfer2_to == 0)
                 &&  skirmish->transfer_to
                 && (fusionee_dispatcher_pid(dev, skirmish-> transfer_to) != current->pid))
            || (     skirmish->transfer2_to
                 && (fusionee_dispatcher_pid(dev, skirmish-> transfer2_to) != current->pid)) )
     {
		ret = fusion_skirmish_wait(skirmish, NULL);
		if (ret)
			return ret;
	}

#ifdef FUSION_BLOCK_SIGNALS
	if (current->pid <= PID_MAX_DEFAULT && !m_pidlocks[current->pid]++)
		block_all_signals(skirmish_signal_handler, dev, &m_sigmask);
#endif

	skirmish->lock_fid   = fusion_id;
	skirmish->lock_pid   = current->pid;
	skirmish->lock_count = 1;
	skirmish->lock_time  = jiffies;

	skirmish->lock_total++;

	fusion_skirmish_unlock(skirmish);

	return 0;
}

int fusion_skirmish_swoop(FusionDev * dev, int id, int fusion_id)
{
	int ret;
	FusionSkirmish *skirmish;

	ret = fusion_skirmish_lock(&dev->skirmish, id, false, &skirmish);
	if (ret)
		return ret;

	dev->stat.skirmish_prevail_swoop++;

     if (   skirmish->lock_fid
         || (    (skirmish->transfer2_to == 0)
              &&  skirmish->transfer_to
              && (fusionee_dispatcher_pid(dev, skirmish->transfer_to) != current->pid))
         || (     skirmish->transfer2_to
              && (fusionee_dispatcher_pid(dev, skirmish-> transfer2_to) != current->pid)) )
     {
		if (skirmish->lock_pid == current->pid) {
			skirmish->lock_count++;
			skirmish->lock_total++;
			fusion_skirmish_unlock(skirmish);
			return 0;
		}

		fusion_skirmish_unlock(skirmish);

		return -EAGAIN;
	}
#ifdef FUSION_BLOCK_SIGNALS
	if (current->pid <= PID_MAX_DEFAULT && !m_pidlocks[current->pid]++)
		block_all_signals(skirmish_signal_handler, dev, &m_sigmask);
#endif

	skirmish->lock_fid   = fusion_id;
	skirmish->lock_pid   = current->pid;
	skirmish->lock_count = 1;

	skirmish->lock_total++;

	fusion_skirmish_unlock(skirmish);

	return 0;
}

int
fusion_skirmish_lock_count(FusionDev * dev, int id, int fusion_id,
			   int *ret_lock_count)
{
	int ret;
	FusionSkirmish *skirmish;

	ret = fusion_skirmish_lock(&dev->skirmish, id, false, &skirmish);
	if (ret)
		return ret;

	if (skirmish->lock_fid == fusion_id &&
	    skirmish->lock_pid == current->pid) {
		*ret_lock_count = skirmish->lock_count;
	} else {
		*ret_lock_count = 0;
	}

	fusion_skirmish_unlock(skirmish);

	return 0;
}

int fusion_skirmish_dismiss(FusionDev * dev, int id, int fusion_id)
{
	int ret;
	FusionSkirmish *skirmish;
	unsigned long lock_jiffies = 0;

	ret = fusion_skirmish_lock(&dev->skirmish, id, false, &skirmish);
	if (ret)
		return ret;

	dev->stat.skirmish_dismiss++;

	if (skirmish->lock_pid != current->pid) {
		fusion_skirmish_unlock(skirmish);
		return -EIO;
	}

	if (--skirmish->lock_count == 0) {
		skirmish->lock_fid = 0;
		skirmish->lock_pid = 0;

		lock_jiffies = jiffies - skirmish->lock_time;

		fusion_skirmish_notify(skirmish, true);

#ifdef FUSION_BLOCK_SIGNALS
		if (current->pid <= PID_MAX_DEFAULT
		    && !--m_pidlocks[current->pid])
			unblock_all_signals();
#endif
	}

	fusion_skirmish_unlock(skirmish);

#ifdef FUSION_SKIRMISH_YIELD
	/* Locked > 20 ms ? */
	if (lock_jiffies > HZ / 50)
		yield();
#endif

	return 0;
}

int fusion_skirmish_destroy(FusionDev * dev, int id)
{
	int ret;
	FusionSkirmish *skirmish;
#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	int i;
	FusionSkirmish *s;
#endif

	ret = fusion_skirmish_lock(&dev->skirmish, id, true, &skirmish);
	if (ret)
		return ret;

#ifdef FUSION_DEBUG_SKIRMISH_DEADLOCK
	/* remove from all pre-acquisition lists */
	fusion_list_foreach(s, dev->skirmish.list) {
		for (i = 0; i < MAX_PRE_ACQUISITIONS; i++) {
			if (s->pre_acquis[i] == id + 1)
				s->pre_acquis[i] = 0;
		}
	}
#endif

	up(&dev->skirmish.lock);

#ifdef FUSION_BLOCK_SIGNALS
	if (skirmish->lock_pid == current->pid &&
	    current->pid <= PID_MAX_DEFAULT && !--m_pidlocks[current->pid])
		unblock_all_signals();
#endif

	fusion_skirmish_unlock(skirmish);

	/* FIXME: gap? */

	return fusion_entry_destroy(&dev->skirmish, id);
}

int
fusion_skirmish_wait_(FusionDev * dev, FusionSkirmishWait * wait,
		      FusionID fusion_id)
{
	int ret, ret2;
	FusionSkirmish *skirmish;

	FUSION_SKIRMISH_LOG
	    ("FusionSkirmish: %s( 0x%x, lock count %u, notify count %u, timeout %u ) called...\n",
	     __FUNCTION__, wait->id, wait->lock_count, wait->notify_count,
	     wait->timeout);

	/* Lookup and lock the entry. */
	ret = fusion_skirmish_lock(&dev->skirmish, wait->id, false, &skirmish);
	if (ret) {
		FUSION_SKIRMISH_LOG
		    ("FusionSkirmish: Failed to lookup skirmish with id 0x%x!\n",
		     wait->id);
		return ret;
	}

	FUSION_SKIRMISH_LOG("FusionSkirmish: Found entry at %p!\n", skirmish);

	/* Statistics... */
	dev->stat.skirmish_wait++;

	/* Check if not a resumed call. */
	if (!wait->lock_count) {
		/* Cannot wait for skirmish not held by the current task. */
		if (skirmish->lock_pid != current->pid) {
			fusion_skirmish_unlock(skirmish);
			FUSION_SKIRMISH_LOG
			    ("FusionSkirmish: Tried to wait for skirmish not held by the current task!\n");
			return -EIO;
		}

		/* Remember lock and notification counters. */
		wait->lock_count = skirmish->lock_count;
		wait->notify_count = skirmish->notify_count;

		/* Temporarily give up the skirmish. */
		skirmish->lock_fid = 0;
		skirmish->lock_pid = 0;

#ifdef FUSION_BLOCK_SIGNALS
		if (current->pid <= PID_MAX_DEFAULT
		    && !--m_pidlocks[current->pid])
			unblock_all_signals();
#endif

		/* Notify potential notifiers waiting for the entry. */
		fusion_skirmish_notify(skirmish, true);
	}
	/* This might happen when lock count was not initialized. */
	else if (skirmish->lock_pid == current->pid) {
		fusion_skirmish_unlock(skirmish);
		FUSION_SKIRMISH_LOG
		    ("FusionSkirmish: Tried to resume wait for skirmish still held by the current task!\n");
		return -EIO;
	}

	/* Wait until the notification counter differs. */
	if (wait->timeout) {
		long timeout_jiffies = wait->timeout * HZ / 1000;

		while (wait->notify_count == skirmish->notify_count && !ret)
			ret = fusion_skirmish_wait(skirmish, &timeout_jiffies);

		wait->timeout = (timeout_jiffies * 1000 / HZ) ? : 1;
	} else {
		while (wait->notify_count == skirmish->notify_count && !ret)
			ret = fusion_skirmish_wait(skirmish, NULL);
	}

	/* Check for normal or unusual results. */
	switch (ret) {
	case 0:
		break;

	case -ETIMEDOUT:
		FUSION_SKIRMISH_LOG
		    ("FusionSkirmish: Timeout while waiting for notification!\n");

		/* Relock after timeout. */
		ret2 =
		    fusion_skirmish_lock(&dev->skirmish, wait->id, false,
					 &skirmish);
		if (ret2) {
			FUSION_SKIRMISH_LOG
			    ("FusionSkirmish: Failed to relookup skirmish with id 0x%x!\n",
			     wait->id);
			return ret2;
		}
		break;

	case -EINTR:
		/* Return immediately upon signal. */
		FUSION_SKIRMISH_LOG
		    ("FusionSkirmish: Interrupted while waiting for notification!\n");
		return ret;

	default:
		/* Return immediately upon unusual result. */
		FUSION_SKIRMISH_LOG
		    ("FusionSkirmish: Error while waiting for notification (%d)!\n",
		     ret);
		return ret;
	}

	/* Wait until the lock can be taken again. */
	while (skirmish->lock_pid) {
		ret2 = fusion_skirmish_wait(skirmish, NULL);

		/* Check for normal or unusual results. */
		switch (ret2) {
		case 0:
			break;

		case -EINTR:
			/* Return immediately upon signal. */
			FUSION_SKIRMISH_LOG
			    ("FusionSkirmish: Interrupted while waiting for relock!\n");
			return ret2;

		default:
			/* Return immediately upon unusual result. */
			FUSION_SKIRMISH_LOG
			    ("FusionSkirmish: Error while waiting for notification (%d)!\n",
			     ret2);
			return ret2;
		}
	}

#ifdef FUSION_BLOCK_SIGNALS
	if (current->pid <= PID_MAX_DEFAULT && !m_pidlocks[current->pid]++)
		block_all_signals(skirmish_signal_handler, dev, &m_sigmask);
#endif

	skirmish->lock_fid   = fusion_id;
	skirmish->lock_pid   = current->pid;
	skirmish->lock_count = wait->lock_count;

	fusion_skirmish_unlock(skirmish);

	FUSION_SKIRMISH_LOG("FusionSkirmish: ...done (%d).\n", ret);

	return ret;
}

int fusion_skirmish_notify_(FusionDev * dev, int id, FusionID fusion_id)
{
	int ret;
	FusionSkirmish *skirmish;

	ret = fusion_skirmish_lock(&dev->skirmish, id, false, &skirmish);
	if (ret)
		return ret;

	dev->stat.skirmish_notify++;

	if (skirmish->lock_pid != current->pid) {
		fusion_skirmish_unlock(skirmish);
		return -EIO;
	}

	skirmish->notify_count++;

	fusion_skirmish_notify(skirmish, true);

	fusion_skirmish_unlock(skirmish);

	return 0;
}

void fusion_skirmish_dismiss_all(FusionDev * dev, int fusion_id)
{
	FusionLink *l;

	down(&dev->skirmish.lock);

	fusion_list_foreach(l, dev->skirmish.list) {
		FusionSkirmish *skirmish = (FusionSkirmish *) l;

		down(&skirmish->entry.lock);

		if (skirmish->lock_fid == fusion_id) {
			m_pidlocks[skirmish->lock_pid] = 0;

			skirmish->lock_fid   = 0;
			skirmish->lock_pid   = 0;
			skirmish->lock_count = 0;

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		if (skirmish->transfer2_from == fusion_id) {
			skirmish->transfer2_to       = 0;
			skirmish->transfer2_from     = 0;
			skirmish->transfer2_from_pid = 0;
			skirmish->transfer2_count    = 0;

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		if (skirmish->transfer_from == fusion_id) {
			skirmish->transfer_to       = skirmish->transfer2_to;
			skirmish->transfer_from     = skirmish->transfer2_from;
			skirmish->transfer_from_pid = skirmish->transfer2_from_pid;
			skirmish->transfer_count    = skirmish->transfer2_count;

			if (skirmish->transfer2_to) {
				skirmish->transfer2_to       = 0;
				skirmish->transfer2_from     = 0;
				skirmish->transfer2_from_pid = 0;
				skirmish->transfer2_count    = 0;
			}

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		up(&skirmish->entry.lock);
	}

	up(&dev->skirmish.lock);
}

void fusion_skirmish_dismiss_all_from_pid(FusionDev * dev, int pid)
{
	FusionLink *l;

	down(&dev->skirmish.lock);

	m_pidlocks[pid] = 0;

	fusion_list_foreach(l, dev->skirmish.list) {
		FusionSkirmish *skirmish = (FusionSkirmish *) l;

		down(&skirmish->entry.lock);

		if (skirmish->lock_pid == pid) {
			skirmish->lock_fid   = 0;
			skirmish->lock_pid   = 0;
			skirmish->lock_count = 0;

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		if (skirmish->transfer2_from_pid == pid) {
			skirmish->transfer2_to       = 0;
			skirmish->transfer2_from     = 0;
			skirmish->transfer2_from_pid = 0;
			skirmish->transfer2_count    = 0;

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		if (skirmish->transfer_from_pid == pid) {
			skirmish->transfer_to       = skirmish->transfer2_to;
			skirmish->transfer_from     = skirmish->transfer2_from;
			skirmish->transfer_from_pid = skirmish->transfer2_from_pid;
			skirmish->transfer_count    = skirmish->transfer2_count;

			if (skirmish->transfer2_to) {
				skirmish->transfer2_to       = 0;
				skirmish->transfer2_from     = 0;
				skirmish->transfer2_from_pid = 0;
				skirmish->transfer2_count    = 0;
			}

			wake_up_interruptible_all(&skirmish->entry.wait);
		}

		up(&skirmish->entry.lock);
	}

	up(&dev->skirmish.lock);
}

void
fusion_skirmish_transfer_all(FusionDev * dev,
                             FusionID to, FusionID from, int from_pid, unsigned int serial)
{
	FusionLink *l;

	down(&dev->skirmish.lock);

	fusion_list_foreach(l, dev->skirmish.list) {
		FusionSkirmish *skirmish = (FusionSkirmish *) l;

		down(&skirmish->entry.lock);

		if (skirmish->lock_pid == from_pid) {
			if (skirmish->transfer_to == 0) {
				FUSION_ASSERT(skirmish->transfer_from == 0);
				FUSION_ASSERT(skirmish->transfer_from_pid == 0);
				FUSION_ASSERT(skirmish->transfer_count == 0);
				FUSION_ASSERT(skirmish->lock_count > 0);

				skirmish->transfer_to       = to;
				skirmish->transfer_from     = from;
				skirmish->transfer_from_pid = from_pid;
				skirmish->transfer_count    = skirmish->lock_count;
				skirmish->transfer_serial   = serial;

				skirmish->lock_fid   = 0;
				skirmish->lock_pid   = 0;
				skirmish->lock_count = 0;

				wake_up_interruptible_all(&skirmish->entry.wait);
			} else if (skirmish->transfer2_to == 0) {
				FUSION_ASSERT(skirmish->transfer2_from == 0);
				FUSION_ASSERT(skirmish->transfer2_from_pid == 0);
				FUSION_ASSERT(skirmish->transfer2_count == 0);
				FUSION_ASSERT(skirmish->lock_count > 0);

				skirmish->transfer2_to       = to;
				skirmish->transfer2_from     = from;
				skirmish->transfer2_from_pid = from_pid;
				skirmish->transfer2_count    = skirmish->lock_count;
				skirmish->transfer2_serial   = serial;

				skirmish->lock_fid   = 0;
				skirmish->lock_pid   = 0;
				skirmish->lock_count = 0;

				wake_up_interruptible_all(&skirmish->entry.wait);
			}
		}

		up(&skirmish->entry.lock);
	}

	up(&dev->skirmish.lock);
}

void fusion_skirmish_reclaim_all(FusionDev * dev, int from_pid)
{
	FusionLink *l;

	down(&dev->skirmish.lock);

	fusion_list_foreach(l, dev->skirmish.list) {
		FusionSkirmish *skirmish = (FusionSkirmish *) l;

		down(&skirmish->entry.lock);

		if ((skirmish->transfer2_to == 0)
			   &&  skirmish->transfer_to
			   && (skirmish->transfer_from_pid == from_pid) )
		{
			FUSION_ASSERT(skirmish->transfer_to != 0);
			FUSION_ASSERT(skirmish->transfer_from != 0);
			FUSION_ASSERT(skirmish->transfer_count > 0);
			if( skirmish->lock_pid != -1 ) {
				print_skirmish( skirmish );
			}
			FUSION_ASSERT(skirmish->lock_pid == -1);

			skirmish->lock_fid   = skirmish->transfer_from;
			skirmish->lock_pid   = skirmish->transfer_from_pid;
			skirmish->lock_count = skirmish->transfer_count;

			skirmish->transfer_to       = 0;
			skirmish->transfer_from     = 0;
			skirmish->transfer_from_pid = 0;
			skirmish->transfer_count    = 0;
		} else if (skirmish->transfer2_to
					 && (skirmish->transfer2_from_pid == from_pid) ) {
			FUSION_ASSERT(skirmish->transfer2_to != 0);
			FUSION_ASSERT(skirmish->transfer2_from != 0);
			FUSION_ASSERT(skirmish->transfer2_count > 0);
			FUSION_ASSERT(skirmish->lock_pid == -1);

			skirmish->lock_fid   = skirmish->transfer2_from;
			skirmish->lock_pid   = skirmish->transfer2_from_pid;
			skirmish->lock_count = skirmish->transfer2_count;

			skirmish->transfer2_to       = 0;
			skirmish->transfer2_from     = 0;
			skirmish->transfer2_from_pid = 0;
			skirmish->transfer2_count    = 0;
		}
		up(&skirmish->entry.lock);
	}

	up(&dev->skirmish.lock);
}

void fusion_skirmish_return_all(FusionDev * dev, int from_fusion_id, int to_pid, unsigned int serial)
{
	FusionLink *l;

	down(&dev->skirmish.lock);

	fusion_list_foreach(l, dev->skirmish.list) {
		FusionSkirmish *skirmish = (FusionSkirmish *) l;

		down(&skirmish->entry.lock);

		if (skirmish->transfer2_to == 0) {
			if (skirmish->transfer_to       == from_fusion_id &&
			    skirmish->transfer_from_pid == to_pid         &&
			    skirmish->transfer_serial   == serial            )
			{
				FUSION_ASSERT(skirmish->transfer_from != 0);
				FUSION_ASSERT(skirmish->transfer_count > 0);
				if( skirmish->lock_count != 0 ) {
					print_skirmish( skirmish );
				}
				FUSION_ASSERT(skirmish->lock_count == 0);
				FUSION_ASSERT(skirmish->lock_fid == 0);
				FUSION_ASSERT(skirmish->lock_pid == 0);

				skirmish->lock_pid = -1;
			}
		}
		else if (skirmish->transfer2_from_pid == to_pid   &&
			    skirmish->transfer2_to       == from_fusion_id &&
			    skirmish->transfer2_serial   == serial            )
		{
			FUSION_ASSERT(skirmish->transfer2_from != 0);
			FUSION_ASSERT(skirmish->transfer2_count > 0);
			FUSION_ASSERT(skirmish->lock_count == 0);
			FUSION_ASSERT(skirmish->lock_fid == 0);
			FUSION_ASSERT(skirmish->lock_pid == 0);

			skirmish->lock_pid = -1;
		}

		up(&skirmish->entry.lock);
	}

	up(&dev->skirmish.lock);
}

