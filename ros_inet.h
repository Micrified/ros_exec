#if !defined(ROS_INET_H)
#define ROS_INET_H

/*
 *******************************************************************************
 *                          (C) Copyright 2020 TUDelft                         *
 * Created: 17/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Simple networking interface                                                *
 *                                                                             *
 *******************************************************************************
*/

// Standard Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

// Networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// Multithreading/Polling
#include <poll.h>
#include <pthread.h>


/*
 *******************************************************************************
 *                             Symbolic Constants                              *
 *******************************************************************************
*/


// Maximum number of pollable file-descriptors (concurrent connections)
#define MAX_POLLABLE_FDS             8


/*
 *******************************************************************************
 *                           Interface Declarations                            *
 *******************************************************************************
*/


// Resets an array of pollable file-descriptors (must be safe first)
struct pollfd *get_new_pollable_fds ();

// Accepts a new connection to a listener socket
struct pollfd on_new_connection (int fd);

// Accepts a simple message from a pollable socket. Returns nonzero on error
int on_message (int fd, uint8_t *msg_buffer);

// Returns a socket bound to the given port. Returns -1 if cannot bind
int get_bound_socket (const char *port);


#endif