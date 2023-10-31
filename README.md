# MeMS (Memory Management System)

MeMS is a custom memory management system implemented in C, designed to manage memory using the mmap and munmap system calls. It provides memory allocation and deallocation functionalities while adhering to specific constraints and requirements.

![Image](https://i.imgflip.com/84hgvp.jpg)


## Table of Contents

- [Overview](#overview)
- [Code Structure](#code-structure)
- [Functions](#functions)
- [Usage](#usage)
- [About the Code](#about-the-code)
- [License](#license)

## Overview

MeMS (Memory Management System) is a memory management solution that utilizes mmap and munmap system calls for memory allocation and deallocation, with the following key features:

- Memory allocation in multiples of the system's PAGE_SIZE (typically 4096 bytes, but can be changed if need so).
- Use of a free list data structure to keep track of allocated and deallocated memory segments.
- Support for allocating and deallocating memory within the MeMS virtual address space.

## Code Structure

The code for MeMS consists of the following components:

- `struct sub_node` and `struct main_node`: Data structures used for managing memory segments, including holes and processes.
- `initializing_main_node()`: Initializes a main_node struct.
- `mems_init()`: Initializes the MeMS system, creating the initial main node.
- `mems_finish()`: Deallocates all memory allocated by the MeMS system.
- `search_free_list(size_t size)`: Searches for available memory segments in the free list.
- `mems_malloc(size_t size)`: Allocates memory of the specified size.
- `mems_print_stats()`: Prints statistics on the MeMS system, including main nodes and sub nodes.
- `print_sub_node(struct sub_node* node)`: Prints information about a sub node.
- `mems_get(void*v_ptr)`: Retrieves information about a given MeMS virtual address.
- `merge_holes(struct main_node* node)`: Merges adjacent holes in a main node.
- `mems_free(void *v_ptr)`: Frees memory associated with a MeMS virtual address.

## Features

1. **Initialization:** Call `mems_init()` at the beginning of your program to initialize the MeMS system.

2. **Memory Allocation:** Use `void* mems_malloc(size_t size)` to allocate memory dynamically. It returns a MeMS virtual address.

3. **Memory Deallocation:** Use `void mems_free(void *v_ptr)` to free the memory allocated at the specified MeMS virtual address.

4. **Get Physical Address:** Use `void* mems_get(void *v_ptr)` to get the MeMS physical address mapped to the given virtual address.

5. **Print Statistics:** Call `mems_print_stats()` to print detailed statistics about the MeMS system.

6. **Cleanup:** At the end of your program, call `mems_finish()` to free all allocated memory and clean up MeMS resources.



## How to run

1. Download and unzip the folder.
2. Open `main` folder in `terminal`
3. Run
```
$ make
```
![Image](https://i.etsystatic.com/20023820/r/il/f37ede/2933086041/il_1588xN.2933086041_i98v.jpg)

![GIF](https://media2.giphy.com/media/v1.Y2lkPTc5MGI3NjExNGUzaWFvOW03cmU5cHE4bGNhdHd0a2RlbGZ0cmFycnF6dWpqcTlkeCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/iDrOnCNzeuZJ1sLugR/giphy.gif)

Then,
```
$ make run
```

![Animated GIF](https://i.kym-cdn.com/photos/images/original/002/595/955/ac9.gif)


And finally,

```
$ make clean
```

![Dump Garbage](https://media.tenor.com/gOprzBYItkEAAAAd/dump-garbage.gif)
