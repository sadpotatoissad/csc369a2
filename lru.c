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
    printf("check");
    struct frame *hold_frame;
    struct frame *temp_frame;
    //struct frame *temp_f;
    struct frame *cur_frame;
    int i, frame_no;

    //shift to correct position
    frame_no = (p->frame) >> PAGE_SHIFT;
    hold_frame = &(coremap[frame_no]);
    /*
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
        hold_frame = frames_tail;
        for (i = 0; i<num_frames; i++){
            if(hold_frame == cur_frame){
                printf("already in queue, matching hold_frame %i with cur_frame %i\n", ((hold_frame->pte->frame)>>12),((cur_frame->pte->frame)>>12) );
                return;
            }
            hold_frame = hold_frame->next;

        }
        hold_frame = frames_head;
        hold_frame->next = cur_frame;
        frames_head = cur_frame;
        num_frames++;
    }
    * */


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
		//check if hold_frame is in the list, if in, no change
		int flag = 0;
		cur_frame = frames_head;
		for (i = 0; i < num_frames; i++){
            if ((num_frames > 1)&&(cur_frame->next == hold_frame)){
                if((frames_head == cur_frame) && (num_frames > 2)){
                    flag = 1;
                    //when theres three or more frames and p is next to head
                    frames_head->next = frames_head->next->next;
                    frames_tail->next = cur_frame->next;
                    frames_tail = cur_frame->next;
                    frames_tail->next = NULL;
                    break;
                }else if(cur_frame->next->next == frames_tail){
                //if (num_frames = 2){
                    //when theres 2 frames and p is the first one
                   // frames_head = frames_head->next;
                   // frames_tail->next = cur_frame;
                   // frames_tail = cur_frame;
                    //frames_tail->next = NULL;
                    //when there is three frames and p is before tail
                    if(num_frames == 3){
                        flag = 1;
                        frames_head->next = frames_head->next->next;
                        frames_tail->next = cur_frame->next;
                        frames_tail = cur_frame->next;
                        frames_tail->next = NULL;
                        break;
                    }else{
                        //when there is more than three frames and p is before tail
                        flag = 1;
                        frames_tail->next = cur_frame->next;
                        frames_tail = cur_frame->next;
                        cur_frame->next = cur_frame->next->next;
                        break;

                    }


                }else if(num_frames > 4){
                    flag = 1;
                    frames_tail->next = cur_frame->next;
                    frames_tail = cur_frame->next;
                    cur_frame->next = cur_frame->next->next;
                    frames_tail->next = NULL;
                    break;


                }

            }
            else if((num_frames == 2) && (hold_frame == cur_frame)){
                //has 2 frames
                if(cur_frame == frames_head){
                    //p is head
                    flag = 1;
                    frames_head = frames_tail;
                    frames_tail->next = cur_frame;
                    frames_tail = cur_frame;
                    frames_tail->next = NULL;
                    break;
                }
                else{
                    //already at tail
                    flag = 1;
                    break;
                }
            }
            else if ((num_frames == 1)&& (hold_frame == cur_frame)){
                //p is only frame in queue
                flag = 1;
                break;
            }
            else if ((frames_head == cur_frame) && (hold_frame == cur_frame)){
                //p is head more than two frames
                flag = 1;
                frames_head = frames_head->next;
                frames_tail->next = cur_frame;
                frames_tail = cur_frame;
                frames_tail->next = NULL;
                break;
            }
            else if ((frames_tail == cur_frame) && (hold_frame == cur_frame)){
                flag = 1;
                //p is tail
                break;
            }
			cur_frame = cur_frame->next;
		}
		// hold_frame is not in the list, add hold_frame to the tail
		if (flag == 0){
			assert(num_frames < memsize);
			frames_tail->next = hold_frame;
			frames_tail = hold_frame;
			hold_frame->next = NULL;
			num_frames++;

		}

	}
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
