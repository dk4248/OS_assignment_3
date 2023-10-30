#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



int main(int argc, char const *argv[])
{
    // // making a 2d array
    // int arr[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    // int *ptrm = (int*)malloc(3*(3*sizeof(int)));
    // ptrm = &arr[0][0];
    // //mems va allocate through mmap
    // int* memsva = (int*)mmap(NULL, 3*(3*sizeof(int)), PROT_READ|PROT_WRITE, MAP_SHARED, -1, 0);
    // memsva = &arr[0][0];
    // // now making a VA to get the value of mmap

    // // printing the first element of the array through pointer
    // printf("%d\n", *ptrm);
    int *ptr[10];
    for (int i = 0; i < 10; i++)
    {
        ptr[i] = 4+i*4;
    }
    for (int i = 0; i < 10; i++)
    {
        printf("%p\n", ptr[i][1]);
    }
    
    
    return 0;
}
