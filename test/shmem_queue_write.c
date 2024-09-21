#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"
#include "../src/include/test_utils.h"

int main(int argc, char *argv[]) {
    init_shmem();
    srand((unsigned int)time(NULL));
    register uint64 write_sum = 0, read_sum = 0;
    int total = SHMEM_SIZE / sizeof(int);
    int actual_wr_size = total * sizeof(int);
    int* num = malloc(actual_wr_size);
    for (int i = 0; i < SHMEM_NUM; i++) {
        for(register int j = 0; j < total; j++)
            num[j] = rand() % 1000;
        register int cnt = 0;
        register char *right = ((char *)shm_send_start[i] + actual_wr_size);
    
        while (shm_send_pointer[i] + sizeof(int) <= right) {
            set_shmem_data(i, (char *)&num[cnt], sizeof(int));
            write_sum += num[cnt++];
        }
    }
    free(num);
    test_fprintf("MVM write sum = %llu\n", write_sum);

    int read_flag, temp;
    scanf("%d", &read_flag);
    for (int i = 0; i < SHMEM_NUM; i++) {
        register char *right = (char *)shm_recv_start[i] + actual_wr_size;
        while (shm_recv_pointer[i] + sizeof(int) <= right) {
            get_shmem_data(i, (char *)&temp, sizeof(int));
            read_sum += temp;
        }
    }
    test_fprintf("MVM read sum = %llu\n", read_sum);
    return 0;
}