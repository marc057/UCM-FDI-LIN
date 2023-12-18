#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#define BUFFER_LENGTH   2048

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Practica 3 LIN");
MODULE_AUTHOR("Marcos Colombas Garcia");

DEFINE_SPINLOCK(sp);

static char* modlist;

static struct list_item {
    int data;
    struct list_head links;
};

static struct list_head mylist;

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  
    char buffer[BUFFER_LENGTH];

    if (*off > 0) {
        return 0;
    }

    int pos = 0;
    struct list_item* entry;
    spin_lock(&sp);
    list_for_each_entry(entry, &mylist, links) {
        pos += sprintf(&buffer[pos], "%i\n", entry->data);
    }
    sprintf(&buffer[pos], "%c", '\0');
    spin_lock(&sp);
    int num_bytes = strlen(buffer);
    
    /* Transfer data from the kernel to userspace */  
    if (copy_to_user(buf, buffer, num_bytes)) {
        return -EINVAL;
    }
    
    (*off) += len;
    return len;
}

void add(int n) {
    struct list_item *item = (struct list_item *) kmalloc(sizeof(struct list_item), GFP_KERNEL);
    item->data = n;
    spin_lock(&sp);
    list_add_tail(&(item->links), &mylist);
    spin_unlock(&sp);
    printk(KERN_INFO "Modlist: Item %d added correctly\n", n);
}

void remove(int n) {
    struct list_item* entry;
    struct list_item* aux;
    
    list_for_each_entry_safe(entry, aux, &mylist, links) {
        if (entry->data == n) {
            printk(KERN_INFO "Modlist: Deleting %d\n", entry->data);
            spin_lock(&sp);
            list_del(&(entry->links));
            spin_unlock(&sp);
            kfree(entry);
        }
    }
    printk(KERN_INFO "Modlist: Items with number %d removed correctly\n", n);
}

void cleanup(void) {
    struct list_item* entry;
    struct list_item* aux;
    list_for_each_entry_safe(entry, aux, &mylist, links) {
        spin_lock(&sp);  
        list_del(&(entry->links));
        spin_unlock(&sp);
        kfree(entry);
    }
    printk(KERN_INFO "Modlist: List cleanup completed correctly\n");
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    int n;
    char kbuf[BUFFER_LENGTH];

    if (len > BUFFER_LENGTH - 1) {
        printk(KERN_INFO "Modlist: not enough space!!\n");
        return -ENOSPC;
    }

    if (copy_from_user(kbuf, buf, len)) {
        return -EFAULT;
    }
    kbuf[len] = '\0';
    *off+=len;

    if (sscanf(kbuf, "add %i", &n) == 1) {
        add(n);
    }
    else if (sscanf(kbuf, "remove %i", &n) == 1) {
        remove(n);
    }
    else if (strcmp(kbuf, "cleanup") == 0) {
        cleanup();
    }
    else {
        return -EINVAL;
    }

    return len;
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = modlist_read,
    .proc_write = modlist_write,
};


static int __init init_modlist_module( void ) {
    modlist = (char *)kmalloc( BUFFER_LENGTH, GFP_KERNEL);

    if (!modlist) {
        printk(KERN_INFO "Modlist: Can't create modlist\n");
        return -ENOMEM;
    }
    memset( modlist, 0, BUFFER_LENGTH );

    if (!proc_create( "modlist", 0666, NULL, &proc_entry_fops)) {
      kfree(modlist);
      printk(KERN_INFO "Modlist: Can't create /proc entry\n");
      return -ENOMEM;
    }
    spin_lock_init(sp);
    INIT_LIST_HEAD(&mylist);

    printk(KERN_INFO "Modlist: Module loaded\n");
    return 0;
}

static void __exit exit_modlist_module( void )
{
    remove_proc_entry("modlist", NULL);
    kfree(modlist);
    printk(KERN_INFO "Modlist: Module unloaded.\n");
}

module_init(init_modlist_module);
module_exit(exit_modlist_module);