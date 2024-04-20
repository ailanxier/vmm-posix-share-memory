#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 1024  // 1KB

int main() {
    int shm_fd;
    void *ptr;

    // 创建共享内存
    shm_fd = shyper_shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // 设置共享内存大小
    if (shyper_ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // 映射共享内存
    ptr = shyper_mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    printf("start: %p\n", ptr);

    // 使用共享内存
    // sprintf((char*)ptr, "Hello, shared memory!");

    printf("mmap success: %s\n", (char*)ptr);

    // // 取消映射共享内存
    // if (shyper_munmap(ptr, SHM_SIZE) == -1) {
    //     perror("munmap");
    //     exit(EXIT_FAILURE);
    // }

    // // 删除共享内存
    // if (shyper_unlink(SHM_NAME) == -1) {
    //     perror("unlink");
    //     exit(EXIT_FAILURE);
    // }

    printf("all success");
    return 0;
}