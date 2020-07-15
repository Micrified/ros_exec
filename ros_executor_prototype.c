#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ros_exec_shm.h"


typedef struct {
	uint8_t prio;
	uint8_t data;
} task_data_t;

typedef struct {
	pid_t pid; 
	void (*cb) (void *data);

} task_t;

typedef struct shm_data {
	sem_t sem;
	int sum;
} shm_data;

int main (int argc, char *argv[])
{
	pid_t status = -1, pid = -1;
	const char *shm_map_name = "ros_exec_shm";
	size_t shm_map_size = 4096;
	void *shm_ptr = NULL;
	bool is_owner = true;
	shm_data *data_ptr = NULL;

	// Check argument count
	if (argc != 2) {
		printf("%s [n-forks]\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Read number of forks
	int n = atoi(argv[1]);

	// Initialize shared memory
	if ((shm_ptr = map_shared_memory(
		shm_map_name,
		shm_map_size,
		is_owner)) == NULL)
	{
		return EXIT_FAILURE;
	} else {

		// Set up data pointer
		data_ptr = (shm_data *)shm_ptr;

		// Initialize semaphore
		if (sem_init(&(data_ptr->sem), 1, 1) == -1) {
			perror("sem_init");
			return EXIT_FAILURE;
		}

		// Initialize value
		data_ptr->sum = 0;
	}

	printf("Shared memory created - forking!\n");

	// Fork some processes
	for (int i = 1; i < n; ++i) {
		if ((pid = fork()) == 0) {

			// Update self PID
			pid = getpid();

			// Print message
			printf("Hello from PID %d holding %i!\n", 
				getpid(), i);

			// Set as child
			is_owner = false;

			// Get shared semaphore
			sem_wait(&(data_ptr->sem));

			// Add value of 'i' to it
			data_ptr->sum += i;

			// Release shared semaphore
			sem_post(&(data_ptr->sem));

			// Jump to the end
			goto end;
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