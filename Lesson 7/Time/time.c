#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/time.h>
#include <linux/rtc.h>
#include <uapi/linux/rtc.h>


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Dyomin Denis");
MODULE_DESCRIPTION("Procfs read");
MODULE_VERSION("0.2");


#define MODULE_TAG      "Time module "
#define PROC_DIRECTORY  "Lesson_7_time"
#define PROC_FILENAME   "read_time"

struct timespec ts = {0};

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

static int example_read(struct file *file_p, char __user *buffer, size_t length, loff_t *offset);

static struct file_operations proc_fops = {
    .read  = example_read,
};


static int create_proc_example(void)
{
    proc_dir = proc_mkdir(PROC_DIRECTORY, NULL);
    if (NULL == proc_dir)
        return -EFAULT;

    proc_file = proc_create(PROC_FILENAME, S_IFREG | S_IRUGO | S_IWUGO, proc_dir, &proc_fops);
    if (NULL == proc_file)
        return -EFAULT;

    return 0;
}


static void cleanup_proc_example(void)
{
    if (proc_file)
    {
        remove_proc_entry(PROC_FILENAME, proc_dir);
        proc_file = NULL;
    }
    if (proc_dir)
    {
        remove_proc_entry(PROC_DIRECTORY, NULL);
        proc_dir = NULL;
    }
}


static int example_read(struct file *file_p, char __user *buffer, size_t length, loff_t *offset)
{
    struct timeval tv;

    getnstimeofday(&ts);
    do_gettimeofday(&tv);
    printk("timespec: %u (sec)\n", ts.tv_sec);
    printk("timeval: %u (sec)\n", tv.tv_sec);

    return 0;
}


static int __init example_init(void)
{
    int err = create_proc_example();

    if (err)
        goto error;

    printk(KERN_NOTICE MODULE_TAG "loaded\n");
    return 0;

error:
    printk(KERN_ERR MODULE_TAG "failed to load\n");
    cleanup_proc_example();

    return err;
}


static void __exit example_exit(void)
{
    cleanup_proc_example();
    printk(KERN_NOTICE MODULE_TAG "exited\n");
}

module_init(example_init);
module_exit(example_exit);

