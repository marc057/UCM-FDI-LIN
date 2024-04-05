#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define BUFFER_SIZE 512

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom Timer for Android");
MODULE_AUTHOR("Marcos Colombás García");

static struct proc_dir_entry *proc_entry;

struct timer_list my_timer;             /* Structure that describes the kernel timer */
static unsigned int timer_duration = 1; /* Duración del temporizador en segundos */

/* Function invoked when timer expires (fires) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
static void fire_timer(struct timer_list *timer)
#else
static void fire_timer(unsigned long data)
#endif
{
    static char flag = 0;
    char *message[] = {"Tic", "Tac"};

    if (flag == 0)
        printk(KERN_INFO "%s\n", message[0]);
    else
        printk(KERN_INFO "%s\n", message[1]);

    flag = ~flag;

    /* Re-activate the timer after timer_duration seconds from now */
    mod_timer(&my_timer, jiffies + (HZ * timer_duration));
}
 
static ssize_t timer_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    int ret;
    char kbuf[BUFFER_SIZE];
    size_t kbuf_len;

    if (*off > 0) {
        return 0;
    }

    snprintf(kbuf, BUFFER_SIZE, "%u", timer_duration);

    kbuf_len = strlen(kbuf);

    kbuf[kbuf_len] = '\n';

    ret = copy_to_user(buf, kbuf, kbuf_len + 1);
    if (ret < 0) {
        return -EFAULT;
    }

    printk("Valor de timer: %u\n", timer_duration);

    *off += kbuf_len + 1;

    return kbuf_len + 1;
}



static ssize_t timer_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
    int ret;
    unsigned int new_duration;
    char *kbuf;

    kbuf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (kbuf == NULL) {
        kfree(kbuf);
        return -ENOMEM;
    }

    ret = copy_from_user(kbuf, buf, len);
    if (ret < 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    kstrtouint(kbuf, 10, &new_duration);
    if (new_duration <= 0)
    {
        printk(KERN_ERR "Timer: Timer duration must be a positive integer greater than 0\n");
        kfree(kbuf);
        return -EINVAL;
    }

    timer_duration = new_duration;

    printk("Valor de timer: %u\n", timer_duration);
    /* Re-activate the timer with the new timer_duration */
    mod_timer(&my_timer, jiffies + (HZ * timer_duration));

    *off += len;
    kfree(kbuf);

    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
static const struct proc_ops proc_entry_fops = {
    .proc_read = timer_read,
    .proc_write = timer_write,
};
#else
static const struct file_operations proc_entry_fops = {
    .read = timer_read,
    .write = timer_write,
};
#endif

int init_timer_module(void)
{
    proc_entry = proc_create("timer", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL)
    {
        printk(KERN_INFO "Timer: Can't create /proc entry\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "Timer: Module loaded\n");

    /* Create timer */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
    timer_setup(&my_timer, fire_timer, 0);
#else
    init_timer(&my_timer);
    my_timer.data = 0;
    my_timer.function = fire_timer;
#endif
    my_timer.expires = jiffies + (HZ * timer_duration); // Activar timer con timer_duration

    /* Activate the timer for the first time */
    add_timer(&my_timer);

    return 0;
}

void cleanup_timer_module(void)
{
    remove_proc_entry("timer", NULL);
    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
    printk(KERN_INFO "Timer: Module unloaded\n");
}

module_init(init_timer_module);
module_exit(cleanup_timer_module);
