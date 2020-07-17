#include "ros_task_set.h"

/*
 *******************************************************************************
 *                        Internal Function Definitions                        *
 *******************************************************************************
*/


static task_callback_t *task_has_data (task_t *task_p)
{
	void *data_ptr = NULL;

	if (peek(&data_ptr, task_p->queue) != 0) {
		return NULL;
	} else {
		return (task_callback_t *)data_ptr;
	}
}

static void show_task_element (void * const element)
{
	task_callback_t *cb = (task_callback_t *)element;
	printf("{.prio = %d, {.data_size = %zu, .data_p = %p}",
		cb->prio, cb->callback_data->data_size, cb->callback_data->data_p); 
}

/*
 *******************************************************************************
 *                            Prototype Definitions                            *
 *******************************************************************************
*/


task_set_t *make_task_set (size_t len, size_t queue_depth,
 uint8_t *(*alloc)(size_t), void (*release)(uint8_t *))
{
	task_set_t *task_set_p = NULL;
	task_t *tasks = NULL;

	// Parameter check
	if (alloc == NULL || release == NULL) {
		return NULL;
	}

	// Allocate task set
	if ((task_set_p = (task_set_t *)alloc(sizeof(task_set_t))) == NULL) {
		return NULL;
	}

	// Allocate task array
	if ((tasks = (task_t *)alloc(len * sizeof(task_t))) == NULL) {
		return NULL;
	}

	// Configure all tasks to initial parameters
	for (off_t i = 0; i < len; ++i) {
		queue_t *queue_p = NULL;

		// Allocate the task queue
		if ((queue_p = make_queue(queue_depth, alloc, release)) == NULL) {
			return NULL;
		}

		tasks[i] = (task_t) {
			.pid = -1,
			.cb  = NULL,
			.queue = queue_p
		};
	}

	// Configure the task set
	task_set_p->len     = len;
	task_set_p->tasks   = tasks;
	task_set_p->alloc   = alloc;
	task_set_p->release = release;

	// Initialize the shared semaphore
	int pshared = 1; // Inter-process
	if (sem_init(&(task_set_p->sem), pshared, 1) == -1) {
		perror("sem_init");
	}

	return task_set_p;
}


int get_highest_prio_task_index (task_set_t *task_set_p)
{
	int prio_task_index = -1;
	task_callback_t *curr_data = NULL, *best_data = NULL;

	// Parameter check
	if (task_set_p == NULL) {
		fprintf(stderr, "%s:%d: get_highest_prio_task has NULL param!\n",
			__FILE__, __LINE__);
		return -1;
	}

	// Iterate across all tasks
	for (off_t i = 0; i < task_set_p->len; ++i) {
		task_t *task_p = task_set_p->tasks + i;

		// Don't consider tasks that don't have data
		if ((curr_data = task_has_data(task_p)) == NULL) {
			printf("Task %zu has no data -> skipping!\n",i);
			continue;
		}

		// Set task if none is set
		if (prio_task_index == -1) {
			printf("Setting Task %zu as default!\n", i);
			prio_task_index = i;
			continue;
		}

		// Compare current task to best
		if (peek((void **)&best_data, (task_set_p->tasks[prio_task_index]).queue) != 0) {
			fprintf(stderr, "%s:%d: Priority task has no data!\n",
				__FILE__, __LINE__);
			return -1;
		}

		if (curr_data->prio > best_data->prio) {
			printf("Task %zu has a higher prio for its next data element than %u\n",
				i, prio_task_index);
			prio_task_index = i;
		}
	}

	return prio_task_index;
}

