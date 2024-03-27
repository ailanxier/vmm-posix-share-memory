#include"include/posix_shmem.h"

int shyper_shm_open(const char *name, int oflag, mode_t mode) {
    char shmem_name[SHM_NAME_MAX_LEN];
    int fd, shyper_fd;
    struct shm_name shm;

    shm.name = name;
    shm.name_len = strlen(name);
    shm.shm_name = shmem_name;
    shyper_fd = open("/dev/shyper", O_RDWR);

    ioctl(shyper_fd, 0x1302, &shm);

    fd = shm.fd;
    if (fd == -1) {
        perror("open");
        return -1;
    }

    return fd;
}

int shyper_ftruncate(int fd, off_t length) {
    int shyper_fd, ret;
    struct shm_size size;

    size.shm_fd = fd;
    size.size = length;
    shyper_fd = open("/dev/shyper", O_RDWR);

    ioctl(shyper_fd, 0x1303, &size);

    ret = size.ret;
    if (ret == -1) {
        perror("ftruncate");
        return -1;
    }

    return ret;
}

void *shyper_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    int shyper_fd;
    struct shm_mmap mmap;

    mmap.addr = addr;
    mmap.length = length;
    mmap.prot = prot;
    mmap.flags = flags;
    mmap.fd = fd;
    mmap.offset = offset;
    shyper_fd = open("/dev/shyper", O_RDWR);

    ioctl(shyper_fd, 0x1304, &mmap);

    if (mmap.ret == MAP_FAILED) {
        perror("mmap");
        return MAP_FAILED;
    }

    return mmap.ret;
}

int shyper_munmap(void *addr, size_t length) {
    int shyper_fd, ret;
    struct shm_unmap unmap;

    unmap.addr = addr;
    unmap.length = length;
    shyper_fd = open("/dev/shyper", O_RDWR);

    ioctl(shyper_fd, 0x1305, &unmap);

    ret = unmap.ret;
    if (ret == -1) {
        perror("munmap");
        return -1;
    }

    return ret;
}

int shm_unlink(const char *name) {
    int shyper_fd, ret;
    struct shm_unlink unlink;

    unlink.name = name;
    unlink.name_len = strlen(name);
    shyper_fd = open("/dev/shyper", O_RDWR);

    ioctl(shyper_fd, 0x1306, &unlink);

    ret = unlink.ret;
    if (ret == -1) {
        perror("shm_unlink");
        return -1;
    }

    return ret;
}