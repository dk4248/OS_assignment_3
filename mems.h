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
    size_t size;
    struct main_node* next;
    struct sub_node* sub_head;
    struct main_node* prev;
    void* address_in_PA;
    int no_of_process;
    int no_of_holes;
    size_t holes_size;
};

struct main_node* main_head = NULL;

void initializing_main_node(struct main_node* node , size_t size) {
    node->size = size;
    node->next = NULL;
    node->prev = NULL;
    struct sub_node* sub_head = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    node->sub_head = sub_head;
    node->no_of_process = 0;
    node->no_of_holes = 1;
    node->holes_size = PAGE_SIZE;
    node->address_in_PA = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
    initializing_main_node(main_head, PAGE_SIZE);
}

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
    printf("MeMS system is Finished\n");
}

// void print_main_node(struct main_node* node) {
//     printf("Main Node\n");
//     printf("Address in PA: %p\n", node->address_in_PA);
//     printf("No of process: %d\n", node->no_of_process);
//     printf("No of holes: %d\n", node->no_of_holes);
//     printf("Holes size: %ld\n", node->holes_size);
// }

int search_free_list(size_t size) {
    struct main_node* temp = main_head;
    while (temp != NULL) {
        // printf("%p",temp->address_in_PA);
        struct sub_node* temp2 = temp->sub_head;
        while (temp2 != NULL) {
            if (temp2->type == 0 && temp2->size >= size) {
                return 1;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    return 0;
}

void* mems_malloc(size_t size){
    // if the user wants a size which is not present in the free list then we will use mmap and create a new main node
    if(search_free_list(size)){
        // printf("Size is available in the free list\n");
        // printf("%d\n",search_free_list(size));
        // now allocate that node to the user
        struct main_node* temp = main_head;
        struct sub_node* temp2 = temp->sub_head;
        // printf("Checker2\n");
        while(temp!=NULL){
            temp2 = temp->sub_head;
                while(temp2!=NULL){
                    if(temp2->type==0 && temp2->size>size){
                        // printf("Size of main node %ld\n",temp->size);
                        // printf("Checker\n");
                        // now we will create a new sub node for the process
                        struct sub_node* process = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
                        // temp->no_of_holes;
                        temp->holes_size = temp->holes_size - size;
                        process->type = 1;
                        process->n = n+1;
                        temp2->size = temp2->size - size;
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
            temp = temp->next;
    }}
    else{
        //the size user wants is not available in the free list
        struct main_node* new_node = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        int t = size/PAGE_SIZE;
        t++;
        size_t new_size = t*PAGE_SIZE;
        initializing_main_node(new_node, new_size);
        struct main_node* temp = main_head;
        while(temp->next!=NULL){
            temp = temp->next;
        }
        temp->next = new_node;
        new_node->prev = temp;
        struct sub_node* process = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        process->size = size;
        process->next = NULL;
        process->prev = new_node->sub_head;
        process->starting_address = new_node->sub_head->ending_address - size + 1;
        process->ending_address = new_node->sub_head->ending_address;
        new_node->sub_head->ending_address = process->starting_address - 1;
        new_node->sub_head->next = process;
        new_node->no_of_process++;
        new_node->sub_head->size = new_node->sub_head->size - size;
        new_node->size = new_node->size - size;
        new_node->holes_size = new_node->holes_size - size;
        process->type = 1;
        process->n = 2;
        return process->starting_address;
       }
}

void mems_print_stats(){
    int n = 1;
    struct main_node* temp = main_head;
    while (temp != NULL) {
        printf("\n\n");
        printf("Main Node %d\n",n);
        printf("Address in PA: %p\n", temp->address_in_PA);
        printf("No of process: %d\n", temp->no_of_process);
        printf("No of holes: %d\n", temp->no_of_holes);
        printf("Holes size: %ld\n", temp->holes_size);
        struct sub_node* temp2 = temp->sub_head;
        while (temp2 != NULL) {
            printf("\n");
            printf("Sub Node\n");
            printf("Sub Node no: %d\n",temp2->n);
            printf("Size: %ld\n", temp2->size);
            printf("Starting Address: %p\n", temp2->starting_address);
            printf("Ending Address: %p\n", temp2->ending_address);
            printf(temp2->type == 0 ? "Type: HOLE\n" : "Type: PROCESS\n");
            temp2 = temp2->next;
        }
        n++;
        temp = temp->next;
    }
}

void print_sub_node(struct sub_node* node) {
    printf("Sub Node\n");
    printf("Sub Node no: %d\n",node->n);
    printf("Size: %ld\n", node->size);
    printf("Starting Address: %p\n", node->starting_address);
    printf("Ending Address: %p\n", node->ending_address);
    printf("Type: %d\n", node->type);
}

void *mems_get(void*v_ptr){
    // here we will check if the v_ptr is in the main chain or not
    struct main_node* temp = main_head;
    while(temp->next!=NULL){
        struct sub_node* temp2 = temp->sub_head;
        while(temp2->next!=NULL){
            if(temp2->starting_address<=v_ptr && temp2->ending_address>v_ptr){
                print_sub_node(temp2);
                return v_ptr;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    return NULL;
}

void merge_holes(struct main_node* node) {
    struct sub_node* temp = node->sub_head;
    while (temp != NULL && temp->next != NULL) {
        if (temp->type == 0 && temp->next->type == 0) {
            temp->size += temp->next->size;
            temp->next = temp->next->next;

            if (temp->next != NULL) {
                temp->next->prev = temp;
                temp->ending_address = temp->next->ending_address;
            }
            else {
                temp->ending_address = temp->starting_address + temp->size - 1;
            }

            node->no_of_holes--;
            munmap(temp->next, sizeof(struct sub_node));
            struct sub_node* temp2 = temp->next;
            while (temp2 != NULL) {
                temp2->n--;
                temp2 = temp2->next;
            }
        }
        temp = temp->next;
    }
}

void mems_free(void *v_ptr) {
    struct main_node* temp = main_head;
    while (temp != NULL) {
        struct sub_node* temp2 = temp->sub_head;
        while (temp2 != NULL) {
            if (temp2->starting_address == v_ptr) {
                // Check if it was a process node
                if (temp2->type == 1) {
                    temp2->type = 0;
                    temp->no_of_process--;
                    temp->no_of_holes++;
                    temp->holes_size += temp2->size;
                    merge_holes(temp);
                    return;
                } else {
                    printf("Error: The address is already a hole\n");
                    return;
                }
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    printf("Error: The address is not in the MeMS system\n");
}

