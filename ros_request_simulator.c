
// General
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// Custom
#include "ros_simulator_settings.h"

// Returns file-descriptor for a socket connected to given address and port
int get_connected_socket (const char *addr, const char *port)
{
	struct addrinfo hints, *res, *p;
	int s, stat;

	// Setup hints. 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Lookup connection options.
	if ((stat = getaddrinfo(addr, port, &hints, &res)) != 0) {
		fprintf(stderr, "%s:%d: Unable to get address information!\n",
			__FILE__, __LINE__);
		return -1;
	}

	// Bind to first suitable result.
	for (p = res; p != NULL; p = p->ai_next) {

		// Try initializing a socket.
		if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr, "%s:%d: Socket init attempt failed!\n", __FILE__, __LINE__);
			continue;
		}

		// Try connecting to the socket.
		if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
			continue;
		}

		break;
	}

	// Free linked-list of results.
	freeaddrinfo(res);

	// Return result.
	return ((p == NULL) ? -1 : s);
}


int main (void)
{
	int socket = -1;
	const long min_delay = 5000;
	const long max_delay = 100000;
	const long nano_range_max = 999999999;

	// Connection details
	const char *addr = "0.0.0.0", *port = "5577";

	// Attempt to connect socket
	if ((socket = get_connected_socket(addr, port)) == -1) {
		fprintf(stderr, "Unable to connect!\n");
		return EXIT_FAILURE;
	}

	// Dispatch message
	uint8_t message[3] = {0};

	for (off_t i = 0; i < 2; ++i) {

		// Build random sleep time
		struct timespec delay = (struct timespec) {
           .tv_sec = 0,
           .tv_nsec = (min_delay + rand() % max_delay) % nano_range_max
        };

        // Build random message
        message[0] = i; //rand() % 255 % MAX_TASK_COUNT; // ID
        message[1] = rand() % 255;                  // prio
        message[2] = rand() % 255;                  // data

        // Sleep 
        if (nanosleep(&delay, NULL) != 0) {
        	fprintf(stderr, "%s:%d: Notice - sleep interrupted!\n", __FILE__, __LINE__);
        }

		if (write(socket, message, 3 * sizeof(uint8_t)) != 3) {
			fprintf(stderr, "%s:%d: Unable to write to socket!\n", __FILE__, __LINE__);
		} else {
			fprintf(stdout, "Sent {%u, %u, %u}\n", message[0], message[1], message[2]);
		}		
	}


	// Close socket
	close(socket); 

	return EXIT_SUCCESS;
}