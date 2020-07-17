#define _POSIX_SOURCE
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
#include <inttypes.h>
#include <math.h>

#include "ros_exec_shm.h"
#include "ros_queue.h"
#include "ros_task_set.h"
#include "ros_static_allocator.h"


/*
 *******************************************************************************
 *                             Symbolic Constants                              *
 *******************************************************************************
*/


#define MAX_PREEMPTABLE_TASKS        255

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


static uint8_t *alloc (size_t size)
{
	if (g_allocator != NULL) {
		return static_alloc(g_allocator, size);
	} else {
		return NULL;
	}
}

static void release (uint8_t *ptr)
{
	if (g_allocator != NULL) {
		static_free(g_allocator, ptr);
	}
}

static bool isPrime (unsigned int p) {
    int i, root;

    if (p == 1) return false;
    if (p == 2) return true;
    if ((p % 2)  == 0) return false;

    root = (int)(1.0 + sqrt(p));

    for (i = 3; (i < root) && (p % i != 0); i += 2);
    return i < root ? false : true;
}

/*
 *******************************************************************************
 *                               Task Procedure                                *
 *******************************************************************************
*/

void task_callback (void *data)
{
	// Assume we receive callback data
	task_callback_data_t *task_callback_data = (task_callback_data_t *)data;

	// Print length of data and pointer value
	//printf("callback (%zu, %p)\n", task_callback_data->data_size, 
	//	task_callback_data->data_p);

	// Compute primes up to multiples of 10m
	unsigned int primes_to_sum = 50000000;
	volatile int sum = 0;

	for (unsigned int n = 0; n < primes_to_sum; ++n) {
		sum += isPrime(n);
	}

	printf("[%d] Sum = %d\n", g_pid, sum);
}


void task_routine (off_t task_id)
{
	task_t *task_p = NULL;
	task_callback_t *callback_p = NULL;
	int err;
	void (*callback)(void *);

	// Set the task
	task_p = g_task_set->tasks + task_id;

	do {
		// Self suspend
		kill(g_pid, SIGSTOP);

		// **** Critical Section ****
		// (set self as currently running task + extract callback data)
		sem_wait(&(g_task_set->sem));
		g_task_set->current_running_task_id = task_id;

		// Print wakeup
		printf("[%d] Awoken!\n", g_pid);

		// Show task set
		// printf("[%d] My task set:\n", g_pid);
		// show_task_set(g_task_set);

		if ((err = dequeue_callback_for_task(task_id, &callback_p,
			g_task_set)) != 0) {
			fprintf(stderr, "Process %d: Unable to dequeue data (%d)\n",
				g_pid, err);
			continue;
		}
		sem_post(&(g_task_set->sem));
		// **** END critical section ****

		// Execute the callback with the data
		if (task_p->cb != NULL) {
				task_p->cb(callback_p->callback_data);		
		}

		// **** Critical Section ****
		// Free callback data, and update as no longer active
		sem_wait(&(g_task_set->sem));
		if ((err = free_task_callback(callback_p, g_task_set)) != 0) {
			fprintf(stderr, "[%d]: Unable to free data (%d)\n",
				g_pid, err);
		}
		g_task_set->current_running_task_id = -1;
		printf("[%d] Execution complete!\n", g_pid);
		sem_post(&(g_task_set->sem));
		// **** END critical sectin ****

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
	int err, n_tasks = -1;
	size_t task_queue_size = 5;
	off_t task_stack[MAX_PREEMPTABLE_TASKS];
	off_t task_stack_index = 0;

	// Check argument count
	if (argc != 2) {
		printf("%s [n-forks]\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Read number of forks
	n_tasks = atoi(argv[1]);

	printf("Process Count:\t\t\t%d\n", n_tasks);

	// Initialize shared memory
	if ((g_shm = map_shared_memory(
		shm_map_name,
		shm_map_size,
		true)) == NULL)
	{
		return EXIT_FAILURE;
	}

	printf("Shared Memory:\t\t\tReady\n");

	// Initialize static allocator
	g_allocator = install_static_allocator(g_shm, shm_map_size);

	printf("Static Allocator:\t\tReady\n");

	// Initialize task set
	g_task_set = make_task_set(n_tasks, task_queue_size, alloc, release);

	printf("Task Data Set:\t\t\tReady\n");


	// Fork some processes
	for (off_t i = 1; i < n_tasks; ++i) {
		if ((g_pid = fork()) == 0) {
			off_t task_id = i - 1;

			// Update self PID
			g_pid = getpid();

			// Update task information
			g_task_set->tasks[task_id].pid = g_pid;

			// Update task callback
			g_task_set->tasks[task_id].cb = task_callback;


			// Run the task procedure
			task_routine(task_id);

			// Should never reach here
			fprintf(stderr, "ILLEGAL POINT!\n");
			return EXIT_FAILURE;
		}
	}

	// Init network configuration
	// TODO

	do {
		off_t running_task_id = -1;
		char *dummy_data = "Foo";
		off_t task_select = -1;
		int prio_select = -1;
		printf("Select task to invoke: ");
		scanf("%ld", &task_select);

		printf("Assign callback priority (0-255): ");
		scanf("\n%d", &prio_select);
		prio_select = (prio_select + 0xFF) % 0xFF;

		printf("Okay, pushing cb for task %lu at prio %d\n", 
			task_select, prio_select);

		// **** Critical section ****
		sem_wait(&(g_task_set->sem));
		if ((err = enqueue_callback_for_task(task_select, prio_select,
			strlen(dummy_data) + 1, dummy_data, g_task_set)) != 0)
		{
			fprintf(stderr, "Err: Unable to enqueue task data (%d)\n",
				err);
		}

		// Find highest priority task
		int task_to_run = get_highest_prio_task_index(g_task_set);

		// Extract running task ID
		running_task_id = g_task_set->current_running_task_id;

		// Show information
		// printf("----- Task Set: Global -----\n");
		// show_task_set(g_task_set);
		// printf("----------------------------\n");
		// Print update
		printf("Suspending %ld, resuming %d\n",
			running_task_id, task_to_run);

		sem_post(&(g_task_set->sem));
		// **** END critical section ****

		// Signal current running task to stop if exists
		if (running_task_id != -1) {
			kill(g_task_set->tasks[running_task_id].pid, SIGSTOP);
			task_stack[task_stack_index++] = running_task_id;
		}

		// Signal task to run to run
		kill(g_task_set->tasks[task_to_run].pid, SIGCONT);


	} while (1);

	// Wait for child forks
	while ((pid = wait(&status)) > 0);


	// Destroy the task set
	if ((err = destroy_task_set(g_task_set)) != 0) {
		fprintf(stderr, "Unable to destroy task set!\n");
	}

end:
	// Remove shared memory
	if (unmap_shared_memory(
		shm_map_name,
		g_shm,
		shm_map_size,
		true) != 0)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}