#ifndef __POSIX_SHMEM_H__
#define __POSIX_SHMEM_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define u64 unsigned long long
#define SHM_NAME_MAX_LEN 256
#define MAP_FAILED ((void *) -1)

struct shm_name {
    char *name;
    int name_len;
    u64 fd;
    int oflag;
    mode_t mode;
    u64 ipa;
};

struct shm_size {
    u64 shm_fd;
    size_t size;
    u64 pid;
    int ret;
    u64 ipa;
};

struct shm_mmap {
    void *addr;
    size_t length;
    int prot;
    int flags;
    u64 fd;
    off_t offset;
    void *ret;
    u64 ipa;
};

struct shm_unmap {
    void *addr;
    size_t length;
    int ret;
    u64 ipa;
};

struct shm_unlink {
    char *name;
    int name_len;
    int ret;
    u64 ipa;
};

int shyper_shm_open(const char *name, int oflag, mode_t mode);

int shyper_ftruncate(u64 fd, off_t length);

void *shyper_mmap(void *addr, size_t length, int prot, int flags, u64 fd, off_t offset);

int shyper_munmap(void *addr, size_t length);

int shyper_unlink(const char *name);

#endif