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

// Structure: Task data definition
typedef struct {
	uint8_t prio;
	uint8_t data;
} data_t;

// Structure: Task definition
typedef struct {
	pid_t pid;
	void (*cb)(void *data);
	off_t queue_len;
	data_t queue[TASK_MSG_QUEUE_DEPTH];
} task_t;

#endif