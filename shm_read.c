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
	int oflag = O_RDONLY;
	mode_t mode = 0;
	int err, fd = -1;
	size_t size = 1024;
	void *ptr = NULL;

	// Open POSIX shared memory map (initially length = 0)
	if ((fd = shm_open(name, oflag, mode)) == -1) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	// Get size
	struct stat sb;
	fstat(fd, &sb);
	size = sb.st_size;
	printf("Found the shared memory map with a size of %zu\n", size);

	// Map it 
	void *start_addr = NULL;
	int prot         = PROT_READ;
	int flags        = MAP_SHARED;
	off_t offset     = 0x0;
	if ((ptr = mmap(start_addr, size, prot, flags, fd, offset)) == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	// Print the text in it
	printf("Text = \"%s\"\n", (char *)ptr);

	// Unmap it
	if ((err = munmap(ptr, size)) == -1) {
		perror("munmap");
		return EXIT_FAILURE;
	}

	// Don't unlink (don't own it)

	// Terminate program
	return EXIT_SUCCESS;
}