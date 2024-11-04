#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

#define PERM_FIRST 0x600 // if set then it's the first pointer
#define IS_FIRST_PTR(PG_TABLE_ENT) (((PG_TABLE_ENT) & PERM_FIRST) == PERM_FIRST)

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	if(initSizeToAllocate>daLimit-daStart) panic("whats this?");
	da_Start = (uint32 *) daStart;
	brk = (uint32 *) ((uint32) da_Start + initSizeToAllocate);
	rlimit= (uint32 *) daLimit;
	struct FrameInfo * ptr_frame_info; 
	int ret;
	for (uint32 i = daStart; i < daStart+initSizeToAllocate; i+=PAGE_SIZE)
	{
		ret=allocate_frame(&ptr_frame_info); 
		if(ret==E_NO_MEM) panic("we need more memory!");
		ret=map_frame(ptr_page_directory,ptr_frame_info,i,PERM_USER|PERM_WRITEABLE)	;
		if(ret==E_NO_MEM) panic("we need more memory!");
	}
	initialize_dynamic_allocator(daStart,initSizeToAllocate);
	return 0;

}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */
	//if requested size is 0 , return the old break limit
	if(!numOfPages) return brk;
	//if break exceeded the hard limit , return -1
	if(brk+numOfPages*PAGE_SIZE>rlimit) return (void *)-1;
	uint32 * oldBrk=brk;
	brk+=numOfPages*PAGE_SIZE/4;
	struct FrameInfo *ptr_frame_info ; 
	int ret;
	for (uint32 i = (uint32)oldBrk ; i < (uint32)brk; i+=PAGE_SIZE)
	{
		ret=allocate_frame(&ptr_frame_info); 
		if(ret==E_NO_MEM) panic("we need more memory!");
		ret=map_frame(ptr_page_directory,ptr_frame_info,i,PERM_USER|PERM_WRITEABLE);
		if(ret==E_NO_MEM) panic("we need more memory!");
	}
	return oldBrk;	
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{
	if(!size) return NULL;
	
	//if its less or equal to 2KB , then refer it to the block allocator
	if(size<=DYN_ALLOC_MAX_BLOCK_SIZE) return alloc_block_FF(size);

	struct FrameInfo * firstPointer;
	if(isKHeapPlacementStrategyFIRSTFIT()==1)
	{
		uint32 noOfPages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
		struct FrameInfo *ptr_frame_info ; 
		uint32 c=0;
		uint32  * ptr_page_table;
		for(uint32 va=(uint32)rlimit + PAGE_SIZE; va<KERNEL_HEAP_MAX; va+=PAGE_SIZE)
		{
			if(get_frame_info(ptr_page_directory,va,&ptr_page_table)) //if its not 0 or NULL , so its allocated , then we should find somewhere else 
			{
				c=0; //reset the counter
				continue; //if the current page exists , continue
			}
			c++; // to count the number of back-to-back free pages found
			if(c==1) firstPointer=(struct FrameInfo *) va; //save the address of the first page
			if(c==noOfPages) break; // if we got the number we need , no need for more search
		}
		if(c==noOfPages) // if we found the number of pages needed , we should start allocating
		{
			// set the 9th bit to 1 if va is the first pointer
			for (uint32 va=(uint32)firstPointer; va<=(uint32)firstPointer+(noOfPages-1)*PAGE_SIZE; va+=PAGE_SIZE)
			{
				if(allocate_frame(&ptr_frame_info)==E_NO_MEM) return NULL;
				if(va == (uint32)firstPointer)
				{
					if(map_frame(ptr_page_directory,ptr_frame_info,va,PERM_WRITEABLE|PERM_FIRST)==E_NO_MEM) return NULL;
				}

				else
					if(map_frame(ptr_page_directory,ptr_frame_info,va,PERM_WRITEABLE)==E_NO_MEM) return NULL;
			}
			return firstPointer;
		}
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	void *va = virtual_address;
	if(va < sbrk(0) && (uint32 *)va > da_Start){
		free_block(va);
		return;
	} 

	if((uint32)va > KERNEL_HEAP_MAX || va < (void *)rlimit+PAGE_SIZE)
	{
		panic("Wrong address\n");
		return;
	} 

	uint32 * ptr_page_table = NULL;
	struct FrameInfo * ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)va, &ptr_page_table);

	if(!ptr_frame_info) return;
	for(uint32 iter =(uint32)va; iter < KERNEL_HEAP_MAX ; iter+= PAGE_SIZE)
	{
		ptr_frame_info = get_frame_info(ptr_page_directory, iter, &ptr_page_table);
		uint32 page_table_entry = ptr_page_table[PTX(iter)]; // get the page entry itself so I can check the permission bits

		if(iter==(uint32)va) goto HERE; //so I dont check if its the first
		if(!ptr_frame_info || IS_FIRST_PTR(page_table_entry)) return;
		HERE:
			ptr_page_table[PTX(iter)]=ptr_page_table[PTX(iter)] & (~PERM_FIRST); // el tel3ab feh lazem teraga3o makano lama t5alas
			unmap_frame(ptr_page_directory, iter); //does all the needed checks
	}
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
