// User-level Semaphore

#include "inc/lib.h"
// enqueue
// dequeue
// getCurproccess
//insert ready
//init queue
//todo
struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	cprintf("Create Semaphore\n");
	struct semaphore *s = smalloc(semaphoreName,sizeof(struct semaphore)+sizeof(struct __semdata),1);
	s->semdata = (void *)s+sizeof(struct semaphore);
	struct __semdata *semdata = s->semdata;
	cprintf("created a semaphore pointer\n");

	semdata->count=value;
	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	sys_init_queue(&(semdata->queue));

	cprintf("created a semaphore\n");
	return *s;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	struct semaphore *s=((struct semaphore *)(sget(ownerEnvID,semaphoreName)));
	return *s;
}

void wait_semaphore(struct semaphore sem)
{
	cprintf("Wait sem name: %s\n",sem.semdata->name);
	while(xchg(&(sem.semdata->lock),1)!=0);
	cprintf("passed while loop\n");
	if(--(sem.semdata->count)<0)
	{
		struct Env *env= sys_get_cpu_proc();
		sys_enqueue(&(sem.semdata->queue),env);
		cprintf("enqueue env \n");
		sem.semdata->lock=0;
		cprintf("changed lock \n");
		env->env_status = ENV_BLOCKED; 
		// i think test panics here ^ due to VIOLATION OF ACCESS RIGHT but with the master env only 
		cprintf("changed env status\n");
	}
	else sem.semdata->lock=0;
	cprintf("end of wait\n");
}

void signal_semaphore(struct semaphore sem)
{
	cprintf("signal\n");
	while(xchg(&(sem.semdata->lock),1)!=0);
	cprintf("passed while loop\n");
	if(++(sem.semdata->count)<=0)
	{
		struct Env *env= sys_dequeue(&(sem.semdata->queue));		
		sys_sched_insert_ready(env);
	}
	else sem.semdata->lock=0;
	cprintf("signal end\n");
}

int semaphore_count(struct semaphore sem)
{
	int c = sem.semdata->count;
	cprintf("count %d\n",c);
	return c;
}
