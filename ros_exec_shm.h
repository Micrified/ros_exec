/*
 *******************************************************************************
 *                         (C) Copyright 2020 TU Delft                         *
 * Created: 07/07/2020                                                         *
 *                                                                             *
 * Programmer(s):                                                              *
 * - Charles Randolph                                                          *
 *                                                                             *
 * Description:                                                                *
 *  Header file for shared memory object infrastructure                        *
 *                                                                             *
 *******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

/*
 *******************************************************************************
 *                            Function Declarations                            *
 *******************************************************************************
*/


/*\
 * @brief Opens the POSIX shared memory map. Optionally creating if master
 * @note  Address chosen for the shared memory map is arbitrary
 * @param mmap_name     The name of the shared memory map
 * @param size          Size of shared memory map to open
 * @param is_owner      Whether the map should be created by caller
 * @return NULL on error (errno should be set automatically)
\*/
void *map_shared_memory (const char *mmap_name, size_t size, bool is_owner);

/*\
 * @brief Wrapper around munmap
 * @param mmap_name   The name of the shared memory map
 * @param shm_ptr     Pointer to shared memory map
 * @param size        Size of the shared memory map
 * @param is_owner    Whether the map should be freed by caller
 * @return Zero on success; otherwise error (visible in errno)
\*/
int unmap_shared_memory (const char *mmap_name, void *shm_ptr, size_t size,
 bool is_owner);
