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
	// cprintf("Create Semaphore\n");
	struct __semdata *semdata=smalloc(semaphoreName,sizeof(struct semaphore),1);
	semdata->count=value;
	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	sys_init_queue(&(semdata->queue));

	struct semaphore *s=smalloc(semaphoreName,sizeof(struct semaphore),1);
	s->semdata=semdata;
	return *s;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	// cprintf("Get Semaphore %s\n",semaphoreName);
	// struct __semdata *semdata=((struct __semdata *)(sget(ownerEnvID,semaphoreName)));
	// struct semaphore s;
	// s.semdata=semdata;
	// return s; 
	struct semaphore *s=((struct semaphore *)(sget(ownerEnvID,semaphoreName)));
	return *s;

}

void wait_semaphore(struct semaphore sem)
{
	// cprintf("Wait sem name: %s\n",sem.semdata->name);
	while(xchg(&(sem.semdata->lock),1)!=0);
	if(--(sem.semdata->count)<0)
	{
		struct Env *env= sys_get_cpu_proc();
		sys_enqueue(&(sem.semdata->queue),env);
		sem.semdata->lock=0;
		env->env_status=ENV_BLOCKED;
	}
	else sem.semdata->lock=0;
	// cprintf("end of wait\n");
}

void signal_semaphore(struct semaphore sem)
{
	// cprintf("signal\n");
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
