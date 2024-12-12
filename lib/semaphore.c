// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	
	struct __semdata *semdata={0};
	//todo save the semaphore in the env in some way
	semdata->count=value;
	strcpy(semdata->name,semaphoreName);
	semdata->lock=0;
	LIST_INIT(&(semdata->queue));
	struct semaphore s={
		semdata
	};
	int id=sys_createSharedObject(semaphoreName,sizeof(struct semaphore),1,&s);
	return s;
}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("get_semaphore is not implemented yet");
	//Your Code is Here...
}

void wait_semaphore(struct semaphore sem)
{
	int keyw=1;
	while(xchg(&(sem.semdata->lock),keyw)!=0);
	if(--(sem.semdata->count)<0)
	{
		// LIST_INSERT_TAIL(&(sem.semdata->queue),myEnv);
		// enqueue(&(sem.semdata->queue),myEnv);
		sem.semdata->lock=0;
		//todo block the env
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
