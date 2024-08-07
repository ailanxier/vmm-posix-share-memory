#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"
#include "../src/include/test_utils.h"
typedef int TYPE;

#define SHM_NAME "shyper"
TYPE shm_size;

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
    shm_size = (TYPE)size * KB;

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

    bzero(ptr, shm_size);

    register uint64 real_sum = 0, sum = 0;
    int total = shm_size / 4;
    int actual_wr_size = total * 4;
    TYPE* num = malloc(actual_wr_size);

    for(register int i = 0; i < total; i++){
        num[i] = rand() % 1000;
    }
    register int cnt = 0;

    register TYPE *wr_p = ptr;
    register TYPE *right = (TYPE*)((char *)ptr + actual_wr_size);
   
	while (wr_p + 8 <= right) {
#define	WRITE(i)	wr_p[i] = num[cnt++];
		WRITE(0) WRITE(1) WRITE(2) WRITE(3) WRITE(4) WRITE(5) WRITE(6)
		WRITE(7) 
		wr_p += 8;
	}

    register TYPE *rd_p = ptr;
    while (rd_p + 8 <= right) {
#define	READ(i)	rd_p[i]+
        sum += 
            READ(0) READ(1) READ(2) READ(3) READ(4) READ(5) READ(6) 
            rd_p[7];
        rd_p += 8;
    }

    for(register int i = 0; i < cnt; i++){
        real_sum += num[i];
    }
    uint64 diff = sum - real_sum;
    test_fprintf("diff = %llu, real_sum = %llu, sum = %llu\n", diff, real_sum, sum);
    if (diff != 0) {
        register TYPE *rd_p = ptr;
        cnt = 0;
        while (rd_p <= right) {
            if(*rd_p != num[cnt]){
                test_fprintf("Error: rd_p = %d, num[%d] = %d\n", *rd_p, cnt, num[cnt]);
                exit(1);
            }
            cnt++;
            rd_p += 4;
        }
    }
    test_fprintf("success");
    return 0;
}