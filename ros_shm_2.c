#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ros_exec_shm.h"


int main (void)
{
	const char *name = "ros_exec_map";
	void *ptr = NULL;
	size_t size = 1024;

	// Get the shared memory pointer
	if ((ptr = map_shared_memory(name, size, true)) == NULL) {
		return EXIT_FAILURE;
	}

	// Write to the pointer
	char *string = (char *)ptr;
	sprintf(ptr, "Hello from %d\n", getpid());

	// Pause and ask for input
	printf("Press any key to continue: ");
	char c;
	scanf("%c", &c);

	// Destroy the pointer
	if (unmap_shared_memory(name, ptr, size, true) != 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}