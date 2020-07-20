#include "ros_inet.h"


/*
 *******************************************************************************
 *                            Interface Definitions                            *
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
		//fprintf(stderr, "%s:%d: Unable to read message!\n",
		//	__FILE__, __LINE__);
		return -1;
	}

	// Print received message
	//fprintf(stdout, "msg {.callback_id = %u, .callback_prio = %u, .data = %u}\n", 
	//	message[0], message[1], message[2]);

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

