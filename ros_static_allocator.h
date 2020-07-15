#if !defined(ROS_STATIC_ALLOCATOR_H)
#define ROS_STATIC_ALLOCATOR_H

/*
 *******************************************************************************
 *                          (C) Copyright 2020 TUDelft                         *
 * Created: 09/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Static allocator                                                           *
 *                                                                             *
 *******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

/*
 *******************************************************************************
 *                              Type Definitions                               *
 *******************************************************************************
*/


// Structure: Allocator header block
typedef union block_h {
	struct {
		union block_h *next;     // Next element in the linked-list
		size_t size;             // Size rounded to units of sizeof(block_h)
	} d;
	max_align_t align;           // Memory alignment element
} block_h;


// Structure: Static memory block
typedef struct {
	size_t capacity;             // Maximum capacity (bytes) of static memory
	uint8_t *memory;             // Pointer to static memory block
	block_h *free_list;          // Pointer to free memory linked-list
	size_t free_memory_size;     // Remaining free memory
	size_t unit_size;            // Memory base unit size
} static_allocator_t;


/*
 *******************************************************************************
 *                           Interface Declarations                            *
 *******************************************************************************
*/


/*\
 * @brief Installs a static allocator within the given memory block
 * @note  Occupies a segment of the given memory for bookkeeping
 * @param static_memory Pointer to memory to use for the allocator
 * @param size Size of the memory to use
 * @return Pointer to allocator on success; NULL on error
\*/
static_allocator_t *install_static_allocator (uint8_t *static_memory, 
	size_t size);

/*\
 * @brief Allocates the requested amount of memory from the allocator
 * @param static_allocator Pointer to static allocator
 * @param size Amount of static memory to allocate
 * @return Pointer to memory block on success; NULL on error
\*/
uint8_t *static_alloc (static_allocator_t *static_allocator, size_t size);


/*\
 * @brief Returns memory to the free list
 * @param static_allocator Pointer to static allocator
 * @param block_ptr Pointer to the memory block to free
 * @return Zero on success; otherwise error
\*/
int static_free (static_allocator_t *static_allocator, uint8_t *block_ptr);


/*\
 * @brief Debug utility which shows what is on the free list
 * @param static_allocator Pointer to static allocator
 * @return None
\*/
void static_show (static_allocator_t *static_allocator);


/*\
 * @brief Debug utility for memory unification check
 * @param static_allocator Pointer to static allocator
 * @return None
\*/
bool static_is_unified (static_allocator_t *static_allocator);

#endif