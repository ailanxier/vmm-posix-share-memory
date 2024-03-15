#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "misc_device"
#define BUF_SIZE 256
 
static char kernel_buffer[BUF_SIZE];
// static char kernel_buffer2[BUF_SIZE] = "test for read";

static ssize_t misc_device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{
    ssize_t ret;
    pr_info("start device read\n");
    ret = simple_read_from_buffer(user_buffer, count, ppos, kernel_buffer, BUF_SIZE);
    pr_info("%s", kernel_buffer);
    return ret;
}

static ssize_t misc_device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
    ssize_t ret;
    pr_info("start device write\n");
    ret = simple_write_to_buffer(kernel_buffer, BUF_SIZE, ppos, user_buffer, count);
    pr_info("%s", kernel_buffer);
    return ret;
}

static const struct file_operations misc_device_fops = {
    .owner = THIS_MODULE,
    .read = misc_device_read,
    .write = misc_device_write,
};

static struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &misc_device_fops,
};

static int __init misc_device_init(void)
{
    int ret;

    ret = misc_register(&misc_device);
    if (ret) {
        pr_err("failed to register misc device\n");
        return ret;
    }
    pr_info("misc device registered\n");
    return 0;
}

static void __exit misc_device_exit(void)
{
    misc_deregister(&misc_device);
    pr_info("misc device unregistered\n");
}

MODULE_LICENSE("GPL v2");

module_init(misc_device_init);
module_exit(misc_device_exit);
