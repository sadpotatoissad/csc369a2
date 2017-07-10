#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h" //add by Bin for read through trace file

extern unsigned memsize; //modified by Bin from int ot unsigned

extern int debug;

extern struct frame *coremap;

extern char *tracefile;

#define HASHSIZE 4096
int num_frames;
struct frame *frames_head;
struct frame *frames_tail;
int sequence = 0;

// use linked list to record all the occurance of sequence number of a vaddr_index in the trace file
// vaddr_index = vaddr >> PAGE_SHIFT
struct Sequence {
	int seq_no;
	struct Sequence *next;
	
};

// each unique vaddr_index has a struct DataItem to record all its occurance of sequence number in the trace file
struct DataItem {
   struct Sequence *data;   
   struct Sequence *checkpoint;
   unsigned long key; //vaddr_index
   struct DataItem *next;
};

// creat a chaining hash map with size of HASHSIZE
struct DataItem* hashArray[HASHSIZE] = {NULL}; 

int hashCode(int key) {
   return key % HASHSIZE;
}

//search the position of key in hash map
struct DataItem *search(unsigned long key) {
   //get the hash 
   int hashIndex = hashCode(key);  
	struct DataItem *cur = NULL;
   //
	if (hashArray[hashIndex] != NULL) {
		cur = hashArray[hashIndex];
		if (cur->key == key){
			return cur;
		}		
		else {
			while (cur->next != NULL){
				cur = cur->next;
				if (cur->key == key)
					return cur;
			
			}
		}
		
	}
	
   return NULL;        
}

// insert a data into hash map
void insert(unsigned long key,int seq_no) {
	struct Sequence *data = (struct Sequence*) malloc(sizeof(struct Sequence));
	data->seq_no = seq_no;
	data->next = NULL;
	
   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->data = data;  
   item->key = key;
	item->next = NULL;
	item->checkpoint = data;
	
   //get the hash 
   int hashIndex = hashCode(key);
	//
	if (hashArray[hashIndex] == NULL){
		hashArray[hashIndex] = item;
	}
	else {
		struct DataItem *cur;
		cur = hashArray[hashIndex];
		while (cur->next != NULL){
			cur = cur->next;
		}
		cur->next = item;
		
	}
}

//print all info in hash map, including vaddr_index and all its seq_no
//the position of checkpoint is marked by c after seq_no
void display() {
	struct DataItem *cur;
	struct Sequence *temp;
	int i = 0;
	
   for(i = 0; i<HASHSIZE; i++) {
	
      if(hashArray[i] != NULL){
			cur = hashArray[i];
			while (cur != NULL){
				printf(" (%0lx,%d",cur->key,cur->data->seq_no);
				if(cur->checkpoint == cur->data){
					printf("c");
				}
				temp = cur->data;
				while(temp->next != NULL){
					temp = temp->next;
					printf("->%d", temp->seq_no);
					if(cur->checkpoint == temp)
						printf("c");
				}
				printf(")");
				cur = cur->next;
			}
      }
   }
	
   printf("\n");
}

//helper function for opt_evict
struct frame *find_opt_frame(){
	int latest_seq_no = 0;
	unsigned long key;
	
	struct frame *frame_optimal;
	struct frame *frame_cur;
	frame_cur = frames_head;
	int frame_no;

	while(frame_cur != NULL){
		frame_no = frame_cur->pte->frame >> PAGE_SHIFT;
		char *mem_ptr = &physmem[frame_no*SIMPAGESIZE];
		// Calculate pointer to location in page where we keep the vaddr
		addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
		key = *vaddr_ptr >> PAGE_SHIFT;
		struct DataItem *item = search(key);

		
		while (item->checkpoint != NULL){
			//printf("seq_no=%d\n", item->checkpoint->seq_no);
			//printf("sequence=%d\n", sequence);
			if (item->checkpoint->seq_no > sequence ){
				if (item->checkpoint->seq_no > latest_seq_no){
					latest_seq_no = item->checkpoint->seq_no;
					frame_optimal = frame_cur;
				}
				break;
			}
			else if (item->checkpoint->next != NULL){
				item->checkpoint = item->checkpoint->next;
			}
			else { //item->checkpoint->next == NULL
				//printf("frame never appear again\n");
				return frame_cur;
			}
			
		}
		//printf("latest_seq_no=%d\n", latest_seq_no);
		frame_cur = frame_cur->next;
	}
	
	
	return frame_optimal;
		
}
/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
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
		frame_hold = find_opt_frame();
		ret = (frame_hold->pte->frame) >> PAGE_SHIFT;		
	}
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	int i,frame_no;
	struct frame *cur_frame;
	struct frame *hold_frame;
	struct frame *temp_frame;
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
	
	sequence ++;
	if (debug) {
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

//helper function for opt_init()
void add_seq_no(struct DataItem * item, int seq_no){
	struct Sequence *data = (struct Sequence*) malloc(sizeof(struct Sequence));
	data->seq_no = seq_no;
	data->next = NULL;
	
	item->checkpoint->next = data;
	item->checkpoint = data;
}

//helper function for opt_init()
void handle(unsigned long vaddr_index, int seq_no){
	struct DataItem *result = search(vaddr_index);
	if (result == NULL){
		insert(vaddr_index, seq_no);
	}
	else {
		add_seq_no(result, seq_no);
	}
	return;
		
}
/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	FILE *tfp = stdin;
	num_frames = 0;
	frames_head = NULL;
	frames_tail = NULL;
	int seq_no = 0; // vaddr sequence number in the trace file

	//open trace file
	if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
		
	//read through the trace file
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			if(debug)  {
				printf("%c %lx\n", type, vaddr);
			}
			
			unsigned long vaddr_index = (unsigned long)(vaddr >> PAGE_SHIFT);
			handle(vaddr_index, seq_no);
			
			seq_no ++; 
			
		} else {
			continue;
		}
	}
	
	if(debug)
		display();
	//recover checkpoint to begainning
	int i;
	struct DataItem *cur;
	for (i = 0; i < HASHSIZE; i++){
		if (hashArray[i] != NULL){
			cur = hashArray[i];
			while(cur != NULL){
				cur->checkpoint = cur->data;
				cur = cur->next;
			}
		}
	}
	if(debug)
		display();
}


