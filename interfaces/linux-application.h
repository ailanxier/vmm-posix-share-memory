#include <stdio.h>
#include <stdlib.h>

#define MAP_FAILED ((void *) -1)

// create a new shared memory object
// return: file descriptor
// return -1 stand for shm_open failed
int shm_open(const char *__name, int __oflag, mode_t __mode);

// set the size of a shared memory object
// return: 0 if success, -1 if failed
int ftruncate(int __fd, off_t __length) noexcept(true);

// map files or memory objects into share memory
// return: the starting address of the mapped region
// return MAP_FAILED if failed
void *mmap(void *__addr, size_t __len, int __prot, int __flags, int __fd, off_t __offset) noexcept(true);

// unmap files or memory objects from share memory
// return: 0 if success, -1 if failed
int munmap(void *__addr, size_t __len) noexcept(true);

// remove a shared memory object
// return: 0 if success, -1 if failed
int shm_unlink(const char *__name);

// close a shared memory object
// return: 0 if success, -1 if failed
int shm_close(int __fd) noexcept(true);
