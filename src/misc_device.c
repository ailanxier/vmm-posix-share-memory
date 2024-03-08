#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "my_misc_device"
#define BUF_SIZE 256

static char kernel_buffer[BUF_SIZE];

static ssize_t my_misc_device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{
    ssize_t ret;

    ret = simple_read_from_buffer(user_buffer, count, ppos, kernel_buffer, BUF_SIZE);
    return ret;
}

static ssize_t my_misc_device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
    ssize_t ret;

    ret = simple_write_to_buffer(kernel_buffer, BUF_SIZE, ppos, user_buffer, count);
    return ret;
}

static const struct file_operations my_misc_device_fops = {
    .owner = THIS_MODULE,
    .read = my_misc_device_read,
    .write = my_misc_device_write,
};

static struct miscdevice my_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &my_misc_device_fops,
};

static int __init my_misc_device_init(void)
{
    int ret;

    ret = misc_register(&my_misc_device);
    if (ret) {
        pr_err("failed to register misc device\n");
        return ret;
    }
    pr_info("misc device registered\n");
    return 0;
}

static void __exit my_misc_device_exit(void)
{
    misc_deregister(&my_misc_device);
    pr_info("misc device unregistered\n");
}

module_init(my_misc_device_init);
module_exit(my_misc_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple misc device example");
