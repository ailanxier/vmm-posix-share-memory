#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"
#include "../src/include/test_utils.h"

#define SHM_NAME "shyper"
int shm_size;

void parameter_check(int argc, char *argv[]) {
    if (argc != 2) {
        test_fprintf("Usage: %s <rd|wr> <size>\n", argv[0]);
        test_fprintf("Example: %s rd 100 (read 100KB shared memory)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // shared memory size
    char *endptr;
    long int size = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        test_fprintf("Invalid size argument: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }    
    // shm_size is not allowed to exceed __INT_MAX__
    if(__INT_MAX__ / KB < size){
        test_fprintf("Size is too large: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    shm_size = size * KB;

    test_fprintf("shm_size = %ld KB\n", size);
}


int main(int argc, char *argv[]) {
    parameter_check(argc, argv);
    
    srand((unsigned int)time(NULL));
    // create shared memory
    int shm_fd = shyper_shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    // set shared memory size
    shyper_ftruncate(shm_fd, shm_size);

    // map shared memory
    void *ptr = shyper_mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    test_fprintf("address of the shared memory mapping: %p\n", ptr);

    register uint64 real_sum = 0, sum = 0;
    int total = shm_size / 4;
    int actual_wr_size = total * 4;

    register int *right = (int*)((char *)ptr + actual_wr_size);

    register int *rd_p = ptr;
    while (rd_p + 8 <= right) {
#define	READ(i)	rd_p[i]+
        sum += 
            READ(0) READ(1) READ(2) READ(3) READ(4) READ(5) READ(6) 
            rd_p[7];
        rd_p += 8;
    }

    test_fprintf("GVM read sum = %llu\n", sum);
    return 0;
}