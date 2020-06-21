#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N_FORKS     3

int main (void)
{
	// Launch forks on messages
	bool master = false;
	int pid;
	int pids[N_FORKS];

	// Pause forks
	for (int i = 0; i < (N_FORKS - 1); ++i) {
		if ((pid = fork()) == 0) {
			// Am fork
			printf("Hello, am fork %d\n", i + 1);
		} else {
			master = true;
			pids[i] = pid;
		}
	}

	// Resume forks
	if (master) {
		for (int i = 0; i < (N_FORKS - 1); ++i) {
			wait(NULL);
		}
	}

	// Ect ect
	return EXIT_SUCCESS;
}