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
	int *BegBlock= (int *) daStart;
	int *EndBlock=(int *)(daStart+initSizeOfAllocatedSpace-sizeof(int));
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
	int *headerPointer=(int *) (va) -1;
	if(isAllocated){
	*headerPointer=totalSize+1; //if it allocated , then the LSB should be 1, so we just add 1 here and subtract it when we read
	}
	else{
		*headerPointer=totalSize;
	}
	int *footerPointer=(int *)((int)headerPointer+ totalSize-sizeof(int));// because the totalSize includes the header and the footer which are both 4 bytes
	*footerPointer=*headerPointer;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
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

	if(!size) //if requested size is 0 , return NULL
	{
		return NULL;
	}
	struct BlockElement *iter;
	int freeBlockSize;
	int totalSize=size+2*sizeof(int);
	LIST_FOREACH(iter,&freeBlocksList)
	{
		freeBlockSize=*(int *)((int *) iter - 1);
				if(freeBlockSize>=totalSize)
		{
			if(freeBlockSize-totalSize<DYN_ALLOC_MIN_BLOCK_SIZE) // it will take the entire block and we will have internal fragmentation
			{
				set_block_data(iter,freeBlockSize,1);
				 // if its the first element
				if(LIST_FIRST(&freeBlocksList)==iter)
				{
					LIST_FIRST(&freeBlocksList)=iter->prev_next_info.le_next;
					if(iter->prev_next_info.le_next){
					iter->prev_next_info.le_next->prev_next_info.le_prev=NULL;
					}
				}
				//if its the last element
				else if(LIST_LAST(&freeBlocksList)==iter)
				{
					LIST_LAST(&freeBlocksList)=iter->prev_next_info.le_prev;
					iter->prev_next_info.le_prev->prev_next_info.le_next=NULL;
				}
				//if its in the middle
				else
				{
					iter->prev_next_info.le_prev->prev_next_info.le_next=iter->prev_next_info.le_next;
					iter->prev_next_info.le_next->prev_next_info.le_prev=iter->prev_next_info.le_prev;
				}
				return iter;
			}
			else // we will split it into 2 blocks
			{
				set_block_data(iter,totalSize,1); // alloc the first block that we needed
				struct BlockElement* newFreeBlock=(struct BlockElement*)((char *)iter+ totalSize);
				set_block_data((int *)((char *)iter+ totalSize),freeBlockSize-totalSize,0);
				if(LIST_FIRST(&freeBlocksList)==iter && LIST_LAST(&freeBlocksList)==iter)
				{

					LIST_INSERT_HEAD(&freeBlocksList,newFreeBlock);
					LIST_INSERT_TAIL(&freeBlocksList,newFreeBlock);
					newFreeBlock->prev_next_info.le_next=NULL;
					newFreeBlock->prev_next_info.le_prev=NULL;
				}
				else if(LIST_FIRST(&freeBlocksList)==iter)
				{
					LIST_FIRST(&freeBlocksList)=newFreeBlock;
					iter->prev_next_info.le_next->prev_next_info.le_prev=(struct BlockElement*)((int*)newFreeBlock+1);
					newFreeBlock->prev_next_info.le_next=iter->prev_next_info.le_next;
					newFreeBlock->prev_next_info.le_prev=NULL;
				}
				//if its the last element
				else if(LIST_LAST(&freeBlocksList)==iter)
				{
					LIST_LAST(&freeBlocksList)=newFreeBlock;
					iter->prev_next_info.le_prev->prev_next_info.le_next=newFreeBlock;
					newFreeBlock->prev_next_info.le_next=NULL;
					newFreeBlock->prev_next_info.le_prev=iter->prev_next_info.le_prev;
				}
				//if its in the middle
				else
				{
					iter->prev_next_info.le_prev->prev_next_info.le_next=newFreeBlock;
					iter->prev_next_info.le_next->prev_next_info.le_prev=newFreeBlock;
					newFreeBlock->prev_next_info.le_next=iter->prev_next_info.le_next;
					newFreeBlock->prev_next_info.le_prev=iter->prev_next_info.le_prev;
				}
				return iter;
			}
		}
	}
	return NULL;
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...

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

	int blockSize=*(int *)((int *) va - 1);
	set_block_data(va,blockSize,0);
	struct BlockElement * vaNew=(struct BlockElement *) va;
	if(vaNew < LIST_FIRST(&freeBlocksList)) // if the block is before the current first block
	{
		cprintf("bagarab1");
		int *nextHeader= (int *)((char *)va + blockSize - sizeof(int));
		if( *nextHeader % 2 == 0)
		{
			vaNew->prev_next_info.le_next=((struct BlockElement *) (nextHeader+ 1))->prev_next_info.le_next;
			vaNew->prev_next_info.le_next->prev_next_info.le_prev=vaNew->prev_next_info.le_prev;//not sure
			set_block_data(va,blockSize+*nextHeader,0);

		}
		else {
		vaNew->prev_next_info.le_next=LIST_FIRST(&freeBlocksList);
		LIST_FIRST(&freeBlocksList)=vaNew;
		vaNew->prev_next_info.le_next->prev_next_info.le_prev=vaNew->prev_next_info.le_prev;//not sure
		vaNew->prev_next_info.le_prev=NULL;
		}
	}
	else if(vaNew > LIST_LAST(&freeBlocksList)) // if the block is after the current last block
	{
		cprintf("bagarab2");

		int *prevFooter = (int *)((char *)va- 2*sizeof(int));
		if( *prevFooter % 2 == 0)
		{
			set_block_data(prevFooter-*prevFooter+2*sizeof(int),*prevFooter+blockSize,0);
			va=prevFooter-*prevFooter+sizeof(int); //prevHeader
			blockSize+=*prevFooter;
		}
		else {
			vaNew->prev_next_info.le_prev=LIST_LAST(&freeBlocksList)->prev_next_info.le_prev;//not sure
			LIST_LAST(&freeBlocksList)=vaNew;
			vaNew->prev_next_info.le_prev->prev_next_info.le_next=vaNew;
			vaNew->prev_next_info.le_next=NULL;
		}
	}
	else //the block is in the middle
	{
		cprintf("bagarab3\n");
		struct BlockElement *iter;
		int *prevFooter = (int *)((char *)va- sizeof(int));
		int *nextHeader= (int *)((char *)va + blockSize - sizeof(int));
		if( *prevFooter % 2 == 0 || *nextHeader % 2 == 0){
			if( *prevFooter % 2 == 0)
			{
				cprintf("bagarab4\n");
				set_block_data((int*)((char*)prevFooter-*prevFooter+sizeof(int)),*prevFooter+blockSize,0);
				va=(int*)((char*)prevFooter-*prevFooter); //prevHeader
				blockSize+=*prevFooter;
			}
			vaNew=(struct BlockElement *) va;
			if( *nextHeader % 2 == 0 && *nextHeader >0)
			{
				cprintf("bagarab5\n");
				vaNew->prev_next_info.le_next=((struct BlockElement *) (nextHeader+ 1))->prev_next_info.le_next;
				cprintf("check\n");
				set_block_data(va,blockSize+*nextHeader,0);

			}
		}
		else
		{
			cprintf("bagarab6\n");
			LIST_FOREACH(iter,&freeBlocksList)
			{
				if(vaNew > iter && vaNew < iter->prev_next_info.le_next)
				{
					vaNew->prev_next_info.le_next=iter->prev_next_info.le_next;
					vaNew->prev_next_info.le_prev=iter->prev_next_info.le_prev; //not sure

					iter->prev_next_info.le_next=vaNew;
					vaNew->prev_next_info.le_next->prev_next_info.le_prev=vaNew->prev_next_info.le_prev; //not sure
				}


			}
		}

	}
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
