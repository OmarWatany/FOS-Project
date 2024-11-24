#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	struct FrameInfo ** storage = kmalloc(numOfFrames * sizeof(struct FrameInfo*));
	if(!storage) return NULL;
	return memset(storage,0,numOfFrames * sizeof(struct FrameInfo*));
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	struct Share* shared = kmalloc(sizeof(struct Share));
	if(!shared) return NULL;
	*shared = (struct Share) 
	{
		.ID = ((int32)shared & ~(1 << 31)),
		.ownerID = ownerID,
		.size = size,
		.isWritable = isWritable,
		.framesStorage = create_frames_storage(ROUNDUP(size, PAGE_SIZE)/PAGE_SIZE),
		.references = 1,
	};
	strcpy(shared->name,shareName);
	return shared;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	acquire_spinlock(&AllShares.shareslock);

	struct Share *iter;
	LIST_FOREACH(iter,&AllShares.shares_list)
	{
		if(iter->ownerID == ownerID && !strcmp(name,iter->name))
		{
			release_spinlock(&AllShares.shareslock);
			return iter;
		}
	}
	release_spinlock(&AllShares.shareslock);
	return NULL;
}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	uint32 noOfPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	struct Env* myenv = get_cpu_proc(); //The calling environment
	
	if(get_share(ownerID,shareName)) return E_SHARED_MEM_EXISTS;

	struct Share * shared = create_share(ownerID, shareName, size, isWritable);
	if(!shared) return E_NO_SHARE;

	acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_TAIL(&AllShares.shares_list,shared);
	release_spinlock(&AllShares.shareslock);

	// allocate space on kernel heap
	void * sha = kmalloc(size);
	if(!sha) return E_NO_SHARE;

	// map it to virtual_address
	uint32 *ptr_page_table = NULL;
	struct FrameInfo *ptr_frame_info = NULL;

	uint32 perms = PERM_USER| PTR_TAKEN;
	perms |= isWritable ? PERM_WRITEABLE : 0;

	for (uint32 iter = (uint32)sha,i = 0; iter <= (uint32)sha + (noOfPages - 1)*PAGE_SIZE; iter += PAGE_SIZE,i++)
	{
		ptr_frame_info = get_frame_info(ptr_page_directory, iter, &ptr_page_table);
		shared->framesStorage[i] = ptr_frame_info; 
		if (iter == (uint32)sha)
		{
			if (map_frame(myenv->env_page_directory, ptr_frame_info,(uint32)virtual_address, PTR_FIRST | perms) == E_NO_MEM)
			{
				kfree(sha);
				return E_NO_SHARE;
			}
		}
		else if (map_frame(myenv->env_page_directory, ptr_frame_info,(uint32)virtual_address, perms) == E_NO_MEM)
		{
			kfree(sha);
			return E_NO_SHARE;
		}
	}
	
	return shared->ID;
}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("free_share is not implemented yet");
	//Your Code is Here...

}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}
