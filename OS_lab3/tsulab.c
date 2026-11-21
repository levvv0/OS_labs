#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *tsulab_entry;

static ssize_t tsulab_read(struct file *file, char __user *buffer,
                           size_t count, loff_t *offset)
{
    char message[64];
    int length = scnprintf(message, sizeof(message), "Online CPUs: %u\n",
                          num_online_cpus());

    return simple_read_from_buffer(buffer, count, offset, message, length);
}

static const struct proc_ops tsulab_ops = {
    .proc_read = tsulab_read,
};

static int __init tsulab_init(void)
{
    tsulab_entry = proc_create("tsulab", 0444, NULL, &tsulab_ops);
    if (!tsulab_entry)
        return -ENOMEM;

    pr_info("Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tsulab_exit(void)
{
    proc_remove(tsulab_entry);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
