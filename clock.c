#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "time.h"

extern int memsize;

extern int debug;

extern struct frame *coremap;
int *order;
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int i;
	int j;
	int least_recent_idx = -1;//coremap[0].current_time;
	for (i = 0; i < memsize; i++){		
		if (coremap[i].ref == 1){
			//printf("set ref to 0 for frame %d\n", i);
			coremap[i].ref = 0;
		}else {
			if (i==0){
				least_recent_idx = i;
			}
			else if (coremap[i].current_time < coremap[least_recent_idx].current_time){
				least_recent_idx = i;
			}

		}
		if (i == memsize - 1 && least_recent_idx == -1){
			//printf("all frames were 1, so we check least recent for another round\n");
			for (j = 0; j < memsize; j++){
				if (j==0){
					least_recent_idx = j;
				}
				else if (coremap[j].current_time < coremap[least_recent_idx].current_time){
					least_recent_idx = j;
				}
			}
		}
	}

	//printf("clock evict frame %d\n", least_recent_idx);
	//int idx = (int)(random() % memsize);//this is from rand
	return least_recent_idx;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	//printf("clock ref\n");
	coremap[p->frame >> PAGE_SHIFT].current_time = clock();
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	//printf("clock init\n");
	order = malloc(memsize * sizeof(int));
	int i;
	for (i = 0; i < memsize; i++){
		coremap[i].ref = 0;
	}
}
