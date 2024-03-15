#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/misc_device"
#define BUF_SIZE 256

int main() {
    int fd;
    char write_buf[BUF_SIZE] = "Hello, Kernel!";
    char read_buf[BUF_SIZE];

    // Open the device
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }
    printf("Open device, fd = %d\n", fd);

    // Write to the device
    if (write(fd, write_buf, strlen(write_buf)) < 0) {
        perror("Failed to write to the device");
        return errno;
    }

    printf("fd = %d\n", fd);
    fd = open(DEVICE_PATH, O_RDWR);
    printf("fd = %d\n", fd);

    // Read from the device
    if (read(fd, read_buf, BUF_SIZE) < 0) {
        perror("Failed to read from the device");
        return errno;
    }

    printf("Read from the device: %s\n", read_buf);
    // Close the device
    close(fd);

    return 0;
}
