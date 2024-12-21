// User-level Semaphore

#include "inc/lib.h"
struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
#if 1
	struct __semdata *semdata = smalloc(semaphoreName,sizeof(struct __semdata),1) ;
	sys_init_queue(&(semdata->queue));
	strcpy(semdata->name,semaphoreName);
	semdata->count=value;
	semdata->lock=0;
	return (struct semaphore) {
		.semdata = semdata,
	};
	
#else
	/* struct semaphore *s = smalloc(semaphoreName,sizeof(struct semaphore)+sizeof(struct __semdata),1); */
	/* s->semdata = (void *)s+sizeof(struct semaphore); */
	/* struct __semdata *semdata = s->semdata; */

	/* semdata->count=value; */
	/* strcpy(semdata->name,semaphoreName); */
	/* semdata->lock=0; */
	/* sys_init_queue(&(semdata->queue)); */
	/* return *s; */
#endif
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
#if 1
	return (struct semaphore){
		.semdata = sget(ownerEnvID,semaphoreName),
	};
#else
	struct semaphore *s=((struct semaphore *)(sget(ownerEnvID,semaphoreName)));
	return *s;
#endif
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
