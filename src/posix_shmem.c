#include "include/posix_shmem.h"
#include "include/test_utils.h"

char *shm_send_start[SHMEM_NUM];
char *shm_recv_start[SHMEM_NUM];
char *shm_send_pointer[SHMEM_NUM];
char *shm_recv_pointer[SHMEM_NUM];
char temp_msg[SHMEM_SIZE], shmem_name[SHMEM_NAME_MAX_LEN];
int *ack_send_start, *ack_recv_start;
int shm_send_cur_id, shm_recv_cur_id, send_seq_num, recv_ack_num, recv_seq_num;
int shyper_fd, mem_fd;

#ifdef TEST_TIME
    int pid, send_start_cur, send_end_cur, signal_cur, recv_end_cur;
    u64* user_send_start, * user_send_end, * user_signal, * user_recv_end;
#endif

int shyper_shm_open(const char *name, int oflag, mode_t mode) {
    u64 fd;
    struct shm_name shm;

    shm.name = name;
    shm.name_len = strlen(name);

    ioctl(shyper_fd, 0x1302, &shm);

    fd = shm.fd;
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int shyper_ftruncate(u64 fd, off_t length) {
    int ret;
    struct shm_size size;

    size.shm_fd = fd;
    size.size = length;
    size.pid = getpid();

    ioctl(shyper_fd, 0x1303, &size);

    ret = size.ret;
    if (ret == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    return ret;
}

void *shyper_mmap(void *addr, size_t length, int prot, int flags, u64 fd, off_t offset) {
    struct shm_mmap shmmap;
    int ipa_offset;

    shmmap.addr = addr;
    shmmap.length = length;
    shmmap.prot = prot;
    shmmap.flags = flags;
    shmmap.fd = fd;
    shmmap.offset = offset;

    ioctl(shyper_fd, 0x1304, &shmmap);
    ipa_offset = shmmap.offset;

    void *va = mmap(NULL, shmmap.length, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ipa_offset);
    if (shmmap.ret == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return va;
    // return NULL;
}

int shyper_munmap(void *addr, size_t length) {
    int ret;
    struct shm_unmap unmap;

    unmap.addr = addr;
    unmap.length = length;

    ioctl(shyper_fd, 0x1305, &unmap);

    ret = unmap.ret;
    if (ret == -1) {
        perror("munmap");
        return -1;
    }
    return ret;
}

int shyper_unlink(const char *name) {
    int ret;
    struct shm_unlink unlink;

    unlink.name = name;
    unlink.name_len = strlen(name);

    ioctl(shyper_fd, 0x1306, &unlink);

    ret = unlink.ret;
    if (ret == -1) {
        perror("shm_unlink");
        return -1;
    }
    return ret;
}

void *open_new_shmem(const char *name, int size) {
    int fd = shyper_shm_open(name, O_CREAT | O_RDWR, 0666);
    shyper_ftruncate(fd, size);
    return shyper_mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void signal_recv_message_wrapper(int signum) {
#ifdef TEST_TIME
    user_signal[signal_cur++] = gettime();
#endif
    recv_message(temp_msg);
    // test_fprintf("Received message: %s\n", temp_msg);
}

void init_shmem(int test_cnt) {
    shyper_fd = open("/dev/shyper", O_RDWR);
    mem_fd = open("/dev/posix_shmem", O_RDWR);
    ioctl(shyper_fd, 0x1002, getpid());
#ifdef TEST_TIME
    // set kernel timer
    ioctl(shyper_fd, 0x1300, test_cnt);
    user_send_start = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_send_end = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_signal = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_recv_end = (u64 *)malloc(sizeof(u64) * test_cnt);
    if (user_send_start == NULL || user_recv_end == NULL) {
        perror("failed to malloc mvm_start and mvm_end");
        exit(EXIT_FAILURE);
    }
    send_start_cur = recv_end_cur = 0;
#endif
    for (int i = 0; i < SHMEM_NUM; i++) {
        sprintf(shmem_name, "%s_%d", MVM_SEND_NAME, i);
        shm_send_pointer[i] = shm_send_start[i] = open_new_shmem(shmem_name, SHMEM_SIZE);
        sprintf(shmem_name, "%s_%d", MVM_RECV_NAME, i);
        shm_recv_pointer[i] = shm_recv_start[i] = open_new_shmem(shmem_name, SHMEM_SIZE);
    }
    if (SHMEM_NUM > SHMEM_SIZE) {
        test_fprintf("current setting is not supported for SHMEM_NUM > SHMEM_SIZE\n");
        exit(EXIT_FAILURE);
    }
    ack_send_start = (int *)open_new_shmem(MVM_ACK_SEND_NAME, SHMEM_SIZE);
    ack_recv_start = (int *)open_new_shmem(MVM_ACK_RECV_NAME, SHMEM_SIZE);
    bzero(ack_send_start, SHMEM_SIZE);
    bzero(ack_recv_start, SHMEM_SIZE);
    test_fprintf("Share memory %s_0 ~ %d and %s_0 ~ %d are created\n", MVM_RECV_NAME, SHMEM_NUM - 1, MVM_SEND_NAME, SHMEM_NUM - 1);
    shm_send_cur_id = shm_recv_cur_id = 0;
    send_seq_num = recv_seq_num = 1;
    recv_ack_num = send_seq_num + 1;

    // use signal to notify MVM to receive message
    struct sigaction sa_usr1;
    sa_usr1.sa_handler = signal_recv_message_wrapper;
    sa_usr1.sa_flags = 0;
    sigemptyset(&sa_usr1.sa_mask);
    sigaction(SIGUSR1, &sa_usr1, NULL);
}

// Endianness must be the same. Zephyr is little-endian.
int set_shmem_data(int shm_id, const char *data, int size) {
    char *wp = shm_send_pointer[shm_id];
    if(wp + size > shm_send_start[shm_id] + SHMEM_SIZE) {
        test_fprintf("failed to write new data to %s_%d since size %d KB is too large\n", MVM_RECV_NAME, shm_id, size);
        return -1;
    }
    memcpy(wp, data, size);
    shm_send_pointer[shm_id] = wp + size;
    return 0;
}
    
int get_shmem_data(int shm_id, char *data, int size) {
    char *rp = shm_recv_pointer[shm_id];
    if(rp + size > shm_recv_start[shm_id] + SHMEM_SIZE) {
        test_fprintf("failed to read data from %s_%d since size %d KB is too large\n", MVM_SEND_NAME, shm_id, size);
        return -1;
    }
    memcpy(data, rp, size);
    shm_recv_pointer[shm_id] = rp + size;
    return 0;
}

// set the flag to seq_num after sending a message
void set_seq_num(int shm_id, int seq_num) {
    shm_send_pointer[shm_id] = shm_send_start[shm_id];
    set_shmem_data(shm_id, (char *)&seq_num, sizeof(int));
}

// set the flag to ack_num after receiving a message
void set_ack_num(int shm_id, int ack_num) {
    int *wr = ack_send_start + shm_id;
    *wr = ack_num;
}

int is_valid(int shm_id, int is_send) {
    if(is_send){
        // when MVM send a message to GVM, check if the ack_num is equal to recv_ack_num
        // to ensure the message is received by GVM
        int *rp = ack_recv_start + shm_id;
        if(*rp == recv_ack_num){
            recv_ack_num ++;
            return 1;
        }else return 0;
    }
    else{
        int recv_num;
        shm_recv_pointer[shm_id] = shm_recv_start[shm_id];
        get_shmem_data(shm_id, (char *)&recv_num, sizeof(int));
        if(recv_num == recv_seq_num){
            recv_seq_num ++;
            return 1;
        }else return 0;
    }
}

void notify_gvm(int shm_id) {
    sprintf(shmem_name, "%s_%d", MVM_RECV_NAME, shm_id);
    struct shm_notify notify;
    notify.name = shmem_name;
    notify.name_len = strlen(shmem_name);
    ioctl(shyper_fd, 0x1307, &notify);
}

int send_message(int len, const char *data) {
#ifdef TEST_TIME
    // static int cnt = 0;
    // test_fprintf("send cnt:%d\n", cnt++);
    user_send_start[send_start_cur++] = gettime();
#endif
    static int sendFailtime = 0;
    if(!is_valid(shm_send_cur_id, SEND) && send_seq_num > SHMEM_NUM){
        if(sendFailtime <= 200){
            sendFailtime++;
            test_fprintf("send_message fail: shmem pool %s_%d is full, send_seq_num = %d, recv_ack_num = %d\n", 
                MVM_SEND_NAME, shm_send_cur_id, send_seq_num, recv_ack_num, recv_seq_num);
            if(sendFailtime % 20 == 0){
                for(int i = 0; i < SHMEM_NUM; i++){
                    int *num = (int *)ack_recv_start + i;
                    test_fprintf("recv_ack_number[%d] = %d\n", i, *num);
                }
            }
        }
        return ERROR_SHM_FULL;
    }
    sendFailtime = 0;
    shm_send_pointer[shm_send_cur_id] = shm_send_start[shm_send_cur_id] + sizeof(int);
    set_shmem_data(shm_send_cur_id, (char *)&len, sizeof(int));
    set_shmem_data(shm_send_cur_id, data, len * sizeof(char));
    set_seq_num(shm_send_cur_id, send_seq_num++);

#ifdef TEST_TIME
    user_send_end[send_end_cur++] = gettime();
#endif    
    // notify_gvm(shm_send_cur_id);
    shm_send_cur_id = NEXT(shm_send_cur_id);
    return 0;
}

int recv_message(char *data) {
    if(!is_valid(shm_recv_cur_id, RECV)){
        test_fprintf("recv_message fail: shmem pool %s_%d is empty\n", MVM_SEND_NAME, shm_recv_cur_id);
        return ERROR_SHM_EMPTY;
    }

    int len;
    get_shmem_data(shm_recv_cur_id, (char *)&len, sizeof(int));
    get_shmem_data(shm_recv_cur_id, data, len * sizeof(char));
    data[len] = '\0';

    // set the flag to INVALID after reading the message
    set_ack_num(shm_recv_cur_id, recv_seq_num);
    shm_recv_cur_id = NEXT(shm_recv_cur_id);  
#ifdef TEST_TIME  
    user_recv_end[recv_end_cur++] = gettime();
#endif
    return len;
}

#ifdef TEST_TIME
    void record_test_result() {
        char filename[100];
        memset(filename, '\0', sizeof(filename));
        // kernel records the result first
        ioctl(shyper_fd, 0x1308, filename);
        test_fprintf("Record test result to %s\n", filename);
        FILE *fp = fopen(filename, "a+");
        if(fp == NULL){
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < send_start_cur; ++i)
            fprintf(fp, "%llu ", user_send_start[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < send_end_cur; ++i)
            fprintf(fp, "%llu ", user_send_end[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < signal_cur; ++i)
            fprintf(fp, "%llu ", user_signal[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < recv_end_cur; ++i)
            fprintf(fp, "%llu ", user_recv_end[i]);
        fprintf(fp, "\n");
        for(int i = 0; i < recv_end_cur; i++)
            fprintf(fp, "%llu ", user_recv_end[i] - user_send_start[i]);
        fprintf(fp, "\n");
        fclose(fp);
    }

    u64 get_recv_message_checksum() {
        u64 checksum = 0, mod = 1e9 + 7;
        int len = strlen(temp_msg);
        for(int i = 0; i < len; i++)
            checksum = (checksum * 256 % mod + temp_msg[i]) % mod;
        return checksum;
    }
#endif

void close_shmem() {
    // TODO: close all shmem
    ioctl(shyper_fd, 0x1305, NULL);
    close(shyper_fd);
    close(mem_fd);
}


