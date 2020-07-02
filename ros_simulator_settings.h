#if !defined(ROS_SIMULATOR_SETTINGS_H)
#define ROS_SIMULATOR_SETTINGS_H

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

/*
 *******************************************************************************
 *                             Symbolic Constants                              *
 *******************************************************************************
*/

// Maximum number of tasks
#define MAX_TASK_COUNT          4

// Depth of a callback queue
#define TASK_MSG_QUEUE_DEPTH    16

// Maximum supported of pollable file-descriptors
#define MAX_POLLABLE_FDS        8

/*
 *******************************************************************************
 *                              Type Definitions                               *
 *******************************************************************************
*/

// Type describing a message
typedef struct {
	uint8_t prio;
	uint8_t data;
} msg_t;

// Type describing a task
typedef struct {
	int pid;                            // ID returned from fork
	void (*callback)(void *msg);        // Callback to execute in said thread
	off_t queue_index;                  // Amount of pending instances to run
	msg_t queue[TASK_MSG_QUEUE_DEPTH];  // Queue holding message data to run
} task_t;

#endif