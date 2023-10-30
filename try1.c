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
    size_t size;
    struct sub_node* next;
    struct sub_node* prev;
    void* starting_address;
    void* ending_address;
    int type;  // 0 for HOLE and 1 for PROCESS
};

struct main_node {
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
    sub_head->ending_address = (void*)((char*)node->address_in_PA + PAGE_SIZE - 1);
    sub_head->type = 0;
}

void mems_init() {
    main_head = (struct main_node*)mmap(NULL, sizeof(struct main_node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    initializing_main_node(main_head);
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
}

void print_system() {
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
            printf("Size: %ld\n", temp2->size);
            printf("Starting Address: %p\n", temp2->starting_address);
            printf("Ending Address: %p\n", temp2->ending_address);
            printf("Type: %d\n", temp2->type);
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
}

int main(int argc, char const* argv[]) {
    mems_init();
    print_system();
    mems_finish();
    return 0;
}
