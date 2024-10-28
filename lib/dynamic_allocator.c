/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

#define METADATA_SIZE (2 * sizeof(uint32))
#define HDR(va)  ((uint32 *)(va) - 1)
#define FTR(va)  ((uint32 *)((void *)(va) + get_block_size((void*)(va)) - METADATA_SIZE))
#define NBLK(va) ((struct BlockElement *)((void *)(va) + get_block_size(va)))
#define NHDR(va) ((uint32 *)NBLK(va) - 1)
#define PFTR(va) ((uint32 *)(va) - 2)
#define VAFTR(ftr) ((void *)(ftr) - (*(ftr) - (*(ftr) % 2)) + METADATA_SIZE) 

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	LIST_INIT(&freeBlocksList);
	uint32 *BegBlock= (uint32 *) daStart;
	uint32 *EndBlock=(uint32 *)(daStart+initSizeOfAllocatedSpace-sizeof(int));
	*BegBlock=*EndBlock=1;

	set_block_data(BegBlock+2,initSizeOfAllocatedSpace-2*sizeof(int),0);

	struct BlockElement * firstBlock=(struct BlockElement *) (daStart+2*sizeof(int));
	LIST_INSERT_HEAD(&freeBlocksList,firstBlock);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	uint32 *headerPointer= HDR(va);
	*headerPointer = totalSize + isAllocated;
	uint32 *footerPointer= FTR(va);
	*footerPointer=*headerPointer;
}



//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//if requested size is 0 , return NULL
	if(!size) return NULL;

	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================
	if(!size) return NULL; //if requested size is 0 , return NULL
	uint32 freeBlockSize;
	uint32 totalSize=size+METADATA_SIZE;
	struct BlockElement *iter;
	LIST_FOREACH(iter,&freeBlocksList)
	{
		if((freeBlockSize = get_block_size(iter)) < totalSize)
			continue;

		if(freeBlockSize-totalSize<16) // it will take the entire block and we will have internal fragmentation
		{
			set_block_data(iter,freeBlockSize,1);
			LIST_REMOVE(&freeBlocksList,iter);
		}
		else // we will split it into 2 blocks
		{
			set_block_data(iter,totalSize,1); // alloc the first block that we needed
			struct BlockElement* newFreeBlock= (struct BlockElement*)((char *)iter + totalSize);
			set_block_data(newFreeBlock,freeBlockSize-totalSize,0);
			LIST_INSERT_AFTER(&freeBlocksList,iter,newFreeBlock);
			LIST_REMOVE(&freeBlocksList,iter);
		}
		return iter;
	}
	// if freeBlocksList is empty
	// uint32 required_size = size + 2*sizeof(int);
	// struct BlockElement * newBlock=(struct BlockElement *)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
	// LIST_INSERT_TAIL(&freeBlocksList,newBlock);
	// // try to allocate again after we made the space
	// return alloc_block_FF(size);
	return NULL;
}

