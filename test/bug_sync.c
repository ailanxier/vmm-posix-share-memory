#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/include/posix_shmem.h"
#include "../src/include/test_utils.h"
char * msg;
int size = SHMEM_SIZE - sizeof(int) - sizeof(int);
int test_cnt;
u64 checksum = 0;

void generate_message(){
    checksum = 0;
    // int data_size = size - sizeof(u64);
    int data_size = size;
    for(int i = 0; i < data_size; i++){
        msg[i] = rand() % 256;
        // checksum = (checksum * 256 % MOD + msg[i]) % MOD;
    }
    // memcpy(msg + data_size, &checksum, sizeof(u64));
}

void change_message(){
    int cnt = rand() % 10;
    for(int i = 0; i < cnt; i++){
        int pos = rand() % size;
        msg[pos] = rand() % 256;
    }
}

int main() {
    srand((unsigned int)time(NULL));
    msg = (char *)malloc(size);
    init_shmem(0);
    u64 cnt = 0;
    generate_message();        
    while(1){
        change_message();
        send_message(size, msg);
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