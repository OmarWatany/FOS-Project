/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct spinlock* lk)
{
	 // Get the current running process
	struct Env *current_env = get_cpu_proc();

	acquire_spinlock(&ProcessQueues.qlock);
	release_spinlock(lk);

	// Set the process state to blocked on the channel
	current_env->env_status = ENV_BLOCKED;
	current_env->channel = chan;

	// Insert the process into the blocked queue
	enqueue(&chan->queue, current_env);
	sched();
	acquire_spinlock(lk);
	release_spinlock(&ProcessQueues.qlock);
}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	acquire_spinlock(&ProcessQueues.qlock);

	if (queue_size(&chan->queue) > 0)
	{
		// Dequeue one blocked process
		struct Env *env = dequeue(&chan->queue);

		// Change its state to ready
		env->env_status = ENV_READY;

		// Insert it into the ready queue
		sched_insert_ready(env);
	}
	else cprintf("No processes to wake up\n");
	release_spinlock(&ProcessQueues.qlock);
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
	acquire_spinlock(&ProcessQueues.qlock);
	while (queue_size(&chan->queue) > 0)
	{
		// Dequeue the blocked process
		struct Env *env = dequeue(&chan->queue);

		// Change its state to ready
		env->env_status = ENV_READY;

		// Insert it into the ready queue
		sched_insert_ready(env);
	}
	release_spinlock(&ProcessQueues.qlock);
}