int enqueue_callback_for_task (off_t task_id, uint8_t prio, size_t data_size, void *data, 
	task_set_t *task_set_p)
{
	task_callback_data_t *callback_data_p = NULL;
	task_callback_t *callback_p = NULL;
	void *data_copy = NULL;

	// Verify parameters
	if (data == NULL || task_set_p == NULL) {
		fprintf(stderr, "%s:%d: Null parameters!\n", __FILE__, __LINE__);
		return 1;
	}

	// Verify parameters
	if (task_id > task_set_p->len) {
		fprintf(stderr, "%s:%d: Task index is out of bounds (%u > %zu)\n",
			__FILE__, __LINE__, prio, task_set_p->len);
		return 2;
	}

	// Allocate a copy of the memory provided
	if ((data_copy = (void *)task_set_p->alloc(data_size)) == NULL) {
		fprintf(stderr, "%s:%d: Unable to allocate copy of memory!\n",
			__FILE__, __LINE__);
		return 3;
	} else {
		memcpy(data_copy, data, data_size);
	}

	// Allocate the callback data pointer
	if ((callback_data_p = 
		(task_callback_data_t *)task_set_p->alloc(sizeof(task_callback_data_t))) 
		== NULL) {
		fprintf(stderr, "%s:%d: Unable to allocate task callback data!\n",
			__FILE__, __LINE__);
		return 4;
	} else {
		*callback_data_p = (task_callback_data_t){
			.data_p = data_copy,
			.data_size = data_size
		};
	}

	// Allocate the callback data descriptor
	if ((callback_p = (task_callback_t *)task_set_p->alloc(sizeof(task_callback_t))) 
		== NULL) {
		fprintf(stderr, "%s:%d: Unable to allocate task callback descriptor!\n",
			__FILE__, __LINE__);
		return 5;
	} else {
		*callback_p = (task_callback_t){
			.prio = prio,
			.callback_data = callback_data_p
		};
	}

	// Enqueue this for the given task
	task_t *task = task_set_p->tasks + task_id;
	if (enqueue(callback_p, task->queue) != 0) {
		fprintf(stderr, "%s:%d: Unable to enqueue data with given task!\n",
			__FILE__, __LINE__);
		return 6;
	}

	return 0;
}

int dequeue_callback_for_task (off_t task_id, task_callback_t **task_callback_p_p,
	task_set_t *task_set_p)
{
	// Parameter check
	if (task_callback_p_p == NULL || task_set_p == NULL) {
		fprintf(stderr, "%s:%d: Null parameters!\n", __FILE__, __LINE__);
		return 1;
	}

	// Task ID check
	if (task_id > task_set_p->len) {
		fprintf(stderr, "%s:%d: Task ID is out of bounds (%lu > %zu)\n",
			 __FILE__, __LINE__, task_id, task_set_p->len);
		return 2;
	}

	// Locate the task
	task_t *task = task_set_p->tasks + task_id;

	// Dequeue
	if (dequeue((void **)task_callback_p_p, task->queue) != 0) {
		fprintf(stderr, "%s:%d: Unable to dequeue!\n", __FILE__, __LINE__);
		return 3;
	}

	return 0;
}

int free_task_callback (task_callback_t *callback_p, task_set_t *task_set_p)
{
	// Parameter check
	if (callback_p == NULL || task_set_p == NULL) {
		fprintf(stderr, "%s:%d: Null parameters!\n", __FILE__, __LINE__);
		return 1;
	}

	// (1) Free the copy of the data in the callback data subfield
	task_callback_data_t *callback_data_p = callback_p->callback_data;
	task_set_p->release((uint8_t *)(callback_data_p->data_p)); 

	// (2) Free the Free the callback data element
	task_set_p->release((uint8_t *)callback_data_p);

	// (3) Free the entire callback structure
	task_set_p->release((uint8_t *)callback_p);	
}

void show_task_set (task_set_t *task_set_p)
{
	if (task_set_p == NULL) {
		printf("<Null>\n");
		return;
	}

	printf("task_set_t {\n"\
		"\t.len = %zu\n"\
		"\t.queue_depth = %zu\n"\
		".tasks = {\n",
		task_set_p->len,
		task_set_p->queue_depth);

	for (off_t i = 0; i < task_set_p->len; ++i) {
		task_t *t = task_set_p->tasks + i;
		printf("\t[.pid = %d, .queue = {", t->pid);
		show_queue(t->queue, show_task_element);
		printf("}]");
	}

	printf("\t}\n}\n");
}

int destroy_task_set (task_set_t *task_set_p)
{
	int err = 0;

	// Parameter check
	if (task_set_p == NULL) {
		return 1;
	}

	// Destroy the shared semaphore
	if (sem_destroy(&(task_set_p->sem)) == -1) {
		perror("sem_destroy");
		return 2;
	}

	// First release the task set array (user must have freed task queues)
	for (off_t i = 0; i < task_set_p->len; ++i) {
		if (destroy_queue(task_set_p->tasks[i].queue) != 0) {
			fprintf(stderr, "%s:%d: Unable to free queue from task %zu\n",
				__FILE__, __LINE__, i);
		}
	}

	// Release the task array
	task_set_p->release((uint8_t *)task_set_p->tasks);

	// Release the task set itself
	task_set_p->release((uint8_t *)task_set_p);

	return err;
}
