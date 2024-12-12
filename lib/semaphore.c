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
	
	struct __semdata *semdata={0};
	semdata->count=value;
	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	// sys_init_queue((semdata->queue))
	struct semaphore s={
		semdata
	};
	smalloc(semaphoreName,sizeof(struct semaphore),1);
	return s;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	return *((struct semaphore *)(sget(ownerEnvID,semaphoreName))); 
}

void wait_semaphore(struct semaphore sem)
{
	int keyw=1;
	while(xchg(&(sem.semdata->lock),keyw)!=0);
	if(--(sem.semdata->count)<0)
	{
		// sys_enqueue(&(sem.semdata->queue),myEnv);
		sem.semdata->lock=0;
		// struct Env env= sys_get_cpu_proccess();
		// env->env_status=ENV_BLOCKED;
	}
	sem.semdata->lock=0;
}

void signal_semaphore(struct semaphore sem)
{
	int keys=1;
	while(xchg(&(sem.semdata->lock),keys)!=0);
	if(++(sem.semdata->count)<=0)
	{
		// struct Env *env= dequeue(&(sem.semdata->queue));		
		// sched_insert_ready(env);//not sure
	}
	sem.semdata->lock=0;
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
