/*
 *******************************************************************************
 *                         (C) Copyright 2020 TU Delft                         *
 * Created: 03/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Shared memory experiment. Compile with cc -lrt                             *
 *                                                                             *
 *******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main (void)
{
	const char *name = "ros_exec_map";
	int oflag = O_CREAT | O_RDWR | O_TRUNC;
	mode_t mode = 0;
	int err, fd = -1;
	size_t size = 1024;
	void *ptr = NULL;

	// Open POSIX shared memory map (initially length = 0)
	if ((fd = shm_open(name, oflag, mode)) == -1) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	// Set size using ftruncate
	if ((err = ftruncate(fd, size)) == -1) {
		perror("ftruncate");
		return EXIT_FAILURE;
	}

	// Verify size
	// TODO: stat

	// Map it 
	void *start_addr = NULL;
	int prot         = PROT_READ | PROT_WRITE;
	int flags        = MAP_SHARED;
	off_t offset     = 0x0;
	if ((ptr = mmap(start_addr, size, prot, flags, fd, offset)) == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	// Copy some dummy text in
	char *string_ptr = ptr;
	sprintf(string_ptr, "Hello there from %d!\n", getpid());

	// Wait for user input
	char t;
	printf("Press any key to terminate!\n");
	scanf("%c", &t);
	printf("Terminating ... \n");

	// Unmap it
	if ((err = munmap(ptr, size)) == -1) {
		perror("munmap");
		return EXIT_FAILURE;
	}

	// Unlink shared memory map
	if ((err = shm_unlink(name)) == -1) {
		perror("shm_unlink");
		return EXIT_FAILURE;
	}

	// Terminate program
	return EXIT_SUCCESS;
}