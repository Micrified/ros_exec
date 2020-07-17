#define _POSIX_SOURCE

// Standard Headers
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

// Networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// Multithreading/Polling
#include <poll.h>
#include <pthread.h>

// Data Structures
#include "ros_exec_shm.h"
#include "ros_queue.h"
#include "ros_task_set.h"
#include "ros_static_allocator.h"
#include "ros_inet.h"

/*
 *******************************************************************************
 *                             Symbolic Constants                              *
 *******************************************************************************
*/


// Capacity of the stack for preemptable tasks
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

	uint8_t *data_value_ptr = (uint8_t *)data;
	printf("[%d] (data = %d) Sum = %d\n", g_pid, *data_value_ptr, sum);
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


void scheduler (uint8_t *message)
{
	// Task Stack
	static off_t task_stack[MAX_PREEMPTABLE_TASKS];
	static off_t task_stack_index = 0;

	off_t running_task_id = -1;
	off_t highest_prio_task_id = -1;
	int err;


	// If message is NULL; Simply check if anything is ready to run 
	if (message == NULL) {

		// **** Critical Section ****
		sem_wait(&(g_task_set->sem));

		running_task_id = g_task_set->current_running_task_id;

		sem_post(&(g_task_set->sem));
		// **************************

		// If the task ID is not set; check the stack and resume anything on it
		if (running_task_id == -1 && task_stack_index > 0) {
			off_t task_to_resume = task_stack[task_stack_index - 1];
			printf("[%d] Resuming %ld\n", g_pid, task_to_resume);
			kill(g_task_set->tasks[task_to_resume].pid, SIGCONT);
			task_stack_index--;
		}

		return;
	}

	// Decode message
	uint8_t task_id   = message[0];
	uint8_t task_prio = message[1];
	uint8_t task_data = message[2];

	// **** Critical Section ****
	sem_wait(&(g_task_set->sem));

	// Push callback into shared queue
	if ((err = enqueue_callback_for_task(task_id, task_prio, 1, &task_data, 
		g_task_set)) != 0)
	{
		fprintf(stderr, "Err: Unable to enqueue task data (%d)\n",
			err);
	}

	// Locate highest priority task to run
	highest_prio_task_id = get_highest_prio_task_index(g_task_set);

	// Extract running task ID
	running_task_id = g_task_set->current_running_task_id;
	printf("[%d] Will pause %ld, run %ld\n", g_pid, running_task_id,
		highest_prio_task_id);
	sem_post(&(g_task_set->sem));
	// **************************

	// Signal current running task to stop if exists
	if (running_task_id != -1) {
		kill(g_task_set->tasks[running_task_id].pid, SIGSTOP);
		task_stack[task_stack_index++] = running_task_id;
	}

	// Signal highest priority task to run (if exists)
	if (highest_prio_task_id != -1) {
		kill(g_task_set->tasks[highest_prio_task_id].pid, SIGCONT);
	}
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

	// Networking
	off_t fd_index = 0;
	int sock_listen = -1;
	uint8_t message[3] = {0};

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


	// Get pollable file-descriptor set
	struct pollfd *fds = get_new_pollable_fds();

	// Init network configuration
	if ((sock_listen = get_bound_socket("5577")) == -1) {
		fprintf(stderr, "%s:%d: Listener socket could not be created!\n",
			__FILE__, __LINE__);
		goto end;
	} else {
		fprintf(stdout, "Created a listener socket!\n");
	}

	// Register the listener socket to the polling loop for reading (index 0)
	fds[fd_index++] = (struct pollfd) {
		.fd = sock_listen,
		.events = POLLIN, 
		.revents = 0
	};

	// Listen actively on the socket
	if (listen(sock_listen, 10) == -1) {
		fprintf(stderr, "%s:%d: Unable to listen on socket!\n", 
			__FILE__, __LINE__);
		goto end;
	} else {
		fprintf(stdout, "Listening on socket!\n");
	}

	// Poll loop (1ms before executes without waiting)
	while ((err = poll(fds,	fd_index, 0)) != -1) {

		// If nothing is ready then perform clean up
		if (err == 0) {
			scheduler(NULL);
			continue;
		}

		// Locate sockets with work to do
		for (off_t i = 0; i < fd_index; ++i) {

			// Skip those with no pending work
			if ((fds[i].revents & POLLIN) == 0) {
				continue;
			}

			// Accept connections if the listener socket
			if (i == 0) {
				if (fd_index >= MAX_POLLABLE_FDS) {
					fprintf(stderr, "%s:%d: At connection capacity!\n",
						__FILE__, __LINE__);
				} else {
					fds[fd_index++] = on_new_connection(fds[0].fd);					
				}
			} else {

				// Close connection on error and update table
				if (on_message(fds[i].fd, message) != 0) {
					fprintf(stdout, "Disconnecting client!\n");
					close(fds[i].fd);
					for (off_t j = i + 1; j < fd_index; ++j) {
						fds[j - 1] = fds[j];
					}
					fds[--fd_index].fd = -1;
				} else {
					scheduler(message);
				}
			}
		}
	}

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