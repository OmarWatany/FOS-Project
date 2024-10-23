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
#define FTR(va)  					 ((uint32 *)((void *)(va) + get_block_size((va)) - sizeof(uint32)))
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
	uint32 freeBlockSize;
	uint32 totalSize=size+2*sizeof(uint32);
	LIST_FOREACH(iter,&freeBlocksList)
	{
		freeBlockSize = get_block_size(iter);
		cprintf("freeblockSize : %u , totalSize :%u\n", freeBlockSize,totalSize);
		if(freeBlockSize>=totalSize)
		{
			if(freeBlockSize-totalSize<16) // it will take the entire block and we will have internal fragmentation
			{
				cprintf(" fragmentation \n");
				set_block_data(iter,freeBlockSize,1);
				LIST_REMOVE(&freeBlocksList,iter);
				return iter;
			}
			else // we will split it into 2 blocks
			{
				cprintf(" split into two \n");
				set_block_data(iter,totalSize,1); // alloc the first block that we needed
				struct BlockElement* newFreeBlock= (struct BlockElement*)((char *)iter + totalSize);
				set_block_data(newFreeBlock,freeBlockSize-totalSize,0);
				if(LIST_FIRST(&freeBlocksList)==iter && LIST_LAST(&freeBlocksList)==iter) // if list size = 1
				{
					cprintf(" list size == 1 \n");
					LIST_INIT(&freeBlocksList);
					LIST_INSERT_HEAD(&freeBlocksList,newFreeBlock);
				}
				else if(LIST_FIRST(&freeBlocksList)==iter)
				{
					cprintf(" the first element \n");
					LIST_REMOVE(&freeBlocksList,LIST_FIRST(&freeBlocksList));
					LIST_INSERT_HEAD(&freeBlocksList,newFreeBlock);
				}
				//if its the last element
				else if(LIST_LAST(&freeBlocksList)==iter)
				{
					cprintf(" the last element \n");
					LIST_REMOVE(&freeBlocksList,LIST_LAST(&freeBlocksList));
					LIST_INSERT_TAIL(&freeBlocksList,newFreeBlock);
				}
				//if its in the middle
				else
				{
					cprintf(" alloc in the middle \n");
					LIST_NEXT(LIST_PREV(iter)) = newFreeBlock;
					LIST_PREV(LIST_NEXT(iter)) = newFreeBlock;
					LIST_NEXT(newFreeBlock) = LIST_NEXT(iter);
					LIST_PREV(newFreeBlock) = LIST_PREV(iter);
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
	cprintf("inside free \n");
	if(va==NULL)
	{
		return;
	}

	uint32 blockSize=get_block_size(va);
	set_block_data(va,blockSize,0); // set not allocated
	struct BlockElement * vaNew=(struct BlockElement *) va;
	if(LIST_EMPTY(&freeBlocksList))
	{
		LIST_INSERT_HEAD(&freeBlocksList,vaNew);
		return;
	}
	bool f=0;
	struct BlockElement *iter;
	LIST_FOREACH(iter,&freeBlocksList)
	{
		if(vaNew < iter)
		{
			LIST_INSERT_BEFORE(&freeBlocksList,iter,vaNew);
			f=1;
			break;
		}
	}
	if(!f)
	{
		LIST_INSERT_TAIL(&freeBlocksList,vaNew);
	}
	if(*(PFTR(vaNew)) % 2 == 0 && *(PFTR(vaNew))>0)
	{
		cprintf("size before merge: %u , size after merge: %u\n",blockSize,blockSize+*(PFTR(vaNew)));
		blockSize+= *(PFTR(vaNew));
		cprintf("1\n");
		set_block_data((struct BlockElement *)((char*)(vaNew)-*(PFTR(vaNew))),blockSize,0);
		cprintf("2\n");
		LIST_REMOVE(&freeBlocksList,vaNew);
		cprintf("3\n");
		vaNew=(struct BlockElement *)((char*)(vaNew)-*(PFTR(vaNew)));
	}
	if(*(NHDR(vaNew)) % 2 == 0 && *(NHDR(vaNew))>0)
		{
			cprintf("4\n");
			blockSize+= *(NHDR(vaNew));
			cprintf("5\n");
			set_block_data(vaNew,blockSize,0);
			cprintf("6\n");
//			LIST_REMOVE(&freeBlocksList,(struct BlockElement *)((char*)(vaNew)+*(NHDR(vaNew))));
//			if(LIST_NEXT(vaNew)){
//				LIST_REMOVE(&freeBlocksList,LIST_NEXT(vaNew));
//cprintf("ff");
//			}
			if(LIST_NEXT(LIST_NEXT(vaNew)))
			{
				LIST_NEXT(vaNew)=LIST_NEXT(LIST_NEXT(vaNew));
			}
			else
			{
				LIST_NEXT(vaNew)=0;
			}
			cprintf("7\n");
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
