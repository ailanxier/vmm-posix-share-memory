#ifndef __POSIX_SHMEM_H__
#define __POSIX_SHMEM_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>

#define MVM_READ_NAME  "mvm_read_shmem"
#define MVM_WRITE_NAME "mvm_write_shmem"
#define SHMEM_NAME_MAX_LEN 100
#define SHMEM_SIZE 4096
#define SHMEM_NUM 20
#define u64 unsigned long long
#define MAP_FAILED ((void *) -1)
#define NEXT(x) ((x + 1) % SHMEM_NUM)
#define WRITE   1
#define READ    0
#define ERROR_SHM_FULL  10
#define ERROR_SHM_EMPTY -1
extern char *shm_read_start[SHMEM_NUM];
extern char *shm_write_start[SHMEM_NUM];
extern char *shm_write_pointer[SHMEM_NUM];
extern char *shm_read_pointer[SHMEM_NUM];

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
    u64 size;
    u64 pid;
    int ret;
    u64 ipa;
};

struct shm_mmap {
    void *addr;
    u64 length;
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

struct shm_notify{
    char *name;
    int name_len;
};

int shyper_shm_open(const char *name, int oflag, mode_t mode);

int shyper_ftruncate(u64 fd, off_t length);

void *shyper_mmap(void *addr, size_t length, int prot, int flags, u64 fd, off_t offset);

int shyper_munmap(void *addr, size_t length);

int shyper_unlink(const char *name);

void *open_new_shmem(const char *name, int size);

void init_shmem(int test_cnt);

int set_shmem_data(int shm_id, const char *data, int size);

int get_shmem_data(int shm_id, char *data, int size);

int send_message(int len, const char *data);

int recv_message(char *data);

void close_shmem();

#ifdef TEST_TIME
    u64 get_recv_message_checksum();
    void record_test_result();
#endif

#endif