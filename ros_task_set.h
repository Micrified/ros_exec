#if !defined(ROS_TASK_SET_H)
#define ROS_TASK_SET_H

/*
 *******************************************************************************
 *                          (C) Copyright 2020 TUDelft                         *
 * Created: 16/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Data structure for inter-process task managment (compile with -lpthread)   *
 *                                                                             *
 *******************************************************************************
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <semaphore.h>

#include "ros_queue.h"

/*
 *******************************************************************************
 *                              Type Definitions                               *
 *******************************************************************************
*/


// Structure: Describes task callback data
typedef struct {
	void *data_p;                         // Pointer to the data vector
	size_t data_size;                     // Size (in bytes) of data vector
} task_callback_data_t;


// Structure: Describes a callback data element
typedef struct {
	uint8_t prio;                         // Callback priority
	task_callback_data_t *callback_data;  // Callback data pointer
} task_callback_t;


// Structure: Describes a task
typedef struct {
	pid_t pid;                            // PID of the owner task
	void (*cb) (void *callback_data);     // Callback to execute on message
	queue_t *queue;                       // Pointer to data queue
} task_t;


// Structure: Describes a task set
typedef struct {
	sem_t sem;                            // Interprocess access semaphore
	off_t current_running_task_id;        // ID of the current task (signed)
	size_t len;                           // Number of tasks
	size_t queue_depth;                   // Depth of the task data queues
	task_t *tasks;                        // Task element array
	uint8_t *(*alloc)(size_t size);       // Allocator for more memory
	void (*release)(uint8_t *mem_ptr);    // Deallocator for memory
} task_set_t;

/*
 *******************************************************************************
 *                           Interface Declarations                            *
 *******************************************************************************
*/


/*\
 * @brief Creates a task set using the given allocator
 * @param len     The number of tasks in the task set
 * @param queue_depth   The depth of the queue each callback gets
 * @param alloc   Pointer to allocation function
 * @param release Pointer to free function
\*/
task_set_t *make_task_set (size_t len, size_t queue_depth,
 uint8_t *(*alloc)(size_t), void (*release)(uint8_t *));


/*\
 * @brief Returns the index of the highest priority task
 *        based on its data queue. 
 * @note If no task has data, then -1 is returned
 * @param task_set_p The set of tasks
 * @return Task index; -1 if not found 
\*/
int get_highest_prio_task_index (task_set_t *task_set_p);

/*\
 * @brief Allocates and inserts callback data for a task
 * @note  The given data is copied
 * @param task_id    The ID of the task to enqueue the data with
 * @param prio       The priority of the callback instance
 * @param data_size  Size of the data to copy
 * @param data       Pointer to the data to copy
 * @param task_set_p Pointer to the task set
 * @return Zero on success; otherwise:
 *        1: Either data or task_set_p is NULL
 *        2: Task ID is out of bounds
 *        3: Unable to allocate copy of callback data
 *        4: Unable to allocate callback data type
 *        5: Unable to allocate callback descriptor
 *        6: Unable to enqueue the data with the specified task
\*/
int enqueue_callback_for_task (off_t task_id, uint8_t prio, size_t data_size, void *data, 
	task_set_t *task_set_p);

/*\
 * @brief Dequeue data element for given task
 * @note The user MUST perform the following when they no longer need the dequeued data
 *    (1) They MUST free subfield data_p in field callback_data
 *    (2) They MUST free field callback_data
 *    (3) They MUST free the callback itself
 * @param task_id                 ID of the task to dequeue from
 * @param task_callback_data_p_p  Pointer to location to install task data pointer
 * @param task_set_p              Task set
 * @return Zero on success; else
 *         1: Either task_callback_data_p_p ir task_set_p is NULL
 *         2: Task ID is out of bounds
 *         3: Unable to dequeue from task
\*/
int dequeue_callback_for_task (off_t task_id, task_callback_t **task_callback_p_p,
	task_set_t *task_set_p);

/*\
 * @brief Releases memory associated with a callback
 * @param callback_p Pointer to callback data structure
 * @param task_set_p Pointer to the task set
 * @return Zero on success; 1 on bad parameters
\*/
int free_task_callback (task_callback_t *callback_p, task_set_t *task_set_p);


/*\
 * @brief Displays the task set
 * @param task_set_p Pointer to task set
 * @return None
\*/
void show_task_set (task_set_t *task_set_p);


/*\
 * @brief Destroys a task set by releasing allocated memory back to underlying
 *        allocator
 * @param task_set_p Pointer to task set
 * @return Zeron on success; 1 on bad parameters, 2 if unable to destroy semaphore
\*/
int destroy_task_set (task_set_t *task_set_p);


#endif