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

/* #define PBLK(va) 					 () */
#define HDR(va) 					 ((uint32 *)((void *)(va) - sizeof(uint32)))
#define FTR(va)  					 ((uint32 *)((void *)(va) + get_block_size((va)) - 2*sizeof(uint32)))
#define PFTR(va)					 ((uint32 *)((void *)HDR((va)) - sizeof(uint32)))
#define NBLK(va) 					 ((struct BlockElement *)((void *)(va) + get_block_size((va))))
#define NHDR(va)					 (HDR(NBLK((va))))
#define VAFTR(ftr) 				 ((struct BlockElement *)((void *)(ftr) - (*(ftr) - (~(*(ftr)) & 0x1)) + 2 * sizeof(uint32)))// *ftr - 0||1
#define VAHDR(hdr) 				 ((struct BlockElement *)((void *)(hdr) + sizeof(uint32)))

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
	if(isAllocated){
		*headerPointer=totalSize+1; //if it allocated , then the LSB should be 1, so we just add 1 here and subtract it when we read
	}
	else{
		*headerPointer=totalSize;
	}
	uint32 *footerPointer= (uint32 *)((void *)headerPointer + totalSize - sizeof(int));
	*footerPointer=*headerPointer;
}



//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	if(!size) //if requested size is 0 , return NULL
	{
		return NULL;
	}

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

	struct BlockElement *iter;
	uint32 freeBlockSize;
	uint32 totalSize=size+2*sizeof(uint32);
	LIST_FOREACH(iter,&freeBlocksList)
	{
		freeBlockSize = get_block_size(iter);
		if(freeBlockSize>=totalSize)
		{
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
	}
	return NULL;
}

//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	if(!size) //if requested size is 0 , return NULL
	{
		return NULL;
	}
	struct BlockElement *iter;
	struct BlockElement *best_fit=NULL;
	uint32 freeBlockSize;
	uint32 totalSize=size+2*sizeof(uint32);
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
	if(best_fit!=NULL)
	{
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
	}
	return best_fit;
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	if(va==NULL)
	{
		return;
	}

	uint32 blockSize=get_block_size(va);
	set_block_data(va,blockSize,0); // set not allocated
	struct BlockElement * vaNew=(struct BlockElement *) va;
	if(LIST_EMPTY(&freeBlocksList))
	{
		cprintf("was empty\n");
		LIST_INSERT_HEAD(&freeBlocksList,vaNew);
		return;
	}
	bool f=0;
	struct BlockElement *iter;
	LIST_FOREACH(iter,&freeBlocksList)
	{
		if(vaNew < iter)
		{
			cprintf("in the middle\n");
			LIST_INSERT_BEFORE(&freeBlocksList,iter,vaNew);
			f=1;
			break;
		}
	}
	if(!f)
	{
		cprintf("at tail \n");
		LIST_INSERT_AFTER(&freeBlocksList,LIST_LAST(&freeBlocksList),vaNew);
	}
	cprintf("pfooter : %u , nheader: %u \n",*(PFTR(vaNew)),*(NHDR(vaNew)));
	if(*(PFTR(vaNew)) % 2 == 0 && *(PFTR(vaNew))>0)
	{
		blockSize+= *(PFTR(vaNew));
		cprintf("before: header :%u  footer : %u , size to be added :%u\n",*HDR(vaNew),*FTR(vaNew),*(PFTR(vaNew)));
		set_block_data((struct BlockElement *)((char*)(vaNew)-*(PFTR(vaNew))),blockSize,0);
		struct BlockElement * prev=LIST_PREV(vaNew);
		LIST_REMOVE(&freeBlocksList,vaNew);
		vaNew=prev;
		cprintf("coalesce with prev\n");
		cprintf("after: header :%u  footer : %u \n",*HDR(vaNew),*FTR(vaNew));
	}
	if(*(NHDR(vaNew)) % 2 == 0 && *(NHDR(vaNew))>0)
		{
			cprintf("coalesce with next\n");
			cprintf("before header :%u  footer : %u , size to be added :%u  , block size : %u\n",*HDR(vaNew),*FTR(vaNew),*(NHDR(vaNew)),blockSize);
			blockSize+= *(NHDR(vaNew));
			set_block_data(vaNew,blockSize,0);
			struct BlockElement * next=LIST_NEXT(vaNew);
			LIST_REMOVE(&freeBlocksList,next);
			cprintf("after header :%u  footer : %u \n",*HDR(vaNew),*FTR(vaNew));
		}
	cprintf("end of free\n");

}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
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
