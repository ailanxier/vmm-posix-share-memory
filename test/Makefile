CC=gcc

SOURCES=test_linux_shmem.c test_misc_device.c test_hvc.c test_shyper_shmem_vm1.c test_shyper_shmem.c

EXECUTABLES=$(SOURCES:.c=)

all: $(EXECUTABLES)

%: %.c
	$(CC) $< -o $@

clean:
	rm -f $(EXECUTABLES)