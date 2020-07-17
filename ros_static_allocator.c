#include "ros_static_allocator.h"


static_allocator_t *install_static_allocator (uint8_t *static_memory, 
	size_t size)
{
	static_allocator_t static_allocator = {0};
	size_t min_mem_size = (sizeof(static_allocator_t) + 3 * sizeof(block_h));
	// NULL check
	if (static_memory == NULL) {
		return NULL;
	}

	// Check if given memory is large enough (static_allocator + three block)
	if (size < min_mem_size) {
		fprintf(stderr, "install_static_allocator: Requires at least %zu bytes\n",
			min_mem_size);
		return NULL;
	}

	// Set offets and sizes
	off_t useful_offset = sizeof(static_allocator_t);
	size_t useful_size  = size - useful_offset;
	size_t unit_size    = sizeof(block_h);

	// Remove head + init_block + spillover to get actual capacity
	size_t capacity = useful_size - (useful_size % unit_size) - 2 * unit_size;

	// Configure the static allocator
	static_allocator = (static_allocator_t) {
		.capacity         = capacity,
		.memory           = (static_memory + useful_offset),
		.free_list        = NULL,
		.free_memory_size = capacity,
		.unit_size        = unit_size
	};

	// Copy it into the memory
	memcpy((static_allocator_t *)static_memory, &static_allocator, 
		sizeof(static_allocator_t));

	return (static_allocator_t *)static_memory;
}


uint8_t *static_alloc (static_allocator_t *static_allocator, size_t size)
{
	block_h *last, *curr;

	// Parameter check
	if (static_allocator == NULL) {
		fprintf(stderr, "static_alloc: NULL allocator!\n");
		return NULL;
	}

	// Extract useful fields
	size_t unit_size = static_allocator->unit_size;
	size_t capacity  = static_allocator->capacity;
	size_t free_memory_size = static_allocator->free_memory_size;

	// Number of block sized units to allocate
	size_t nblocks = (size + unit_size - 1) / unit_size + 1;

	// If exceeds total capacity or is invalid: return 
	if (((nblocks * unit_size) > free_memory_size) || size == 0) {
		fprintf(stderr, "static_alloc: Either zero size or no more memory!\n");
		return NULL;
	}

	// If uninitialized: create initial list of units
	if ((last = static_allocator->free_list) == NULL) {
		block_h *head = (block_h *)(static_allocator->memory);
		head->d.size = 0;

		block_h *init = head + 1;
		init->d.size = free_memory_size / unit_size;
		init->d.next = static_allocator->free_list = last = head;

		head->d.next = init;
	}

	// Problem: Must not be permitted to merge with the init block

	// Look for free space; stop if wrap around happens
	for (curr = last->d.next; ; last = curr, curr = curr->d.next) {

		// If sufficient space available
		if (curr->d.size >= nblocks) {

			if (curr->d.size == nblocks) {
				last->d.next = curr->d.next;
			} else {
				curr->d.size -= nblocks;
				curr += curr->d.size;
				curr->d.size = nblocks;
			}

			// Reassign free list head
			static_allocator->free_list = last;

			// Update amount of free memory available
			static_allocator->free_memory_size -= nblocks * unit_size;

			return (uint8_t *)(curr + 1);
		}

		// Otherwise insufficient space. If at list head again, no memory left
		if (curr == static_allocator->free_list) {
			return NULL;
		}
	}
}



int static_free (static_allocator_t *static_allocator, uint8_t *block_ptr)
{
	block_h *b, *p;

	// Check if parameter is valid
	if (static_allocator == NULL || block_ptr == NULL) {
		fprintf(stderr, "static_free: NULL parameter!\n");
		return -1;
	}

	// Extract parameters
	uint8_t *memory  = static_allocator->memory;
	size_t unit_size = static_allocator->unit_size;
	size_t capacity  = static_allocator->capacity;

	// Check if memory in range
	if (!(block_ptr >= (memory + 2 * unit_size) &&
		  block_ptr < (memory + capacity + 2 * unit_size)))
	{
		fprintf(stderr, "static_free: Pointer out of memory range!\n");
		return -1;
	}

	// Obtain block header (block_ptr - unit_size)
	b = (block_h *)block_ptr - 1;

	// Update available memory size
	//printf("[Adding %zu bytes]\n", b->d.size * unit_size);
	static_allocator->free_memory_size += b->d.size * unit_size;

	// Find insertion location for block
	for (p = static_allocator->free_list; !(b >= p && b < p->d.next); p = p->d.next) {

		// If the block comes at the end of the list - break
		if (p >= p->d.next && b > p) {
			break;
		}

		// If at the end of the list, but block comes before next link
		if (p >= p->d.next && b < p->d.next) {
			break;
		}		
	}

	// [p] <----b----> [p->next] ----- [X]

	// Check if we can merge forwards
	if (b + b->d.size == p->d.next) {
		b->d.size += (p->d.next)->d.size;
		b->d.next = (p->d.next)->d.next;
	} else {
		b->d.next = p->d.next;
	}

	// Check if we can merge backwards
	if (p + p->d.size == b) {
		p->d.size += b->d.size;
		p->d.next = b->d.next;
	} else {
		p->d.next = b;
	}

	static_allocator->free_list = p;

	return 0;
}



void static_show (static_allocator_t *static_allocator)
{
	printf("Block Size: %zu\n", static_allocator->unit_size);
	printf("Capacity (bytes): %zu\n", static_allocator->capacity);
	printf("Unassigned memory (bytes): %zu\n", static_allocator->free_memory_size);

	if (static_allocator->free_list == NULL) {
		printf("<uninitialized>\n");
		return;
	}

	// Display all blocks
	block_h *p = static_allocator->free_list;
	do {

		// Choose sign
		char sign = '+';
		off_t dist = (off_t)(p->d.next - p);
		if (p->d.next < p) {
			sign = '-';
			dist = (off_t)(p - p->d.next);
		}

		printf("{%zu | bytes = %zu | dist = %c%zu } -> ", 
			(off_t)p,
			p->d.size * static_allocator->unit_size, 
			sign, dist);
		// printf("------------------------------------------------------------\n");
		// printf("Block Address: %zu\n", (off_t)p);
		// printf("Blocks (%ld bytes): %zu\n", static_allocator->unit_size, p->d.size);
		// printf("Next: %zu\n", (off_t)(p->d.next));
		// printf("{Next block is %zu bytes away. We claim next %zu bytes}\n",
		// 	(off_t)((p->d.next) - p), p->d.size * static_allocator->unit_size);
		// printf("------------------------------------------------------------\n");
		p = p->d.next;
	} while (p != static_allocator->free_list);
	putchar('\n');
}


bool static_is_unified (static_allocator_t *static_allocator)
{
	// Parameter check
	if (static_allocator == NULL) {
		fprintf(stderr, "static_is_unified: NULL parameter!\n");
		return false;
	}

	// Check memory
	if (static_allocator->free_list == NULL) {
		fprintf(stderr, "static_is_unified: free_list is NULL!\n");
		return false;
	}

	// Extract required field
	block_h *b = static_allocator->free_list;

	return ((b->d.next)->d.next == b);
}