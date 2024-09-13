#include "include/posix_shmem.h"
#include "include/test_utils.h"

const char VALID   = 1;
const char INVALID = 0;
int shm_write_cur_id, shm_read_cur_id;
char *shm_write_start[SHMEM_NUM];
char *shm_read_start[SHMEM_NUM];
char *shm_write_pointer[SHMEM_NUM];
char *shm_read_pointer[SHMEM_NUM];
char temp_msg[SHMEM_SIZE], shmem_name[SHMEM_NAME_MAX_LEN];

#ifdef TEST_TIME
    int shyper_fd, mem_fd, pid, write_start_cur, write_end_cur, signal_cur, read_end_cur;
    u64* user_write_start, * user_write_end, * user_signal, * user_read_end;
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
    user_write_start = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_write_end = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_signal = (u64 *)malloc(sizeof(u64) * test_cnt);
    user_read_end = (u64 *)malloc(sizeof(u64) * test_cnt);
    if (user_write_start == NULL || user_read_end == NULL) {
        perror("failed to malloc mvm_start and mvm_end");
        exit(EXIT_FAILURE);
    }
    write_start_cur = read_end_cur = 0;
#endif
    for (int i = 0; i < SHMEM_NUM; i++) {
        sprintf(shmem_name, "%s_%d", MVM_WRITE_NAME, i);
        shm_write_pointer[i] = shm_write_start[i] = open_new_shmem(shmem_name, SHMEM_SIZE);
        sprintf(shmem_name, "%s_%d", MVM_READ_NAME, i);
        shm_read_pointer[i] = shm_read_start[i] = open_new_shmem(shmem_name, SHMEM_SIZE);
    }
    test_fprintf("Share memory %s_0 ~ %d and %s_0 ~ %d are created\n", MVM_WRITE_NAME, SHMEM_NUM - 1, MVM_READ_NAME, SHMEM_NUM - 1);
    shm_write_cur_id = shm_read_cur_id = 0;

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = signal_recv_message_wrapper;
    sa_usr1.sa_flags = 0;
    sigemptyset(&sa_usr1.sa_mask);
    sigaction(SIGUSR1, &sa_usr1, NULL);
}

// Endianness must be the same. Zephyr is little-endian.
int set_shmem_data(int shm_id, const char *data, int size) {
    char *wp = shm_write_pointer[shm_id];
    if(wp + size > shm_write_start[shm_id] + SHMEM_SIZE) {
        test_fprintf("failed to write new data to %s_%d since size %d KB is too large\n", MVM_WRITE_NAME, shm_id, size);
        return -1;
    }
    memcpy(wp, data, size);
    shm_write_pointer[shm_id] = wp + size;
    return 0;
}
    
int get_shmem_data(int shm_id, char *data, int size) {
    char *rp = shm_read_pointer[shm_id];
    if(rp + size > shm_read_start[shm_id] + SHMEM_SIZE) {
        test_fprintf("failed to read data from %s_%d since size %d KB is too large\n", MVM_READ_NAME, shm_id, size);
        return -1;
    }
    memcpy(data, rp, size);
    shm_read_pointer[shm_id] = rp + size;
    return 0;
}

int is_valid(int shm_id, int is_write) {
    if(is_write){
        // get_shmem_data is not supported for write
        char *rd = shm_write_start[shm_id];
        return *rd == VALID;
    }else{
        char flag;
        shm_read_pointer[shm_id] = shm_read_start[shm_id];
        get_shmem_data(shm_id, &flag, sizeof(char));
        return flag == VALID;
    }
}

void notify_gvm(int shm_id) {
    sprintf(shmem_name, "%s_%d", MVM_WRITE_NAME, shm_id);
    struct shm_notify notify;
    notify.name = shmem_name;
    notify.name_len = strlen(shmem_name);
    ioctl(shyper_fd, 0x1307, &notify);
}

int send_message(int len, const char *data) {
#ifdef TEST_TIME
    // static int cnt = 0;
    // test_fprintf("send cnt:%d\n", cnt++);
    user_write_start[write_start_cur++] = gettime();
#endif
    if(is_valid(shm_write_cur_id, WRITE)){
        test_fprintf("shmem pool %s is full, cur shm_id is %d\n", MVM_WRITE_NAME, shm_write_cur_id);
        return ERROR_SHM_FULL;
    }
    shm_write_pointer[shm_write_cur_id] = shm_write_start[shm_write_cur_id] + sizeof(VALID);
    set_shmem_data(shm_write_cur_id, (char *)&len, sizeof(int));
    set_shmem_data(shm_write_cur_id, data, len * sizeof(char));

    // set the flag to VALID after writing the message
    shm_write_pointer[shm_write_cur_id] = shm_write_start[shm_write_cur_id];
    set_shmem_data(shm_write_cur_id, &VALID, sizeof(char));

#ifdef TEST_TIME
    user_write_end[write_end_cur++] = gettime();
#endif    
    notify_gvm(shm_write_cur_id);
    shm_write_cur_id = NEXT(shm_write_cur_id);
    return 0;
}

int recv_message(char *data) {
    if(!is_valid(shm_read_cur_id, READ)){
        test_fprintf("shmem pool %s is empty, cur shm_id is %d\n", MVM_READ_NAME, shm_read_cur_id);
        return ERROR_SHM_EMPTY;
    }

    shm_read_pointer[shm_read_cur_id] = shm_read_start[shm_read_cur_id] + sizeof(VALID);
    int len;
    get_shmem_data(shm_read_cur_id, (char *)&len, sizeof(int));
    get_shmem_data(shm_read_cur_id, data, len * sizeof(char));
    data[len] = '\0';

    // set the flag to INVALID after reading the message
    char *wr = shm_read_start[shm_read_cur_id];
    *wr = INVALID;

    shm_read_cur_id = NEXT(shm_read_cur_id);  
#ifdef TEST_TIME  
    user_read_end[read_end_cur++] = gettime();
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
        for (int i = 0; i < write_start_cur; ++i)
            fprintf(fp, "%llu ", user_write_start[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < write_end_cur; ++i)
            fprintf(fp, "%llu ", user_write_end[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < signal_cur; ++i)
            fprintf(fp, "%llu ", user_signal[i]);
        fprintf(fp, "\n");
        for (int i = 0; i < read_end_cur; ++i)
            fprintf(fp, "%llu ", user_read_end[i]);
        fprintf(fp, "\n");
        for(int i = 0; i < read_end_cur; i++)
            fprintf(fp, "%llu ", user_read_end[i] - user_write_start[i]);
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


