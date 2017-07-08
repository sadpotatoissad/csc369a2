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

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
    int ret;
    struct frame *frame;
    if(num_frames == 0){
        perror("incorrect evict no frames in memory");
        return -1;
    }
    //remove tail from frame list (the first frame in)
    frame_hold = frames_head;
    frames_head = frames_head->next;
    frames_tail->next = frame_hold;
    frames_tail = frame_hold;
    frame_hold->next = NULL;
    ret = (frame_hold->pte->frame) >> PAGE_SHIFT;
    printf("evicted %i\n", ret);
	return ret;

}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
    printf("check");
    struct frame *hold_frame;
    struct frame *temp_frame;
    struct frame *temp_frame2;
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
    else if (!(num_frames < memsize)){
        //out of memory case
        perror("out of mem");
        return;
    }
    else{
        //check if already in queue; if in queue, return
        temp_frame2 = frames_tail;
        hold_frame = frames_tail;
        for (i = 0; i<num_frames; i++){
            if((hold_frame == cur_frame) && (i = 0)){
                //already in queue as the tail, move to head
                frames_tail = frames_tail->next->next;
                temp_frame2->next = NULL;
                frames_head->next = temp_frame2;
                frames_head = temp_frame2;
                return;
            }else if((hold_frame == cur_frame) && (i = (num_frames - 1))){
                //already in queue as head, nothing to do
                return;
            }else if((hold_frame->next) == cur_frame){
                //already in queue, move frame to head
                temp_frame2 = hold_frame->next;
                hold_frame->next = (hold_frame->next->next);
                frames_head->next = temp_frame2;
                frames_head = temp_frame2;
                temp_frame2->next = NULL;
            }
            hold_frame = hold_frame->next;

        }
        hold_frame = frames_head;
        hold_frame->next = cur_frame;
        frames_head = cur_frame;
        num_frames++;
    }
    printf("current queue start\n");
    temp_frame = frames_tail;
    for (i = 0; i<num_frames; i++){
        printf("%i\n",((temp_frame->pte->frame)>>12));
        temp_frame = temp_frame->next;
        }
    printf("current queue end\n");
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
