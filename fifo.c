#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"  // add this just for debug printing vaddr

extern unsigned memsize; // modified type of memsize from int to unsigned for debug printing vaddr

extern int debug;

extern struct frame *coremap;
struct frame *frames_head;
struct frame *frames_tail;
int num_frames;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
    int ret;
    struct frame *frame_hold;
    if(num_frames == 0){
        perror("incorrect evict no frames in memory");
        ret = -1;
    }
    // memsize == 1, don't move head to tail
	else if (num_frames == 1){
		ret = 0;
	}
    else{
		frame_hold = frames_head;
		frames_head = frames_head->next;
		frames_tail->next = frame_hold;
		frames_tail = frame_hold;
		frame_hold->next = NULL;
		ret = (frame_hold->pte->frame) >> PAGE_SHIFT;		
	}
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
    struct frame *hold_frame;
    struct frame *temp_frame;
    struct frame *cur_frame;
    int i, frame_no;
    
    //shift to correct position
    frame_no = (p->frame) >> PAGE_SHIFT;
    hold_frame = &(coremap[frame_no]);
    
    if (num_frames == 0){
		frames_head = hold_frame;
		frames_tail = hold_frame;
		num_frames++;
	}
	else if (num_frames < memsize){
		//check if hold_frame is in the list, if in, no change
		int flag = 0;
		cur_frame = frames_head;
		for (i = 0; i < num_frames; i++){
			if (cur_frame == hold_frame){
				flag = 1;
				break;
			}
			cur_frame = cur_frame->next;
		}
		// hold_frame is not in the list, add hold_frame to the tail
		if (flag == 0){
			frames_tail->next = hold_frame;
			frames_tail = hold_frame;
			hold_frame->next = NULL;
			num_frames++;
			
		}
	
	}

	if (debug){
	    printf("current queue start\n");
	    temp_frame = frames_head;
	    for (i = 0; i<num_frames; i++){
			// in order to print vaddr 
			int temp_frame_no = (temp_frame->pte->frame)>>12;
			// Calculate pointer to start of frame in (simulated) physical memory
			char *mem_ptr = &physmem[temp_frame_no*SIMPAGESIZE];
			// Calculate pointer to location in page where we keep the vaddr
			addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
	        printf("%0lx\n",*vaddr_ptr);
	        temp_frame = temp_frame->next;
	        }
	    printf("current queue end\n");
	}
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
    //initialize queue of frames
    num_frames = 0;
    frames_head = NULL;
    frames_tail = NULL;
}
