#include "ros_exec_shm.h"

void *map_shared_memory (const char *mmap_name, size_t size, bool is_owner)
{
	void *shm_ptr = NULL, *start_addr = NULL;
	int fd = -1, oflag = O_RDWR; 
	int prot = PROT_READ | PROT_WRITE, flags = MAP_SHARED;
	mode_t mode = 0x0;
	off_t offset = 0x0;

	// Extend the oflags if owner
	if (is_owner) {
		oflag |= (O_CREAT | O_TRUNC);
	}

	// Open POSIX shared memory map
	if ((fd = shm_open(mmap_name, oflag, mode)) == -1) {
		perror("shm_open");
		return NULL;
	}

	// Set the size using ftruncate
	if (ftruncate(fd, size) == -1) {
		perror("ftruncate");
		return NULL;
	}

	// Map in the memory
	if ((shm_ptr = mmap(start_addr, size, prot, flags, fd, offset)) 
		== MAP_FAILED)
	{
		perror("mmap");
		return NULL;
	}

	return shm_ptr;
}


int unmap_shared_memory (const char *name, void *shm_ptr, size_t size, bool is_owner)
{
	int err = 0;

	// Unmap the memory
	if ((err = munmap(shm_ptr, size)) == -1) {
		perror("munmap");
		return -1;
	}

	// If not the owner - return early
	if (!is_owner) {
		return err;
	}

	// Unlink if the owner
	if ((err = shm_unlink(name)) == -1) {
		perror("shm_unlink");
		return err;
	}

	return err;
}