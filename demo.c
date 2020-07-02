#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <math.h>

#define SHARE_THREADS   0
#define SHARE_PROCESSES 1

sem_t g_mutex;


int g_pid = -1;


// Returns zero if the natural number is not prime. Otherwise returns one.
static int isPrime (unsigned int p) {
    int i, root;

    if (p == 1) return 0;
    if (p == 2) return 1;
    if ((p % 2)  == 0) return 0;

    root = (int)(1.0 + sqrt(p));

    for (i = 3; (i < root) && (p % i != 0); i += 2);
    return i < root ? 0 : 1;
}

static void *routine (void *arg)
{
	// Set the global PID (access controlled)
	sem_wait(&g_mutex);
	g_pid = getpid();
	printf("Updated GID to %d!\n", g_pid);
	sem_post(&g_mutex);

	// Compute number of primes.
	unsigned int sum = 0;
    for (unsigned int i = 0; i < 50000000; i++) {
    	if ((i % 10000000) == 0) {
    		printf("%d%%\n", (i / 10000000) * 25);
    	}
        sum += isPrime(i);
    }

	printf("Routine: Done!\n");

	// Set the global PID (access controlled)
	sem_wait(&g_mutex);
	g_pid = -1;
	printf("Updated GID to %d!\n", g_pid);
	sem_post(&g_mutex);

	return NULL;
}

void signal_pause_thread (pthread_t *pthread_p)
{
	// SIGSTOP
	sem_wait(&g_mutex);
	if (g_pid != -1) {
		pthread_suspend_np(*pthread_p);
		// if (kill(g_pid, SIGSTOP) == -1) {
		// 	perror("kill(sigstop)");
		// 	exit(EXIT_FAILURE);
		// }
	}
	sem_post(&g_mutex);
}

void signal_resume_thread (pthread_t *pthread_p)
{
	// SIGCONT
	sem_wait(&g_mutex);
	if (g_pid != -1) {
		pthread_continue_np(*pthread_p);
		// if (kill(g_pid, SIGCONT) == -1) {
		// 	perror("kill(sigcont)");
		// 	exit(EXIT_FAILURE);
		// }
	}
	sem_post(&g_mutex);
}


int main (void)
{
	int err;
	pthread_attr_t attr;
	pthread_t thread_id;
	void *outcome;

	// Init the mutex
	sem_init(&g_mutex, SHARE_PROCESSES, 1);

	// Init attributes
	if ((err = pthread_attr_init(&attr)) != 0) {
		errno = err;
		perror("pthread_attr_init");
		return EXIT_FAILURE;
	}

	// Init thread properties
	// if ((err = pthread_attr_setstacksize(&attr, 1024)) != 0) {
	// 	errno = err;
	// 	perror("pthread_attr_setstacksize");
	// 	return EXIT_FAILURE;
	// }

	// Main
	// printf("Main PID = %d!\n", getpid());

	// if ((g_pid = fork()) == 0) {
	// 	routine(NULL);
	// 	goto end;
	// }

	// // Launch thread
	if ((err = pthread_create(&thread_id, &attr, &routine, NULL)) != 0) {
		errno = err;
		perror("pthread_create");
		return EXIT_FAILURE;
	}

	// Destroy attribute
	if ((err = pthread_attr_destroy(&attr)) != 0) {
		errno = err;
		perror("pthread_attr_destroy");
		return EXIT_FAILURE;
	}

	// Pause thread on user input
	int n, done = 0;
	while (done == 0) {
		printf("Press (1) to pause, (2) to continue, and anything else to exit!\n");
		scanf("%d", &n);
		switch (n) {
			case 1: {
				signal_pause_thread(&thread_id);
				printf("Signalled to pause ... \n");
			}
			break;
			case 2: {
				signal_resume_thread(&thread_id);
				printf("Signalled to resume ... \n");
			}
			break;
			default: {
				done = 1;
			}
		}
	}

	// Join threads (block until returns)
	printf("Main: Waiting ...\n");
	// sem_wait(&g_mutex);
	// int copy = g_pid;
	// sem_post(&g_mutex);
	// if (copy != -1) {
	// 	waitpid(copy, NULL, 0);
	// }
	if ((err = pthread_join(thread_id, &outcome)) != 0) {
		errno = err;
		perror("pthread_join");
		return EXIT_FAILURE;
	}
	printf("Main: Done!\n");

end:
	return EXIT_SUCCESS;
}