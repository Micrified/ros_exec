
// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Polling/Multithreading functionality
#include <poll.h>
#include <pthread.h> // Link -lpthread


// Networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>


/*
 *******************************************************************************
 *                             Symbolic Constants                              *
 *******************************************************************************
*/

// Maximum number of tasks
#define MAX_TASK_COUNT          4

// Depth of a callback queue
#define TASK_MSG_QUEUE_DEPTH    16

// Maximum supported of pollable file-descriptors
#define MAX_POLLABLE_FDS        8


/*
 *******************************************************************************
 *                              Type Definitions                               *
 *******************************************************************************
*/

// Type describing a message
typedef struct {
	uint8_t prio;
	uint8_t data;
} msg_t;

// Type describing a task
typedef struct {
	void (*callback)(void *msg);
	off_t queue_index;
	msg_t queue[TASK_MSG_QUEUE_DEPTH];
} task_t;

/*
 *******************************************************************************
 *                              Global Variables                               *
 *******************************************************************************
*/

// Pointer to currently active thread

// 
/*
 *******************************************************************************
 *                              Utility Routines                               *
 *******************************************************************************
*/

// Resets an array of pollable file-descriptors (must be safe first)
struct pollfd *get_new_pollable_fds ()
{
	static struct pollfd pollable_fds[MAX_POLLABLE_FDS];
	for (off_t i = 0; i < MAX_POLLABLE_FDS; ++i) {
		pollable_fds[i] = (struct pollfd) { .fd = -1, .events = 0, .revents = 0};
	}
	return pollable_fds;
}

// Accepts a new connection to a listener socket
struct pollfd on_new_connection (int fd)
{
	struct sockaddr_storage new_addr;
	socklen_t new_addr_size = sizeof(new_addr);
	int sock_new = -1;

	// Try to accept connection
	if ((sock_new = accept(fd, (struct sockaddr *)&new_addr, &new_addr_size))
		== -1) {
		fprintf(stderr, "%s:%d: Attempt to accept connection failed!\n", 
			__FILE__, __LINE__);
	} else {
		fprintf(stdout, "Accepted a new connection!\n");
	}

	return (struct pollfd) { .fd = sock_new, .events = POLLIN, .revents = 0};
}

// Accepts a simple message from a pollable socket. Returns nonzero on error
int on_message (int fd, uint8_t *msg_buffer)
{
	uint8_t message[3] = {0};

	// Expect: Message
	if (read(fd, message, 3 * sizeof(uint8_t)) != 3) {
		fprintf(stderr, "%s:%d: Unable to read message!\n",
			__FILE__, __LINE__);
		return -1;
	}

	// Print received message
	fprintf(stdout, "msg {.callback_id = %u, .callback_prio = %u, .data = %u}\n", 
		message[0], message[1], message[2]);

	// Copy to message buffer if available
	if (msg_buffer != NULL) {
		memcpy(msg_buffer, message, 3 * sizeof(uint8_t));
	}

	return 0;
}

// Returns a socket bound to the given port. Returns -1 if cannot bind
int get_bound_socket (const char *port)
{
	int flags    = AI_PASSIVE;
	int family   = AF_UNSPEC;
	int socktype = SOCK_STREAM;
	struct addrinfo hints, *res, *p;
	int s, stat, y = 1;

	// Configure hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags    = flags;
	hints.ai_family   = family;
	hints.ai_socktype = socktype;

	// Lookup connection options
	if ((stat = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "%s:%d: Problem looking for connection options!\n",
			__FILE__, __LINE__);
		return -1;
	}

	// Bind to first suitable result
	for (p = res; p != NULL; p = p->ai_next) {

		// Try to initialize a socket
		if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr, "%s:%d: Warning: Socket init failed on " \
				" getaddrinfo result!\n", __FILE__, __LINE__);
			continue;
		}

		// Try to re-use the port
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) == -1) {
			fprintf(stderr, "%s:%d: Couldn't reuse port %s!\n",
				__FILE__, __LINE__, port);
			continue;
		}

		// Try binding to the socket. Continue if not possible
		if (bind(s, p->ai_addr, p->ai_addrlen) == -1) {
			continue;
		}

		break;
	}

	// Free linked-list of results
	freeaddrinfo(res);

	// Return result
	return ((p == NULL) ? -1 : s);
}



/*
 *******************************************************************************
 *                             Scheduler Routines                              *
 *******************************************************************************
*/


// Next callback to run
int scheduler (uint8_t *message, task_t *tasks)
{
	uint8_t id = message[0], prio = message[1], data = message[2];

	// Validate the message ID
	if (id > MAX_TASK_COUNT) {
		fprintf(stderr, "%s:%d: Invalid task identifier!\n", __FILE__, __LINE__);
		return -1;
	}

	// Push message data onto queue
	if (tasks[id].queue_index < TASK_MSG_QUEUE_DEPTH) {
		tasks[id].queue[tasks[id].queue_index++] = (msg_t){.prio = prio, .data = data};
	} else {
		fprintf(stderr, "%s:%d: Task %d ran out of queue space!\n",
			__FILE__, __LINE__, id);
	}

	// Find the highest priority task to run (lowest number is highest prio)
	off_t highest_prio_task_index = -1;
	for (off_t i = 0; i < MAX_TASK_COUNT; ++i) {

		// Skip tasks with no pending data
		if (tasks[i].queue_index == 0) {
			continue;
		}

		// If no best task yet, set this one
		if (highest_prio_task_index == -1) {
			highest_prio_task_index = i;
			continue;
		}

		// Otherwise: Compare priority of latest message to that of current best task
		if (tasks[i].queue[0].prio > tasks[highest_prio_task_index].queue[0].prio) {
			highest_prio_task_index = i;
		}
	}

	// Return chosen task
	return highest_prio_task_index;
}

// Context switching
void context_switch (int task_id, task_t *tasks)
{
	printf("Context-switch (%d, tasks)\n", task_id);
}

void generic_callback (void *msg)
{

}


/*
 *******************************************************************************
 *                              PThread Routines                               *
 *******************************************************************************
*/


int main (void)
{
	int err;
	off_t fd_index = 0;
	int sock_listen = -1;
	uint8_t message[3] = {0};
	task_t tasks[MAX_TASK_COUNT] = {0};

	// Obtain the file-descriptor set
	struct pollfd *fds = get_new_pollable_fds();

	// Configure callbacks
	for (off_t i = 0; i < MAX_TASK_COUNT; ++i) {
		tasks[i] = (task_t){
			.callback = generic_callback, 
			.queue_index = 0, 
			.queue = {{0,0}}
		};		
	}

	// Start the listener socket using 4290
	if ((sock_listen = get_bound_socket("4290")) == -1) {
		fprintf(stderr, "%s:%d: Listener socket could not be created!\n",
			__FILE__, __LINE__);
		goto exit;
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
		goto exit;
	} else {
		fprintf(stdout, "Listening on socket!\n");
	}

	// Poll loop (1ms before executes without waiting)
	while ((err = poll(fds,	fd_index, 1)) != -1) {

		// If nothing is ready then perform clean up
		if (err == 0) {
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
					context_switch(scheduler(message, tasks), tasks);
				}
			}
		}
	}

	// Exit of loop necessitates polling error
	fprintf(stderr, "%s:%d: Polling error (%s)\n", __FILE__, 
		__LINE__, strerror(errno));
exit:
	return EXIT_FAILURE;
}