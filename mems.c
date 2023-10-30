/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>
#include<assert.h>
#include<string.h>
#include<math.h>
#include<limits.h>
#include<time.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/wait.h>
#include<sys/ipc.h>

/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
#define PAGE_SIZE 4096


struct main_node{
    int n;
    void* address_in_PA; // This is the pointer to the address of the space allocated to the PAGE in the main node in the physical address space or OSs heap
    struct main_node* next;  // this is the pointer to the next node in the main chain
    struct sub_node* sub_chain; // this is the pointer to the sub chain of the main chain
    struct main_node* prev; // this is the pointer to the previous node in the main chain
    // default size is PAGE_SIZE
    int no_of_holes; // this is the number of holes in the sub chain
    int no_of_process; // this is the number of process in the sub chain
    int size_of_holes; // this is the size of the holes in the sub chain
};

struct sub_node{
    int n;
    void* start_addr;  // This is the pointer to the starting address of the segment in the MeMs physical address space or OSs heap
    void* end_addr;  // This is the pointer to the ending address of the segment in the MeMs physical address space or OSs heap
    struct sub_node* next;  // this is the pointer to the next node in the sub chain
    struct sub_node* prev;  // this is the pointer to the previous node in the sub chain
    int type; // 0 for hole and 1 for process
    size_t size; // this is the size of the segment
};

struct main_node* head = NULL; // this is the pointer to the head of the main chain

/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/

void initialize_main_node(struct main_node* node){
    struct sub_node* sub_head = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    struct main_node* temp = node;
    struct main_node* temp2 = node;
    int n = 0;
    while(temp->next != NULL){
        temp = temp->next;
        n++;
    }
    node->n = n;
    node->address_in_PA = NULL;
    node->next = NULL;
    node->sub_chain = NULL;
    node->prev = NULL;
    node->no_of_holes = 1;
    node->no_of_process = 0;
}

void initialze_process_node(struct sub_node* head , struct sub_node* node, size_t size , void *start_addr){
    struct sub_node* temp = head;
    struct sub_node* temp2 = head;
    int n = 0;
    while(temp->next != NULL){
        temp = temp->next;
        n++;
    }
    node->n = n;
    node->start_addr = start_addr;
    node->end_addr = start_addr + size - 1;
    node->next = NULL;
    node->prev = NULL;
    node->type = 1;
    node->size = size;
}

