#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "time.h"
#include "sim.h"  // add this by Bin just for printing vadrr

extern unsigned memsize; // modified type of memsize from int to unsigned by Bin just for printing vaddr

extern int debug;

extern struct frame *coremap;

int clock_hand = 0;
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int i,j;
	int frame_no = -1;
	for (i = clock_hand; i < memsize; i++){		
		if ((coremap[i].pte->frame) & CLOCK_REF){
			printf("set ref to 0 for frame %d\n", i);
			coremap[i].pte->frame &= ~CLOCK_REF;
		}else {
			frame_no = i;
			break;
		}
		if (i == memsize - 1 && frame_no == -1){
			printf("all frames were 1, so we check least recent for another round\n");
			for (j = 0; j < memsize; j++){
				if ((coremap[j].pte->frame) & CLOCK_REF){
					printf("set ref to 0 for frame %d\n", i);
					coremap[j].pte->frame &= ~CLOCK_REF;
				}else {
					frame_no = j;
					break;
				}
			}
		}
	}
	
	if (frame_no == memsize -1){
		clock_hand = 0;
	}
	else{
		clock_hand = frame_no + 1;
	}
	printf("frame_no=%d\n", frame_no);
	printf("clock_hand=%d\n", clock_hand);
	return frame_no;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	//for debug
	int i;
	printf("\n");
	for (i=0; i<memsize; i++){
		// Calculate pointer to start of frame in (simulated) physical memory
		char *mem_ptr = &physmem[i*SIMPAGESIZE];
		// Calculate pointer to location in page where we keep the vaddr
		addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
        printf("%0lx\n",*vaddr_ptr);
	}
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	return;
}
