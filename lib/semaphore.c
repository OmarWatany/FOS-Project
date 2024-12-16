// User-level Semaphore

#include "inc/lib.h"
struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	struct semaphore *s = smalloc(semaphoreName,sizeof(struct semaphore)+sizeof(struct __semdata),1);
	s->semdata = (void *)s+sizeof(struct semaphore);
	struct __semdata *semdata = s->semdata;

	semdata->count=value;
	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	sys_init_queue(&(semdata->queue));
	return *s;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	struct semaphore *s=((struct semaphore *)(sget(ownerEnvID,semaphoreName)));
	return *s;
}

void wait_semaphore(struct semaphore sem)
{
	while(xchg(&(sem.semdata->lock),1)!=0);
	if(--(sem.semdata->count)<0)
	{
		struct Env *env= sys_get_cpu_proc();
		sem.semdata->lock=0;
		sys_enqueue(&(sem.semdata->queue),env);
	}
	sem.semdata->lock=0;
}

void signal_semaphore(struct semaphore sem)
{
	while(xchg(&(sem.semdata->lock),1)!=0);
	if(++(sem.semdata->count)<=0)
	{
		struct Env *env= sys_dequeue(&(sem.semdata->queue));		
		sys_sched_insert_ready(env);
	}
	sem.semdata->lock=0;
}

int semaphore_count(struct semaphore sem)
{
	int c = sem.semdata->count;
	cprintf("count %d\n",c);
	return c;
}