//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//if requested size is 0 , return NULL
	if(!size) return NULL;
	struct BlockElement *iter;
	struct BlockElement *best_fit=NULL;
	uint32 freeBlockSize;
	uint32 totalSize=size+METADATA_SIZE;
	uint32 min_fragmentation=0xFFFFFFFF;
	LIST_FOREACH(iter,&freeBlocksList)
	{
		freeBlockSize = get_block_size(iter);
		if(freeBlockSize>=totalSize && freeBlockSize-totalSize<min_fragmentation)
		{
			min_fragmentation=freeBlockSize-totalSize;
			best_fit=iter;
		}
	}
	if(!best_fit)return NULL;

	freeBlockSize = get_block_size(best_fit);
	if(freeBlockSize-totalSize<16) // it will take the entire block and we will have internal fragmentation
	{
		set_block_data(best_fit,freeBlockSize,1);
		LIST_REMOVE(&freeBlocksList,best_fit);
	}
	else // we will split it into 2 blocks
	{
		set_block_data(best_fit,totalSize,1); // alloc the first block that we needed
		struct BlockElement* newFreeBlock= (struct BlockElement*)((char *)best_fit+ totalSize);
		set_block_data(newFreeBlock,freeBlockSize-totalSize,0);
		LIST_INSERT_AFTER(&freeBlocksList,best_fit,newFreeBlock);
		LIST_REMOVE(&freeBlocksList,best_fit);
	}
	return best_fit;
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	if(!va) return;

	uint32 blockSize=get_block_size(va);
	set_block_data(va,blockSize,0); // set not allocated
	struct BlockElement * vaNew=(struct BlockElement *) va;
	if(LIST_EMPTY(&freeBlocksList))
	{
		LIST_INSERT_HEAD(&freeBlocksList,vaNew);
		return;
	}

	if(vaNew > LIST_LAST(&freeBlocksList)) LIST_INSERT_TAIL(&freeBlocksList,vaNew);
	struct BlockElement *iter;
	LIST_FOREACH(iter,&freeBlocksList)
	{
		if(vaNew < iter)
		{
			LIST_INSERT_BEFORE(&freeBlocksList,iter,vaNew);
			break;
		}
	}
	struct BlockElement * temp= NBLK(vaNew);
	if (is_free_block(temp) && get_block_size(temp))
	{
		blockSize += get_block_size(temp);
		set_block_data(vaNew,blockSize,0);
		temp = LIST_NEXT(vaNew);
		if(temp) LIST_REMOVE(&freeBlocksList,temp);
	}
	if(*PFTR(vaNew) %2 == 0 && *PFTR(vaNew))
	{
		temp = (struct BlockElement *)VAFTR(PFTR(vaNew));
		blockSize+= get_block_size(temp);
		set_block_data(temp,blockSize,0);
		LIST_REMOVE(&freeBlocksList,vaNew);
	}
}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	if (new_size % 2 != 0) new_size++;	//ensure that the size is even (to use LSB as allocation flag)
	if (!is_initialized)
	{
		uint32 required_size = new_size + METADATA_SIZE /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
		uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}
	if (!va) return alloc_block_FF(new_size);
	if (!new_size) 
	{
		free_block(va);
		return NULL;
	}

	uint32 oldSz = get_block_size(va), totalSize = new_size+METADATA_SIZE;
	if (totalSize < 16) totalSize = 16;
	if (totalSize == oldSz) return va; // Don't know if he wants to free it or not.

	void *result = NULL;
	if( totalSize > oldSz )
	{
		struct BlockElement * next = NBLK(va);
		uint32 nextSize = 0;
		if( next ) nextSize = get_block_size(next);
		if( nextSize == 0 || !is_free_block(next) ||  nextSize < totalSize - oldSz )
		{
			if (!LIST_SIZE(&freeBlocksList)) return NULL; //if there is no space , later we should sbrk
			result = memcpy(alloc_block_FF(new_size),va,oldSz - METADATA_SIZE);
			free_block(va);
			return result;
		}
		else if (is_free_block(next))
		{
			uint32 newFreeBlockSize = nextSize + oldSz - totalSize ;
			// In place if the size increased and there is space after it
			if(newFreeBlockSize < 16)
			{
				set_block_data(va,oldSz+nextSize,1);
				LIST_REMOVE(&freeBlocksList,next);
			}
			else
			{
				set_block_data(va,totalSize,1);
				struct BlockElement * newBlock = (struct BlockElement *)(va + totalSize);
				struct BlockElement * freePrev = LIST_PREV(next);
				LIST_REMOVE(&freeBlocksList,next);
				set_block_data(newBlock, newFreeBlockSize, 0);
				LIST_INSERT_AFTER(&freeBlocksList,freePrev,newBlock);
			}
			return va;
		}
	}

	// In place if the size is decreased
	struct BlockElement * next = NBLK(va);
	uint32 newFreeBlockSize = oldSz - totalSize ;
	struct BlockElement * newBlock = va + totalSize;
	if(next && get_block_size(next) && is_free_block(next))
	{
		newFreeBlockSize += get_block_size(next);
		set_block_data(newBlock, newFreeBlockSize, 0);
		LIST_INSERT_AFTER(&freeBlocksList,LIST_PREV(next),newBlock);
		LIST_REMOVE(&freeBlocksList,next);
	}
	else
	{
		if(newFreeBlockSize < 16) goto RETURN_VA;
		set_block_data(newBlock, newFreeBlockSize, 0);
		free_block(newBlock);
	}
	set_block_data(va, totalSize, 1);
	RETURN_VA:
		return va;
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
