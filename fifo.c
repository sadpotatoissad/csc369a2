#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;
static struct frame *frames_head;
static struct frame *frames_tail;
static int num_frames;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
    int ret;
    struct frame *frame;
    if(num_frames == 0){
        perror("incorrect evict no frames in memory");
        return -1;
    }
    //remove tail from frame list (the first frame in)
    if(num_frames == 1){
        num_frames--;
        frame_head = nullptr;
        frame_tail = nullptr;
    }
    else if (num_frames == 2){
        frame = frame_tail;
        frame_tail = frame_tail->next;
        frame_tail.next = nullptr;
        num_frames--;
    }
    else{
        frame = frame_tail;
        frame_tail = frame_tail->next;
        num_frames--;
    }
    ret = (frame->pte->frame) >> PAGE_SHIFT;
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
    struct frame *hold_frame;
    struct frame *cur_frame;
    int i, frame_location;
    frame_location = (p->frame) >> PAGE_SHIFT;
    cur_frame = &(coremap[location]);
    if(frame_head == nullptr){
        frame_head = cur_rame;
        frame_tail = cur_frame;
        frame_head->next = nullptr;
        num_frames = 1;
    }
    else if (count == memsize){
        //out of mem
        perror("out of mem");
        return;
    }
    else{
        bool in_queue;
        in_queue = false;
        //check if already in queue; if in queue, nothing to be done
        hold_frame = frames_tail;
        for (i = 0; i<num_frames; i++){
            if(hold_frame == frames_head){
                return;
            }
            hold_frame = hold_frame->next;

        }
        //not in queue, add frame to queue
        if(in_queue){
            return;
        }
        else{
        hold_frame = frame_head;
        hold_frame->next = cur_frame;
        frame_head = cur_frame;
        num_frames++;
        }
    }
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
    //initialize queue of frames
    num_frames = 0
    frame_head = nullptr;
    frame_tail = nullptr;
}
