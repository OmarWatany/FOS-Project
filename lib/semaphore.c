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
	struct __semdata *semdata=smalloc(semaphoreName,sizeof(struct semaphore),1);
	semdata->count=value;

	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	sys_init_queue(&(semdata->queue));
	struct semaphore s={
		semdata
	};

	return s;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	cprintf("Get Semaphore\n");
	return *((struct semaphore *)(sget(ownerEnvID,semaphoreName))); 
}

void wait_semaphore(struct semaphore sem)
{
	cprintf("Wait sem name: %s\n",sem.semdata->name);
	while(xchg(&(sem.semdata->lock),1)!=0);
	if(--(sem.semdata->count)<0)
	{
		cprintf("inside if\n");
		struct Env *env= sys_get_cpu_proc();
		cprintf("inside if\n");
		sys_enqueue(&(sem.semdata->queue),env);
		cprintf("inside if\n");
		env->env_status=ENV_BLOCKED;
		cprintf("inside if\n");
		sem.semdata->lock=0;
		cprintf("end of if if\n");
	}
	else sem.semdata->lock=0;
	cprintf("end of wait\n");
}

void signal_semaphore(struct semaphore sem)
{
	cprintf("signal\n");
	int keys=1;
	while(xchg(&(sem.semdata->lock),1)!=0);
	if(++(sem.semdata->count)<=0)
	{
		struct Env *env= sys_dequeue(&(sem.semdata->queue));		
		sys_sched_insert_ready(env);
	}
	else sem.semdata->lock=0;
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
