#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ros_task_set.h"
#include "ros_queue.h"
#include "ros_static_allocator.h"

#define MEM_SIZE 8192

// Memory (shared)
uint8_t g_memory[MEM_SIZE];

// Allocator 
static_allocator_t *g_allocator = NULL;

// Task set
task_set_t *g_task_set = NULL;



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

int main (void)
{
	size_t n_tasks = 5, queue_depth = 5;

	// Setup static allocator
	g_allocator = install_static_allocator(g_memory, MEM_SIZE);

	// Setup task set
	g_task_set = make_task_set(n_tasks, queue_depth, alloc, release);

	// Show task set
	show_task_set(g_task_set);

	// Accept user input
	char string_data[256];
	char selection = -1;
	int err, task_id = -1;
	task_callback_t *cb_p = NULL;
	do {
		printf("Specify task for operation: ");
		scanf("%d", &task_id);
		printf("\nInput captured: %d\n", task_id);
		if (task_id > n_tasks) {
			printf("\nThat task index is out of bounds!\n");
			continue;
		}

		printf("Press (s) to enqueue a string, or (d) to dequeue a string: ");
		scanf("\n%c", &selection);
		switch (selection) {
			case 's': {
				printf("\nOkay, enter your string: ");
				scanf("%s", string_data);
				printf("\nInput captured: \"%s\"\n", string_data);

				// Insert data for specified task
				if ((err = enqueue_callback_for_task(task_id, 1, strlen(string_data) + 1, 
					string_data, g_task_set)) != 0) {
					printf("\nError %d\n", err);
					continue;
				} else {
					printf("\nDone\n");
				}

				break;
			}
			case 'd': {
				if ((err = dequeue_callback_for_task(task_id, &cb_p, g_task_set)) != 0) {
					printf("\nError %d\n", err);
					continue;
				} else {
					char *string_data = (char *)cb_p->callback_data->data_p;
					printf("Extracted data: \"%s\" from queue of task %d\n", string_data, task_id);
					free_task_callback(cb_p, g_task_set);
				}
				break;
			}

			default: {
				printf("\nInput \'%c\' isn't valid!\n", selection);
				continue;
			}
		}

		// Print state of everything
		show_task_set(g_task_set);
		static_show(g_allocator);
		printf("-----------------------------------------------------------------\n");
	} while (1);


	// Destroy task set
	if (destroy_task_set(g_task_set) != 0) {
		fprintf(stderr, "Unable to free task set!\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}