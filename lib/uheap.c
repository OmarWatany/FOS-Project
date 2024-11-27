#include <inc/lib.h>
#define PTR_FIRST 0x200 // if set then it's the first pointer
#define PTR_TAKEN 0x400 // if set then it's taken
#define IS_FIRST_PTR(PG_TABLE_ENT) (((PG_TABLE_ENT) & PTR_FIRST) == PTR_FIRST)
#define IS_TAKEN(PG_TABLE_ENT) (((PG_TABLE_ENT) & PTR_TAKEN) == PTR_TAKEN)

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	// if its less or equal to 2KB , then refer it to the block allocator
	if (size+8 <= DYN_ALLOC_MAX_BLOCK_SIZE)
		return alloc_block_FF(size);
	uint32 * ptr_page_table;
	uint32 firstPointer;
	if (sys_isUHeapPlacementStrategyFIRSTFIT())
	{
		
		uint32 noOfPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
		uint32 c = 0;
		for (uint32 va = (uint32)(myEnv->rlimit) + PAGE_SIZE; va < USER_HEAP_MAX; va += PAGE_SIZE) //searching for enough space with FF
		{
			if (sys_is_user_page_taken((myEnv->env_page_directory),va)) // if its taken or not
			{
				c = 0;	  // reset the counter
				continue; // if its taken , continue
			}
			c++; // to count the number of back-to-back free pages found
			if (c == 1)
				firstPointer = va; // save the address of the first page

			if (c == noOfPages)
				break; // if we got the number we need , no need for more search
		}
		if (c == noOfPages) // if we found the number of pages needed , call the system call
		{
			sys_allocate_user_mem(firstPointer,size);
			return (void* )firstPointer;
		}
	}

	return NULL;

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	void *va = virtual_address;
	if (va < sbrk(0) && (uint32 *)va > myEnv->da_Start)
	{
		free_block(va);
		return;
	}

	if ((uint32)va > USER_HEAP_MAX || va < (void *)(myEnv->rlimit) + PAGE_SIZE)
	{
		panic("invalid address");
	}
	uint32 noOfPages=1;
	for(uint32 iter=(uint32)va+ PAGE_SIZE;iter<USER_HEAP_MAX;iter+=PAGE_SIZE)
	{
		if (sys_is_user_page_first((myEnv->env_page_directory),iter) | !sys_is_user_page_taken((myEnv->env_page_directory),iter)) // if its taken or first
		{
			break;
		}
		noOfPages++;
	}
	sys_free_user_mem((uint32)va,noOfPages*PAGE_SIZE);
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	size = ROUNDUP(size,PAGE_SIZE) ;
	void *va = malloc(size);
	if(!va) return NULL;
	int sharedId = sys_createSharedObject(sharedVarName, size, isWritable,va);
	if(sharedId == E_SHARED_MEM_EXISTS || sharedId == E_NO_SHARE)
	{
		free(va);
		return NULL;
	}
	return va;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	int size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);
	if(size == 0) return NULL;

	void* ptr = malloc(size);
	if (ptr == NULL) return NULL;

	int ret = sys_getSharedObject(ownerEnvID, sharedVarName, ptr);
	if (ret != E_SHARED_MEM_NOT_EXISTS) return ptr;
	free(ptr);
	return NULL;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
