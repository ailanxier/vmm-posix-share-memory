#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"
#include "../src/include/test_utils.h"
char * msg;
int size = SHMEM_SIZE - sizeof(int) - sizeof(char);
int test_cnt;
u64 checksum = 0, mod = 1e9 + 7;

void generate_message(){
    checksum = 0;
    for(int i = 0; i < size; i++){
        msg[i] = 'a' + rand() % 26;
        // checksum = (checksum * 256 % mod + msg[i]) % mod;
    }
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    msg = (char *)malloc(size);
    test_cnt = atoi(argv[1]);
    init_shmem(test_cnt);
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 1000000; // 1 ms
    
    generate_message();
    while(test_cnt--){
        send_message(size, msg);
        // while (nanosleep(&req, &rem) == -1) {
        //     req = rem; // 信号处理程序返回时继续睡眠，保证至少隔 1ms 
        // }
    }
    // record_test_result();
    // close_shmem();
    // u64 recv_checksum = get_recv_message_checksum();
    // if (recv_checksum == checksum) {
    //     test_fprintf("Checksum is correct\n");
    // } else {
    //     test_fprintf("Checksum is incorrect\n");
    // }
    return 0;
}