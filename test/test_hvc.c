#include <unistd.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <wait.h>

int test_shmem_init(char *name) {
    int shyper_fd = open("/dev/shyper", O_RDWR);
    struct {
        char *name;
        int name_len;
    } ioctl_release;
    ioctl_release.name = name;
    ioctl_release.name_len = strlen(name);
    ioctl(shyper_fd, 0x1302, &ioctl_release);
    close(shyper_fd);
    return 0;
}

int main() {
    char *name = "my_shared_memory";
    test_shmem_init(name);
    return 0;
}