#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"  //for testing


extern unsigned memsize;//change back to int after testing

extern int debug;

extern struct frame *coremap;
static struct frame *frames_head;
static struct frame *frames_tail;
static int num_frames;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
    int ret;
    struct frame *frame_hold;
    if(num_frames == 0){
        perror("incorrect evict no frames in memory");
        return -1;
    }
    //remove tail from frame list (the first frame in)
    if((memsize == 1)&&(num_frames == 1)){
        num_frames--;
        frame_hold = frames_head;
        frames_head = NULL;
        frames_tail = NULL;
        return ((frame_hold->pte->frame) >> PAGE_SHIFT);
    }
    frame_hold = frames_head;
    frames_head = frames_head->next;
    frames_tail->next = frame_hold;
    frames_tail = frame_hold;
    num_frames--;
    frame_hold->next = NULL;
    ret = ((frame_hold->pte->frame) >> PAGE_SHIFT);
    printf("evicted %i (evict function)\n", ret);
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
    struct frame *hold_frame;
    struct frame *temp_frame; //just used for debug
    struct frame *cur_frame;
    struct frame *prev_frame;
    int i, frame_no, flag;
    flag = 0;

    //shift to correct position
    frame_no = (p->frame) >> PAGE_SHIFT;
    hold_frame = &(coremap[frame_no]);

    if (num_frames == 0){
        //first frame to be added
		frames_head = hold_frame;
		frames_tail = hold_frame;
        hold_frame->next = NULL;
		frames_head->next = NULL;
		frames_tail->next = NULL;
		num_frames++;
	}
	else {
		//check if hold_frame is in the list

		cur_frame = frames_head->next;
		prev_frame = frames_head;
		//check if frame p is already in queue
		for (i = 0; i < (num_frames - 1); i++){
            //if p is the head of queue
            if((i == 0) && (prev_frame == hold_frame) && (prev_frame == frames_head)){
                if(num_frames == 1){
                    flag = 1;
                    break;
                }
                //p is head, move p to tail
                else{
                    frames_head = frames_head->next;
                    frames_tail->next = hold_frame;
                    frames_tail = hold_frame;
                    frames_tail->next = NULL;
                    flag = 1;
                    break;
                }
            }
            //move frame to tail
            else if (cur_frame == hold_frame){
                if (cur_frame != frames_tail){
                    prev_frame->next = cur_frame->next;
                    frames_tail->next = hold_frame;
                    frames_tail = hold_frame;
                    frames_tail->next = NULL;
                    flag = 1;
                    break;
                }
                else{
                flag = 1;
                break;
                }
            }
            prev_frame = cur_frame;
            cur_frame = cur_frame->next;
            }

		// hold_frame is not in the queue, add hold_frame to the tail
		if (flag == 0){
			assert(num_frames < memsize);
			frames_tail->next = hold_frame;
			frames_tail = hold_frame;
			hold_frame->next = NULL;
			num_frames++;

		}
	}
    if(debug){
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
void lru_init() {
    num_frames = 0;
    frames_head = NULL;
    frames_tail = NULL;
}
