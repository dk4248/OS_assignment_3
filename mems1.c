#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

#define PAGE_SIZE 4096

#ifdef MAP_ANONYMOUS
#define MY_MAP_ANONYMOUS MAP_ANONYMOUS
#else
#define MY_MAP_ANONYMOUS MAP_ANON
#endif

struct main_node{
    int n;  // this is the index of the Main node
    // size is default PAGE_SIZE
    struct main_node *next;  // this is the pointer to the next main node
    struct sub_node *sub_head;  // this is the pointer to the sub chains head
    struct main_node *prev;  // this is the pointer to the previous main node
    void* address_in_PA;  // this is the address of the main node in the physical address
    int no_of_process;  // this is the number of process in the sub chain
    int no_of_holes;  // this is the number of holes in the sub chain
    size_t holes_size;  // this is the total size of the holes in the sub chain
};

struct sub_node{
    int n;  // this is the index of the sub node
    size_t size;  // this is the size of the sub node
    struct sub_node *next;  // this is the pointer to the next sub node
    struct sub_node *prev;  // this is the pointer to the previous sub node
    void* starting_address;  // this is the starting address of the sub node
    void* ending_address;  // this is the ending address of the sub node
    int type;  // 0 for HOLE and 1 for PROCESS
};

// A global variable for the main Head of the main_node chain
struct main_node *main_head = NULL;

void initializing_main_node(struct main_node *node){
    int n = 0;
    struct main_node *temp = main_head;
    while(temp->next != NULL){
        temp = temp->next;
        n++;
    }
    node->n = n;
    node->next = NULL;
    node->prev = temp;
    struct sub_node *sub_head = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE,  MAP_PRIVATE, -1, 0);
    node->sub_head = sub_head;
    node->no_of_process = 0;
    node->no_of_holes = 1;
    node->holes_size = PAGE_SIZE;
    node->address_in_PA = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    sub_head->n = 0;
    sub_head->size = PAGE_SIZE;
    sub_head->next = NULL;
    sub_head->prev = NULL;
    sub_head->starting_address = node->address_in_PA;
    sub_head->ending_address = (void*)((char*)node->address_in_PA + PAGE_SIZE - 1);
    sub_head->type = 0;
}

void mems_init(){
    // initializing the main_head
    main_head = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    initializing_main_node(main_head);
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
// */
// void mems_finish(){
//     struct main_node *temp = main_head;
//     while(temp != NULL){
//         temp = temp->next;
//     }
//     while(temp != NULL){
//         struct main_node *temp2 = temp;
//         temp = temp->prev;
//         munmap(temp2->address_in_PA, PAGE_SIZE);
//     }
//     // now we will free all main nodes and sub nodes
//     temp = main_head;
//     while(temp != NULL){
//         struct main_node *temp2 = temp;
//         temp = temp->next;
//         munmap(temp2, sizeof(main_node));
//     }
// }


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
// void* mems_malloc(size_t size){
//     // if the size is greater than the PAGE_SIZE then we will look out for space in the main chain and sub chain in total 

// }


/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
// void mems_print_stats(){

// }


/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
// */
// void *mems_get(void*v_ptr){
    
// }


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
// void mems_free(void *v_ptr){
    
// }

void print_system(){
    struct main_node *temp = main_head;
    while(temp != NULL){
        printf("Main Node %d\n", temp->n);
        printf("Address in PA: %p\n", (void*)temp->address_in_PA);
        printf("No of process: %d\n", temp->no_of_process);
        printf("No of holes: %d\n", temp->no_of_holes);
        printf("Holes size: %ld\n", temp->holes_size);
        struct sub_node *temp2 = temp->sub_head;
        while(temp2 != NULL){
            printf("Sub Node %d\n", temp2->n);
            printf("Size: %ld\n", temp2->size);
            printf("Starting Address: %p\n", temp2->starting_address);
            printf("Ending Address: %p\n", temp2->ending_address);
            printf("Type: %d\n", temp2->type);
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
}

int main(int argc, char const *argv[])
{
    mems_init();
    print_system();
    return 0;
}
