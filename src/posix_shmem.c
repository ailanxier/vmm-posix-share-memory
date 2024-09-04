#include "include/posix_shmem.h"
#include "include/test_utils.h"

const char VALID   = 1;
const char INVALID = 0;
int shm_write_cur_id, shm_read_cur_id;
char *shm_write_start[SHMEM_NUM];
char *shm_read_start[SHMEM_NUM];
char *shm_write_pointer[SHMEM_NUM];
char *shm_read_pointer[SHMEM_NUM];
int shyper_fd, memfd, pid;
uint64 mvm_start;
char temp_msg[3000];

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

    void *va = mmap(NULL, shmmap.length, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, ipa_offset);
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
    close(shyper_fd);
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
    recv_message(temp_msg);
    test_fprintf("Received message: %s\n", temp_msg);
}

void init_shmem() {
    shyper_fd = open("/dev/shyper", O_RDWR);
    memfd = open("/dev/posix_shmem", O_RDWR);
    ioctl(shyper_fd, 0x1002, getpid());
    char name[SHMEM_NAME_MAX_LEN];
    for (int i = 0; i < SHMEM_NUM; i++) {
        sprintf(name, "%s_%d", MVM_WRITE_NAME, i);
        shm_write_pointer[i] = shm_write_start[i] = open_new_shmem(name, SHMEM_SIZE);
        sprintf(name, "%s_%d", MVM_READ_NAME, i);
        shm_read_pointer[i] = shm_read_start[i] = open_new_shmem(name, SHMEM_SIZE);
    }
    test_fprintf("Share memory %s_0 ~ %d and %s_0 ~ %d are created\n", MVM_WRITE_NAME, SHMEM_NUM - 1, MVM_READ_NAME, SHMEM_NUM - 1);
    shm_write_cur_id = shm_read_cur_id = 0;
    signal(SIGUSR1, signal_recv_message_wrapper);
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
    char *name = (char *)malloc(SHMEM_NAME_MAX_LEN);
    sprintf(name, "%s_%d", MVM_WRITE_NAME, shm_id);
    struct shm_notify notify;
    notify.name = name;
    notify.name_len = strlen(name);
    ioctl(shyper_fd, 0x1307, &notify);
}

int send_message(int len, const char *data) {
    // mvm_start = gettime();
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
    // uint64 mvm_end = gettime();
    // test_fprintf("mvm ioctl -> hvc -> hypervisor -> zephyr -> hypervisor -> kernel -> mvm: %llu us\n", mvm_end - mvm_start);
    // test_fprintf("mvm start %llu us, end %llu us\n", mvm_start, mvm_end);
    return 0;
}