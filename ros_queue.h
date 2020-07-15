#if !defined(ROS_QUEUE_H)
#define ROS_QUEUE_H

/*
 *******************************************************************************
 *                          (C) Copyright 2020 TUDelft                         *
 * Created: 09/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Interface for generic queue                                                *
 *                                                                             *
 *******************************************************************************
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


/*
 *******************************************************************************
 *                              Type Definitions                               *
 *******************************************************************************
*/


// Structure: Queue data structure
typedef struct {
	void **array;                       // Array of queue elements
	off_t  ptr;                         // Queue indexing pointer
	size_t len;                         // Queue length
	size_t cap;                         // Total capacity

	uint8_t *(*alloc)(size_t size);     // Allocator for more memory
	void (*release)(uint8_t *mem_ptr);  // Deallocator for memory
} queue_t;


/*
 *******************************************************************************
 *                           Interface Declarations                            *
 *******************************************************************************
*/


/*\
 * @brief Creates a queue with the given allocator 
 * @param capacity Maximum number of queue elements
 * @param alloc Pointer to memory allocation routine
 * @param release Pointer to memory de-allocation routine
 * @return NULL on error; else valid pointer to queue_t instance
\*/
queue_t *make_queue (size_t capacity, uint8_t *(*alloc)(size_t), 
	void (*release)(uint8_t *));


/*\
 * @brief Inserts an element into the queue
 * @param elem_p Pointer to element to store
 * @param queue_p Pointer to queue
 * @return Zero on success; 1 on bad param; 2 on reached capacity
\*/
int enqueue (void *elem_p, queue_t *queue_p);

/*\
 * @brief Removes an element from the queue
 * @param elem_p_p Pointer at which to copy dequeued element pointer
 * @param queue_p Pointer to queue
 * @return Zero on success; 1 on bad param; 2 on zero capacity
\*/
int dequeue (void **elem_p_p, queue_t *queue_p);


/*\
 * @brief Frees memory associated with queue
 * @param queue_p Pointer to queue
 * @return Zero on success; 1 on bad parameter
\*/
int destroy_queue (queue_t *queue_p);

/*\
 * @brief Queue debug utility
 * @param queue_p Pointer to queue
 * @param show Pointer to function to call on element
 * @return None
\*/
void show_queue (queue_t *queue_p, void (*show)(void * const elem));

#endif