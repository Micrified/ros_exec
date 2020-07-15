#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "ros_static_allocator.h"


uint8_t buffer[4096];

int main (void)
{

	// Initialize the static memory allocator within the buffer
	static_allocator_t *static_allocator_p = install_static_allocator(buffer, 4096);

	// Check output
	if (static_allocator_p == NULL) {
		return EXIT_FAILURE;
	}

	// Seed random generator
	srand((unsigned)time(NULL));

	// Static pointer array
	void *pointers[4096];
	size_t memsize[4096] = {0};
	off_t pointer_index = 0;

	// Initialize the mode swap counter (how many time operation flops)
	int mode_swap_count = 4;


	while (mode_swap_count > 0) {
		uint8_t *ptr = NULL;

		// Check
		static_show(static_allocator_p);
		printf("\n\n++++++++++++++++++++++++++++++++++++++++++++\n\n\n");

		// If in mode 1: Allocate
		if (mode_swap_count % 2 == 0) {

			// Obtain a random size to allocate up to 512B
			size_t z = rand() % 512;

			// 
			size_t nblocks = ((z + static_allocator_p->unit_size - 1) / static_allocator_p->unit_size) + 1;

			printf("Allocating (%zu->%zu) bytes!\n", z, nblocks * static_allocator_p->unit_size);

			// Allocate the memory. 
			if (pointer_index >= 4096 || (z % 5) == 0 || (ptr = static_alloc(static_allocator_p, z)) == NULL) {
				mode_swap_count--;
				printf("Swapping modes (--> dealloc)\n");
				continue;
			}

			// Push back onto stack
			pointers[pointer_index] = ptr;
			memsize[pointer_index] = (nblocks  * static_allocator_p->unit_size);
			pointer_index++;
			
		} else {

			// If nothing left in pointer array, break
			if (pointer_index == 0) {
				mode_swap_count--;
				printf("Swapping modes (--> alloc)\n");
				continue;
			}

			// Get a random index to free
			off_t index_to_free = rand() % pointer_index;

			// Get pointer at that index
			ptr = pointers[index_to_free];

			// Show free
			printf("Freeing block of %zu bytes\n", memsize[index_to_free]);

			// Free
			if (static_free(static_allocator_p, ptr) != 0) {
				fprintf(stderr, "Error: Cannot free!?\n");
				return EXIT_FAILURE;
			}

			// Shuffle pointers and sizes
			for (off_t i = index_to_free + 1; i < pointer_index; ++i) {
				pointers[i - 1] = pointers[i];
				memsize[i - 1] = memsize[i];
			}

			// Update length
			pointer_index--;
		}
	}

	// Done
	printf("Shuffle finished!\n");

	// Release anything left
	for (off_t i = 0; i < pointer_index; ++i) {
		if (static_free(static_allocator_p, pointers[i]) != 0) {
			fprintf(stderr, "Error: Cannot free!?\n");
		}
	}

	// Show memory
	static_show(static_allocator_p);

	// Perform unification check
	assert(static_is_unified(static_allocator_p) == true);

	return EXIT_SUCCESS;
}