CC=gcc

SOURCES=test_linux_shmem.c test_misc_device.c

EXECUTABLES=$(SOURCES:.c=)

all: $(EXECUTABLES)

%: %.c
	$(CC) $< -o $@

clean:
	rm -f $(EXECUTABLES)