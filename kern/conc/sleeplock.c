// Sleeping locks

#include "inc/types.h"
#include "inc/x86.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/environment_definitions.h"
#include "inc/assert.h"
#include "inc/string.h"
#include "sleeplock.h"
#include "channel.h"
#include "../cpu/cpu.h"
#include "../proc/user_environment.h"

void init_sleeplock(struct sleeplock *lk, char *name)
{
	init_channel(&(lk->chan), "sleep lock channel");
	init_spinlock(&(lk->lk), "lock of sleep lock");
	strcpy(lk->name, name);
	lk->locked = 0;
	lk->pid = 0;
}

int holding_sleeplock(struct sleeplock *lk)
{
	int r;
	acquire_spinlock(&(lk->lk));
	r = lk->locked && (lk->pid == get_cpu_proc()->env_id);
	release_spinlock(&(lk->lk));
	return r;
}
//==========================================================================

void acquire_sleeplock(struct sleeplock *lk)
{
	// Acquire the internal spinlock
	acquire_spinlock(&lk->lk);

	// Check if the lock is already held
	while (lk->locked) sleep(&lk->chan, &lk->lk); // Sleep on the lock's channel if it's held

	// Lock is free, so acquire it
	lk->locked = 1;
	lk->pid = get_cpu_proc()->env_id;

	// Release the internal spinlock
	release_spinlock(&lk->lk);
}

void release_sleeplock(struct sleeplock *lk)
{
	// Acquire the internal spinlock
	acquire_spinlock(&lk->lk);

	// Check if the current process holds the lock
	if (lk->locked && lk->pid == get_cpu_proc()->env_id)
	{
		lk->locked = 0; // Release the lock
		lk->pid = 0;
		wakeup_all(&lk->chan); // Wake up any waiting processes
	}

	// Release the internal spinlock
	release_spinlock(&lk->lk);
}
