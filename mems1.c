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

struct sub_node {
    int n;
    size_t size;
    struct sub_node* next;
    struct sub_node* prev;
    void* starting_address;
    void* ending_address;
    int type;  // 0 for HOLE and 1 for PROCESS
};

struct main_node {
    size_t size; // this is in multiples of PAGE_SIZE
    struct main_node* next;
    struct sub_node* sub_head;
    struct main_node* prev;
    void* address_in_PA;
    int no_of_process;
    int no_of_holes;
    size_t holes_size;
};

struct main_node* main_head = NULL;

void initializing_main_node(struct main_node* node) {
    // node->size = PAGE_SIZE;
    node->next = NULL;
    node->prev = NULL;
    struct sub_node* sub_head = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    node->sub_head = sub_head;
    node->no_of_process = 0;
    node->no_of_holes = 1;
    node->holes_size = PAGE_SIZE;
    node->address_in_PA = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    sub_head->size = PAGE_SIZE;
    sub_head->next = NULL;
    sub_head->prev = NULL;
    sub_head->starting_address = node->address_in_PA;
    sub_head->ending_address = (void*)((char*)node->address_in_PA + node->size - 1);
    sub_head->type = 0;
    sub_head->n = 1;
}

void mems_init() {
    main_head = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    initializing_main_node(main_head);
    printf("%p\n", main_head);
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
// */
void mems_finish() {
    struct main_node* temp = main_head;
    while (temp != NULL) {
        temp = temp->next;
    }
    while (temp != NULL) {
        struct main_node* temp2 = temp;
        temp = temp->prev;
        munmap(temp2->address_in_PA, PAGE_SIZE);
    }
    // now we will free all main nodes and sub nodes
    temp = main_head;
    while (temp != NULL) {
        struct main_node* temp2 = temp;
        temp = temp->next;
        munmap(temp2, sizeof(struct main_node));
    }
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

void print_main_node(struct main_node* node) {
    printf("Main Node\n");
    printf("Address in PA: %p\n", node->address_in_PA);
    printf("No of process: %d\n", node->no_of_process);
    printf("No of holes: %d\n", node->no_of_holes);
    printf("Holes size: %ld\n", node->holes_size);
}

int search_free_list(size_t size) {
    printf("Welcome to search_free_list\n");
    struct main_node* temp = main_head;
    printf("%p\n", main_head);
    while (temp != NULL) { // Iterate through the main_node linked list
        struct sub_node* temp2 = temp->sub_head; // Start with the sub_head of the current main_node
        
        while (temp2 != NULL) { // Iterate through the sub_node linked list
            if (temp2->type == 0 && temp2->size >= size) {
                printf("Checker4\n");
                return 1;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    return 0;
}

void* mems_malloc(size_t size){
    printf("Welcome to mems_malloc\n");
    // if the user wants a size which is not present in the free list then we will use mmap and create a new main node
    if(search_free_list(size)){
        printf("%d\n",search_free_list(size));
        // now allocate that node to the user
        struct main_node* temp = main_head;
        struct sub_node* temp2 = temp->sub_head;
        while(temp2->next!=NULL){
            if(temp2->type==0 && temp2->size>size){
                printf("Checker1\n");
                // now we will create a new sub node for the process
                struct sub_node* process = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                printf("Checker2\n");
                process->size = size;
                process->next = NULL;
                process->prev = temp2;
                process->starting_address = temp2->ending_address - size + 1;
                process->ending_address = temp2->ending_address;
                temp2->ending_address = process->starting_address - 1;
                // now we will add process node to the end of the sub chain
                struct sub_node* temp3 = temp->sub_head;
                int n = 1;
                while(temp3->next!=NULL){
                    temp3 = temp3->next;
                    n++;
                }
                temp3->next = process;
                temp->no_of_process++;
                temp->no_of_holes--;
                temp->holes_size = temp->holes_size - size;
                process->type = 1;
                process->n = n+1;
                return process->starting_address;
            }
            if(temp2->type==0 && temp2->size==size){
                // simply chaneg the node to a process node
                temp2->type = 1;
                temp->no_of_process++;
                temp->no_of_holes--;
                temp->holes_size = temp->holes_size - size;
                return temp2->starting_address;
            }
            temp2 = temp2->next;
        }
    }
    else{
        // create a new main node and allocate that node to the user
        // here there will be two cases
        // 1. if the size is less than PAGE_SIZE
        // 2. if the size is greater than PAGE_SIZE
        struct main_node* temp = main_head;
        while(temp->next!=NULL){
            temp = temp->next;
        }
        struct main_node* new_node = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        initializing_main_node(new_node);
        temp->next = new_node;
        new_node->prev = temp;
        int no_of_pages = size/PAGE_SIZE;
        if(size/PAGE_SIZE!=0){
            no_of_pages++;
        }
        new_node->size = no_of_pages*PAGE_SIZE;
        new_node->address_in_PA = mmap(NULL, new_node->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        // now the head sub node will be a hole and a new sub node will be created for the process
        struct sub_node* process = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        process->size = size;
        process->next = NULL;
        process->prev = new_node->sub_head;
        process->starting_address = new_node->sub_head->ending_address - size + 1;
        process->ending_address = new_node->sub_head->ending_address;
        new_node->sub_head->ending_address = process->starting_address - 1;
        new_node->sub_head->next = process;
        new_node->sub_head->type = 0;
        new_node->sub_head->n = 1;
        new_node->no_of_process = 1;
        new_node->no_of_holes = 1;
        new_node->holes_size = new_node->size - size;
        process->type = 1;
        process->n = 2;
        return process->starting_address;
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
    struct main_node* temp = main_head;
    while (temp != NULL) {
        printf("Main Node\n");
        printf("Address in PA: %p\n", temp->address_in_PA);
        printf("No of process: %d\n", temp->no_of_process);
        printf("No of holes: %d\n", temp->no_of_holes);
        printf("Holes size: %ld\n", temp->holes_size);
        struct sub_node* temp2 = temp->sub_head;
        while (temp2 != NULL) {
            printf("Sub Node\n");
            printf("Sub Node no: %d\n",temp2->n);
            printf("Size: %ld\n", temp2->size);
            printf("Starting Address: %p\n", temp2->starting_address);
            printf("Ending Address: %p\n", temp2->ending_address);
            printf("Type: %d\n", temp2->type);
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
}


/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
// */
void *mems_get(void*v_ptr){
    // here we will check if the v_ptr is in the main chain or not
    struct main_node* temp = main_head;
    while(temp->next!=NULL){
        struct sub_node* temp2 = temp->sub_head;
        while(temp2->next!=NULL){
            if(temp2->starting_address<=v_ptr && temp2->ending_address>=v_ptr){
                return v_ptr;
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
// void mems_free(void *v_ptr){
    
    
// }

int main(int argc, char const *argv[])
{
    mems_init();
    mems_print_stats();
    mems_finish();
    int *ptr = (int*)mems_malloc(sizeof(int)*25);
    printf("Virtual address: %lu\n", (unsigned long)ptr);
    mems_print_stats();
    return 0;
}
