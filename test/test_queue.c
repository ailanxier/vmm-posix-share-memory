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
    while(1){
        char cmd[100];
        char msg[100];
        scanf("%s %s", cmd, msg);
        if(strcmp(cmd, "exit") == 0){
			break;
		}else if(strcmp(cmd, "rd") == 0){
			if(recv_message(msg) == ERROR_SHM_EMPTY){
                test_fprintf("Failed to receive message\n");
            }else{
                test_fprintf("MVM received message: %s\n", msg);
            }
		}else if(strcmp(cmd, "wr") == 0){
			if(send_message(strlen(msg), msg) == ERROR_SHM_FULL){
                test_fprintf("Failed to send message\n");
            }else{
                // This printing may interfere with the timing of the signal handler, causing the measurement time to increase.
                // test_fprintf("MVM sent message: %s\n", msg);
            }
		}else{
			test_fprintf("Invalid command: %s\n", cmd);
			continue;
		}
    }
    return 0;
}