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

int parameter_check(int argc, char *argv[]) {
    if (argc != 3) {
        test_fprintf("Usage: %s <rd|wr> <size>\n", argv[0]);
        test_fprintf("Example: %s rd 100 (read 100KB shared memory)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int is_read = 0;
    if (strcmp(argv[1], "rd") == 0) {
        is_read = 1;
    } else if (strcmp(argv[1], "wr") == 0) {
        is_read = 0;
    } else {
        test_fprintf("Usage: %s <rd|wr> <size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // shared memory size
    char *endptr;
    long int size = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        test_fprintf("Invalid size argument: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }    
    // shm_size is not allowed to exceed __INT_MAX__
    if(__INT_MAX__ / KB < size){
        test_fprintf("Size is too large: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    shm_size = size * KB;

    test_fprintf("shm_size = %ld KB\n", size);

    return is_read;
}


int main(int argc, char *argv[]) {
    int is_read = parameter_check(argc, argv);

    // create shared memory
    int shm_fd = shyper_shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    // set shared memory size
    shyper_ftruncate(shm_fd, shm_size);

    // map shared memory
    void *ptr = shyper_mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // test_fprintf("address of the shared memory mapping: %p\n", ptr);
    // int *rp_size = (int *)ptr;
    // shyper_set_ipa(rp_size[1]);

    if (is_read) {
        // read shared memory
        int *rp = (int *)ptr;
        test_fprintf("Read data[0] = %d\n", rp[0]);
        int cmd;
        while(scanf("%d", &cmd) != EOF){
            if(cmd == 0){
                break;
            }
            test_fprintf("Read data[0] = %d, va_start = %p\n", rp[0], ptr);
        }
    } else {
        // write shared memory
        int *wp = (int *)ptr;
        srand((unsigned int)time(NULL));
        wp[0] = rand();
        test_fprintf("Write data[0] = %d\n", wp[0]);
        int *rp = (int *)ptr;
        test_fprintf("Read data[0] = %d\n", rp[0]);
        // int *rp2 = (int *)ptr;
        // test_fprintf("Read len = %d\n", rp2[1]);
    }
    return 0;
}