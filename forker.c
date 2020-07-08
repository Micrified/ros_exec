#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (void)
{
	pid_t pid;
	int status, N = 3;

	// Fork
	for (int i = 1; i < N; ++i) {
		if ((pid = fork()) == 0) {
			printf("Child: %d\n", getpid());
			sleep(i);
			return EXIT_SUCCESS;
		}
	}
	printf("Parent: %d\n", getpid());

	// Wait for child processes
	while ((pid = wait(&status)) > 0);

	// Print final success value
	printf("Parent: Done!\n");

	return EXIT_SUCCESS;
}