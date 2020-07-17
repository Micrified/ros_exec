#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "ros_exec_shm.h"
#include "ros_queue.h"
#include "ros_task_set.h"
#include "ros_static_allocator.h"


/*
 *******************************************************************************
 *                              Global Variables                               *
 *******************************************************************************
*/


// Access semaphore 
sem_t g_sem;

// Shared memory pointer
void *g_shm;

// Allocator
static_allocator_t *g_allocator;

// Task set
task_set_t *g_task_set;

// PID of self-process
pid_t g_pid = -1;


/*
 *******************************************************************************
 *                              Support Functions                              *
 *******************************************************************************
*/


uint8_t *alloc (size_t size)
{
	if (g_allocator != NULL) {
		return static_alloc(g_allocator, size);
	} else {
		return NULL;
	}
}

void release (uint8_t *ptr)
{
	if (g_allocator != NULL) {
		static_free(g_allocator, ptr);
	}
}


/*
 *******************************************************************************
 *                               Task Procedure                                *
 *******************************************************************************
*/


void task_routine (uint8_t task_id)
{
	do {
		// Self suspend
		printf("Process %d going to sleep ... zzz\n", g_pid);
		kill(g_pid, SIGSTOP);

		// Run code
		switch (task_id) {
			
		}
	} while (1);
}

/*
 *******************************************************************************
 *                                    Main                                     *
 *******************************************************************************
*/


int main (int argc, char *argv[])
{
	// Configuration
	const char *shm_map_name  = "ros_exec_shm";
	const size_t shm_map_size = 8192;
	pid_t status, pid = -1;
	int n_tasks = -1;
	size_t task_queue_size = 5;

	// Check argument count
	if (argc != 2) {
		printf("%s [n-forks]\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Read number of forks
	n_tasks = atoi(argv[1]);

	printf("Process Count:\t\t%d\n", n_tasks);

	// Initialize shared memory
	if ((g_shm = map_shared_memory(
		shm_map_name,
		shm_map_size,
		is_owner)) == NULL)
	{
		return EXIT_FAILURE;
	}

	printf("Shared Memory:\t\tReady\n");

	// Initialize static allocator
	g_allocator = install_static_allocator(g_shm, shm_map_size);

	printf("Static Allocator:\t\tReady\n");

	// Initialize task set
	g_task_set = make_task_set(n_tasks, task_queue_size, alloc, release);

	printf("Task Data Set:\t\tReady\n");


	// Fork some processes
	for (off_t i = 1; i < n_tasks; ++i) {
		if ((g_pid = fork()) == 0) {

			// Update self PID
			g_pid = getpid();

			// Print message
			printf("Process (%d) for task %lu!\n", 
				getpid(), i - 1);

			// Run the task procedure
			task_routine();

			// Should never reach here
			fprintf(stderr, "ILLEGAL POINT!\n");
			return EXIT_FAILURE;
		}
	}

	// Wait for child forks
	while ((pid = wait(&status)) > 0);

	// Print result
	printf("Sum = %d\n", data_ptr->sum);

end:
	// Remove shared memory
	if (unmap_shared_memory(
		shm_map_name,
		shm_ptr,
		shm_map_size,
		is_owner) != 0)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}