void mems_init(){
    head = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    initialize_main_node(head);
    head->address_in_PA = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish(){
    struct main_node* temp = head;
    struct main_node* temp2 = head;
    while(temp->next != NULL){  // this loop takes the temp pointer to the last node of the main chain
        temp = temp->next;
    }
    while(temp->prev != NULL){  // now this loop frees the memory of the sub chain of the last node of the main chain
        temp2 = temp->prev;
        munmap(temp, PAGE_SIZE);
        temp = temp2;
    }
    munmap(temp, PAGE_SIZE);
    munmap(head, PAGE_SIZE);
}


/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 

int check_for_Space(size_t size){
    struct main_node* temp = head;
    struct sub_node* temp2 = head->sub_chain;
    while(temp->next != NULL){
        while(temp2->next != NULL){
            if(temp2->type == 0 && temp2->size >= size){
                return 1;
            }
            temp2 = temp2->next;
        }
    }
    return 0;
}
void* mems_malloc(size_t size){
    //first we will check the size of the memory that is required by the user
    if(size > PAGE_SIZE){
        // here we will require more than one page
    }
    else if (size < PAGE_SIZE){
        // here we will check if we have enough space in the free list
        if(check_for_Space(size) == 1){
            // here we will allocate the memory from the free list]
            struct main_node* temp = head;
            struct sub_node* temp2 = head->sub_chain;
            while(temp->next != NULL){
                while(temp2->next != NULL){
                    if(temp2->type == 0 && temp2->size > size){
                        struct sub_node* new_sub_node = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
                        initialze_process_node(temp->sub_chain, new_sub_node, size, temp2->start_addr + temp2->size - size - 1);
                        struct sub_node* temp3 = temp->sub_chain;
                        while(temp3->next != NULL){
                            temp3 = temp3->next;
                        }
                        temp3->next = new_sub_node;
                        new_sub_node->prev = temp3;
                        temp2->size = temp2->size - size;
                        temp2->end_addr = temp2->end_addr - size;
                        temp->no_of_process = temp->no_of_process + 1;
                        temp->no_of_holes = temp->no_of_holes + 1;
                        temp->size_of_holes = temp->size_of_holes - size;
                        return new_sub_node->start_addr;
                    }
                    if(temp2->type == 0 && temp2->size == size){
                        temp2->type = 1;
                        temp->no_of_process = temp->no_of_process + 1;
                        temp->no_of_holes = temp->no_of_holes - 1;
                        temp->size_of_holes = temp->size_of_holes - size;
                        return temp2->start_addr;
                    }
                    temp2 = temp2->next;
                }
                temp = temp->next;
            }

        }
        else{
            // here we will create a new main node and allocate the memory from the OSs heap
            struct main_node* temp = head;
            struct main_node* temp2 = head;
            while(temp->next != NULL){
                temp = temp->next;
            }
            struct main_node* new_node = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
            initialize_main_node(new_node);
            new_node->address_in_PA = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
            temp->next = new_node;
            new_node->prev = temp;
            struct sub_node* new_sub_node = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
            initialze_process_node(new_node->sub_chain, new_sub_node, size, new_node->address_in_PA + PAGE_SIZE - size - 1);
            struct sub_node* temp3 = new_node->sub_chain;
            while(temp3->next != NULL){
                temp3 = temp3->next;
            }
            temp3->next = new_sub_node;
            new_sub_node->prev = temp3;
            new_node->no_of_process = 1;
            new_node->no_of_holes = 1;
            new_node->size_of_holes = PAGE_SIZE - size;
            return new_node->address_in_PA + PAGE_SIZE - size - 1;
        }
    }

}


/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats(){
    struct main_node* temp = head;
    struct sub_node* temp2 = head->sub_chain;
    while(temp->next != NULL){
        printf("Main Node: %d\n", temp->n);
        printf("Address in PA: %p\n", temp->address_in_PA);
        printf("No of holes: %d\n", temp->no_of_holes);
        printf("No of process: %d\n", temp->no_of_process);
        printf("Size of holes: %d\n", temp->size_of_holes);
        while(temp2->next != NULL){
            printf("Sub Node: %d\n", temp2->n);
            printf("Start Address: %p\n", temp2->start_addr);
            printf("End Address: %p\n", temp2->end_addr);
            printf("Type: %d\n", temp2->type);
            printf("Size: %d\n", temp2->size);
            temp2 = temp2->next;
        }
        temp = temp->next;
    }

}


/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void*v_ptr){
    struct main_node* temp = head;
    struct sub_node* temp2 = head->sub_chain;
    while(temp->next != NULL){
        while(temp2->next != NULL){
            if(temp2->start_addr<=v_ptr && temp2->end_addr>=v_ptr){
                return temp2->start_addr;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    return NULL;
    
}


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
void mems_free(void *v_ptr){
    // this is bit tricky as we have to remove the nodes from in between also 
    struct main_node* temp = head;
    struct sub_node* temp2 = head->sub_chain;
    while(temp->next != NULL){
        while(temp2->next != NULL){
            if(temp2->start_addr<=v_ptr && temp2->end_addr>=v_ptr){
                temp2->type = 0;
                temp->no_of_process = temp->no_of_process - 1;
                temp->no_of_holes = temp->no_of_holes + 1;
                temp->size_of_holes = temp->size_of_holes + temp2->size;
                if(temp2->prev != NULL && temp2->next != NULL){
                    if(temp2->prev->type == 0 && temp2->next->type == 0){
                        temp2->prev->size = temp2->prev->size + temp2->size + temp2->next->size;
                        temp2->prev->end_addr = temp2->prev->end_addr + temp2->size + temp2->next->size;
                        temp2->prev->next = temp2->next->next;
                        temp2->next->next->prev = temp2->prev;
                        munmap(temp2, sizeof(struct sub_node));
                        munmap(temp2->next, sizeof(struct sub_node));
                    }
                    else if(temp2->prev->type == 0 && temp2->next->type == 1){
                        temp2->prev->size = temp2->prev->size + temp2->size;
                        temp2->prev->end_addr = temp2->prev->end_addr + temp2->size;
                        temp2->prev->next = temp2->next;
                        temp2->next->prev = temp2->prev;
                        munmap(temp2, sizeof(struct sub_node));
                    }
                    else if(temp2->prev->type == 1 && temp2->next->type == 0){
                        temp2->next->size = temp2->next->size + temp2->size;
                        temp2->next->start_addr = temp2->next->start_addr - temp2->size;
                        temp2->next->prev = temp2->prev;
                        temp2->prev->next = temp2->next;
                        munmap(temp2, sizeof(struct sub_node));
                    }
                    else if(temp2->prev->type == 1 && temp2->next->type == 1){
                        temp2->type = 0;
                    }
                }
                else if(temp2 == temp->sub_chain){
                    if(temp2->next->type == 0){
                        temp2->size = temp2->size + temp2->next->size;
                        temp2->end_addr = temp2->end_addr + temp2->next->size;
                        temp2->next = temp2->next->next;
                        temp2->next->prev = temp2;
                        munmap(temp2->next, sizeof(struct sub_node));
                    }
                    else{
                        temp2->type = 0;
                    }
                }
                else if(temp2->next == NULL){
                    if(temp2->prev->type == 0){
                        temp2->prev->size = temp2->prev->size + temp2->size;
                        temp2->prev->end_addr = temp2->prev->end_addr + temp2->size;
                        temp2->prev->next = NULL;
                        munmap(temp2, sizeof(struct sub_node));
                    }
                    else{
                        temp2->type = 0;
                    }
                }
                return;
            }
            temp2 = temp2->next;
        }
    temp = temp->next;
    }
}
int main(int argc, char const *argv[])
{
    mems_init();
    void* ptr = mems_malloc(100);
    printf("%p\n", ptr);
    return 0;
}
