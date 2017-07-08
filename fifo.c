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
        frames_head = NULL;
        frames_tail = NULL;
        frames_tail->next = NULL;
    }
    else if (num_frames == 2){
        frame = frames_tail;
        frames_tail = frames_tail->next;
        frames_head = frames_tail;
        frames_tail->next = NULL;
        num_frames--;
    }
    else{
        frame = frames_tail;
        frames_tail = frames_tail->next;
        num_frames--;
    }
    //shift to provide correct frame number
    ret = (frame->pte->frame) >> PAGE_SHIFT;
    printf("evicted %i\n", ret);
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
    printf("check");
    struct frame *hold_frame;
    struct frame *cur_frame;
    int i, frame_location;
    //shift to correct position
    frame_location = (p->frame) >> PAGE_SHIFT;
    cur_frame = &(coremap[frame_location]);
    if(frames_head == NULL){
        //queue is empty
        printf("queue is empty\n");
        frames_head = cur_frame;
        frames_tail = cur_frame;
        frames_head->next = NULL;
        num_frames = 1;
        printf("queue now has: %i\n", num_frames);
    }
    else if (num_frames == memsize){
        //out of memory case
        perror("out of mem");
        return;
    }
    else{
        //check if already in queue; if in queue, return
        hold_frame = frames_tail;
        for (i = 0; i<num_frames; i++){
            if(hold_frame == cur_frame){
                return;
            }
            hold_frame = hold_frame->next;

        }
        hold_frame = frames_head;
        hold_frame->next = cur_frame;
        frames_head = cur_frame;
        num_frames++;
    }
    printf("current queue start\n");
    hold_frame = frames_tail;
    for (i = 0; i<num_frames; i++){
        printf("%i\n",((hold_frame->pte->frame)>>12));
        hold_frame = hold_frame->next;
        }
    printf("current queue end\n");
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
