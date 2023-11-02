#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>

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
    int type;
};

struct main_node {
    int no_of_pages;
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
    node->no_of_pages = size / PAGE_SIZE;
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
    printf("%p\n", main_head);
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
void* mems_malloc(size_t size) {
    if (search_free_list(size)) {
        // space is available in the free list
        // we will search for the first hole which is greater than the size and then we will allocate the memory to the process node
        struct main_node* temp = main_head;
        while(temp!=NULL){
            struct sub_node* temp2 = temp->sub_head;
            while(temp2!=NULL){
                if(temp2->type==0 && temp2->size>size){
                    // now we will create a process node and add it before the hole node
                    struct sub_node* process_node = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    process_node->size = size;
                    process_node->starting_address = temp2->starting_address;
                    process_node->ending_address = (void*)((char*)temp2->starting_address + size - 1);
                    process_node->type = 1;
                    temp2->starting_address = process_node->ending_address + 1;
                    temp2->size -= size;
                    temp->no_of_process++;
                    temp->holes_size -= size;
                    // Update the linked list pointers to insert the new process node.
                    if(temp->no_of_process==1){
                        process_node->prev = NULL;
                        process_node->next = temp2;
                        temp2->prev = process_node;
                        temp->sub_head = process_node;
                    }
                    else{
                        process_node->prev = temp2->prev;
                        process_node->next = temp2;
                        temp2->prev->next = process_node;
                        temp2->prev = process_node;
                    }
                    return process_node->starting_address;
                }
                temp2 = temp2->next;
            }
            temp = temp->next;
        }
    } else {
        // The size is not available in the free list, so we will create a new main node.
        struct main_node* temp = main_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        size_t size_of_main_node = size/ PAGE_SIZE;
        size_of_main_node++;
        temp->no_of_pages = size_of_main_node;
        struct main_node* new_node = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        initializing_main_node(new_node, size);
        new_node->no_of_pages++;
        temp->next = new_node;
        new_node->prev = temp;
        struct sub_node* process_node = (struct sub_node*)mmap(NULL, sizeof(struct sub_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        process_node->size = size;
        process_node->starting_address = new_node->sub_head->starting_address;
        process_node->ending_address = (void*)((char*)new_node->sub_head->starting_address + size - 1);
        process_node->type = 1;
        new_node->sub_head->starting_address = process_node->ending_address + 1;
        new_node->sub_head->size -= size;

        // Update the linked list pointers to insert the new process node.
        process_node->prev = NULL;
        process_node->next = new_node->sub_head;
        new_node->sub_head->prev = process_node;
        new_node->sub_head = process_node;
        new_node->no_of_process++;
        new_node->holes_size -= size;
        return process_node->starting_address;
    }
    return NULL;
}

void mems_print_stats() {
    int total_pages_used = 0;
    size_t total_holes_size = 0;
    printf("\n\n");
    printf("-------------------- MeMS System Stats --------------------\n");
    int count = 0;
    struct main_node* temp = main_head;
    while (temp != NULL) {
        // Use uintptr_t for decimal address conversion
        uintptr_t addr_start = (uintptr_t)temp->address_in_PA;
        uintptr_t addr_end = (uintptr_t)((char*)temp->address_in_PA + temp->size - 1);
        
        printf("MAIN[%lu,%lu]", addr_start, addr_end);  // Print decimal addresses
        printf(" -> ");
        struct sub_node* temp2 = temp->sub_head;
        while (temp2 != NULL) {
            if (temp2->type == 0) {
                uintptr_t hole_start = (uintptr_t)temp2->starting_address;
                uintptr_t hole_end = (uintptr_t)temp2->ending_address;
                
                printf("H[%lu,%lu]", hole_start, hole_end);  // Print decimal addresses
                printf(" <-> ");
                total_holes_size += temp2->size;
            } else {
                uintptr_t process_start = (uintptr_t)temp2->starting_address;
                uintptr_t process_end = (uintptr_t)temp2->ending_address;
                
                printf("P[%lu,%lu]", process_start, process_end);  // Print decimal addresses
                printf(" <-> ");
            }
            temp2 = temp2->next;
        }
        printf("NULL");
        printf("\n");
        total_pages_used += temp->no_of_pages;
        temp = temp->next;
        count++;
    }
    printf("\n");
    printf("Pages Used: %d\n", total_pages_used);
    printf("Space Unused: %ld\n", total_holes_size);
    printf("Main Chain Length: %d\n", count);
    printf("\n");
    printf("Sub - Chain Length Array:");
    temp = main_head;
    while (temp != NULL) {
        if (temp == main_head) {
            printf("[ ");
        }
        printf("%d, ", temp->no_of_process + temp->no_of_holes);
        if (temp->next == NULL) {
            printf(" ]");
        }
        temp = temp->next;
    }
    printf("\n");
}
// void print_sub_node(struct sub_node* node) {
//     printf("Sub Node\n");
//     printf("Sub Node no: %d\n",node->n);
//     printf("Size: %ld\n", node->size);
//     printf("Starting Address: %p\n", node->starting_address);
//     printf("Ending Address: %p\n", node->ending_address);
//     printf("Type: %d\n", node->type);
// }

void *mems_get(void*v_ptr){
    // here we will check if the v_ptr is in the main chain or not
    struct main_node* temp = main_head;
    while(temp->next!=NULL){
        struct sub_node* temp2 = temp->sub_head;
        while(temp2->next!=NULL){
            if(temp2->starting_address<=v_ptr && temp2->ending_address>v_ptr){
                return v_ptr;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    return NULL;
}

// void search_for_adjacent_holes() {
//     printf("Searching for adjacent holes\n");
//     struct main_node* temp = main_head;
//     while (temp != NULL) {
//         struct sub_node* temp2 = temp->sub_head;
//         while (temp2 != NULL) {
//             if (temp2->next != NULL) {
//                 if (temp2->ending_address + 1 == temp2->next->starting_address && (temp2->type == 0) && (temp2->next->type == 0)) {
//                     temp2->size += temp2->next->size;
//                     temp2->next = temp2->next->next;
//                     temp2->ending_address = temp2->next->ending_address;
//                     temp->no_of_holes--;
//                     munmap(temp2->next, sizeof(struct sub_node));
//                     struct sub_node* temp3 = temp2->next;
//                     while (temp3 != NULL) {
//                         temp3->n--;
//                         temp3 = temp3->next;
//                     }
//                 }
//             }
//             temp2 = temp2->next;
//         }
//         temp = temp->next;
//     }
// }

void helper(void* v_ptr , struct sub_node* temp2 , struct main_node* temp){
    if(temp2->next!=NULL){
        // if the next node is hole then we will merge the two holes
        // if it is process then we will change the type of the node to hole
        if(temp2->next->type==0){
            // we will merge the two holes
            temp2->size += temp2->next->size;
            temp2->ending_address = temp2->next->ending_address;
            if(temp2->next->next!=NULL){
            temp2->next = temp2->next->next;}
            else{
                temp2->next = NULL;
            }
            temp->no_of_holes--;
            struct sub_node* temp3 = temp2->next;
            while (temp3 != NULL) {
                temp3->n--;
                temp3 = temp3->next;
            }
        }else{
            // we will change the type of the node to hole
            temp2->type = 0;
            temp->no_of_process--;
            temp->no_of_holes++;
            temp->holes_size += temp2->size;
        }
    }if(temp2->prev!=NULL){
        // if the previous node is hole then we will merge the two holes
        // if it is process then we will change the type of the node to hole
        if(temp2->prev->type==0){
            // we will merge the two holes
            temp2->prev->size += temp2->size;
            temp2->prev->next = temp2->next;
            temp2->prev->ending_address = temp2->ending_address;
            temp->no_of_holes--;
            temp2->next->prev = temp2->prev;
            // munmap(temp2, sizeof(struct sub_node));
            struct sub_node* temp3 = temp2->next;
            while (temp3 != NULL) {
                temp3->n--;
                temp3 = temp3->next;
            }
        }else{
            // we will change the type of the node to hole
            temp2->type = 0;
            temp->no_of_process--;
            temp->no_of_holes++;
            temp->holes_size += temp2->size;
        }
    }else{
        // we will simply change the type of the node to hole
        temp2->type = 0;
        temp->no_of_process--;
        temp->no_of_holes++;
        temp->holes_size += temp2->size;
    }

    
}


void mems_free(void *v_ptr) {
    // if v_ptr is not in the main chain then we will return else we will take it to a helper function
    struct main_node* temp = main_head;
    while(temp!=NULL){
        struct sub_node* temp2 = temp->sub_head;
        while(temp2!=NULL){
            if(temp2->starting_address<=v_ptr && temp2->ending_address>v_ptr){
                helper(v_ptr,temp2,temp);
                return;
            }
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    printf("The address is not in the main chain\n");
    return;
}
