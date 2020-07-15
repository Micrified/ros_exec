#include "ros_queue.h"


queue_t *make_queue (size_t capacity, uint8_t *(*alloc)(size_t), 
	void (*release)(uint8_t *))
{
	queue_t *queue_p = NULL;
	void **array = NULL;

	// Parameter check
	if (alloc == NULL || release == NULL) {
		return NULL;
	}

	// Allocate queue instance
	if ((queue_p = (queue_t *)alloc(sizeof(queue_t))) == NULL) {
		return NULL;
	}

	// Allocate the queue of pointers itself
	if ((array = (void *)alloc(capacity * sizeof(void *))) == NULL) {
		return NULL;
	}

	// Configure the queue
	(*queue_p) = (queue_t) {
		.array     = array,
		.ptr       = 0,
		.len       = 0,
		.cap       = capacity,
		.alloc     = alloc,
		.release   = release
	};

	return queue_p;
}


int enqueue (void *elem_p, queue_t *queue_p)
{

	// Parameter check
	if (queue_p == NULL || elem_p == NULL) {
		return 1;
	}

	// Capacity check
	if (queue_p->len >= queue_p->cap) {
		return 2;
	}

	// Insert element
	queue_p->array[queue_p->ptr] = elem_p;

	// Update pointer
	queue_p->ptr = (queue_p->ptr + 1) % queue_p->cap;

	// Update length
	queue_p->len++;

	return 0;
}


int dequeue (void **elem_p_p, queue_t *queue_p)
{

	// Parameter check
	if (queue_p == NULL || elem_p_p == NULL) {
		return 1;
	}

	// Capacity check
	if (queue_p->len <= 0) {
		return 2;
	}

	// Compute dequeue index
	int i = queue_p->ptr, len = queue_p->len, cap = queue_p->cap;
	int index = ((i - len) % cap + cap) % cap;

	// Copy element out
	*elem_p_p = queue_p->array[index];

	// Update length
	queue_p->len--;

	return 0;
}


int destroy_queue (queue_t *queue_p)
{
	// Parameter check
	if (queue_p == NULL) {
		return 1;
	}

	// Free the array
	if (queue_p->array != NULL) {
		queue_p->release((uint8_t *)queue_p->array);
	}

	// Free the queue itself
	queue_p->release((uint8_t *)queue_p);

	return 0;
}


void show_queue (queue_t *queue_p, void (*show)(void * const elem))
{

	// Parameter check
	if (queue_p == NULL || show == NULL) {
		printf("Nil\n");
		return;
	}

	// Compute head of queue
	off_t head = 
	(off_t)(((int)queue_p->ptr - (int)queue_p->len) % (int)queue_p->cap);

	// Print queue contents
	for (off_t i = 0; i < queue_p->len; ++i) {
		off_t index = (head + i) % queue_p->cap;
		show(queue_p->array[index]);
	}
}