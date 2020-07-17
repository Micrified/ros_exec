#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Special allocator
#include "ros_static_allocator.h"

// Queue
#include "ros_queue.h"


// Static memory bank
uint8_t global_memory[4096];

// Pointer to static allocator
static_allocator_t *static_allocator_p = NULL;


uint8_t *alloc (size_t size)
{
	return static_alloc(static_allocator_p, size);
}

void release (uint8_t *ptr)
{
	static_free(static_allocator_p, ptr);
}

void show (void * const elem)
{
	char * const string_ptr = (char * const)elem;

	printf("[%p]::", elem);
	printf("\"%s\" -> ", string_ptr);
}


int main (void)
{
	void *elem_p = NULL;
	int err = 0;
	const char *strings[10] = {
		"Apple",
		"Pear",
		"Banana",
		"Orange",
		"Tulip",
		"Strawberry milkshake",
		"Augmented pillowcases",
		"Bamboozled spoons",
		"The dime on the table",
		"Friends afar"
	};

	// Initialize the static allocator
	static_allocator_p = install_static_allocator(global_memory, 4096);

	// Create a queue using the static allocator and some wrapper functions
	queue_t *queue_p = make_queue(10, alloc, release);


	// Print initial configuration
	printf("ptr = %zu, cap = %zu, len = %zu\n", queue_p->ptr,
		queue_p->cap, queue_p->len);

	// Enqueue a few strings
	for (int i = 0; i < 10; ++i) {

		if ((err = enqueue((void *)strings[i], queue_p)) != 0) {
			printf("Err: Couldn't enqueue (%d)\n", err);
			return EXIT_FAILURE;
		} else {
			printf("Enqueued [%p]: \"%s\"\n", strings[i], strings[i]);
		}

		// Show state
		show_queue(queue_p, show); putchar('\n');
	}

	// Begin dequeuing ... 
	printf("---- Now dequeuing! ----\n");


	// Dequeue a few 
	for (int i = 0; i < 3; ++i) {
		dequeue(&elem_p, queue_p);
		printf("Pointer: [%p\n] :: ", (char * const)elem_p);
		printf("Dequeued [%p] \"%s\"\n", (char * const)elem_p, (char * const)elem_p);
	}

	// Dequeue everything 
	printf("---- Enquing a bit ----\n");
	for (int i = 0; i < 2; ++i) {
		if ((err = enqueue((void *)strings[i], queue_p)) != 0) {
			printf("Err: Couldn't enqueue (%d)\n", err);
			return EXIT_FAILURE;
		} else {
			printf("Enqueued [%p]: \"%s\"\n", strings[i], strings[i]);
		}
	}

	// Begin dequeuing ... 
	printf("---- Now ending! ----\n");

	while (dequeue(&elem_p, queue_p) == 0) {
		printf("Pointer: [%p\n] :: ", (char * const)elem_p);
		printf("Dequeued [%p] \"%s\"\n", (char * const)elem_p, (char * const)elem_p);
	}

	// Show state
	show_queue(queue_p, show); putchar('\n');



	return EXIT_SUCCESS;
}