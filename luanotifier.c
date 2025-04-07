#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kmsg_dump.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Lua Notifier Device");

#define DEVICE_NAME "luanotifier"
#define MAX_EVENTS 100
#define MAX_EVENT_SIZE 256

struct lua_notifier_data {
    struct cdev cdev;
    struct class *class;
    dev_t dev;
    char events[MAX_EVENTS][MAX_EVENT_SIZE];
    int event_count;
    int read_index;
    struct mutex lock;
};

static struct lua_notifier_data *notifier_data;

// Function to capture kernel messages
static void lua_kmsg_dump(struct kmsg_dumper *dumper, enum kmsg_dump_reason reason)
{
    char line[MAX_EVENT_SIZE];
    size_t len;
    struct kmsg_dump_iter iter;

    printk(KERN_INFO "Lua Notifier: Capturing kernel messages\n");
    
    kmsg_dump_rewind(&iter);
    while (kmsg_dump_get_line(&iter, true, line, sizeof(line), &len)) {
        mutex_lock(&notifier_data->lock);
        if (notifier_data->event_count < MAX_EVENTS) {
            strncpy(notifier_data->events[notifier_data->event_count], 
                    line, MAX_EVENT_SIZE);
            notifier_data->event_count++;
            printk(KERN_INFO "Lua Notifier: Captured message: %s\n", line);
        }
        mutex_unlock(&notifier_data->lock);
    }
}

static struct kmsg_dumper lua_dumper = {
    .dump = lua_kmsg_dump,
};

static int luanotifier_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Lua Notifier: Device opened\n");
    return 0;
}

static int luanotifier_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Lua Notifier: Device closed\n");
    return 0;
}

static ssize_t luanotifier_read(struct file *file, char __user *buf,
                              size_t count, loff_t *ppos)
{
    int ret = 0;
    char event_str[MAX_EVENT_SIZE];
    
    mutex_lock(&notifier_data->lock);
    if (notifier_data->read_index < notifier_data->event_count) {
        strncpy(event_str, notifier_data->events[notifier_data->read_index], 
                MAX_EVENT_SIZE);
        notifier_data->read_index++;
        ret = strlen(event_str);
        if (copy_to_user(buf, event_str, ret)) {
            ret = -EFAULT;
        }
        // Add a newline after each message
        if (ret > 0 && ret < count) {
            char newline = '\n';
            if (copy_to_user(buf + ret, &newline, 1)) {
                ret = -EFAULT;
            } else {
                ret++;
            }
        }
    }
    mutex_unlock(&notifier_data->lock);
    
    return ret;
}

static ssize_t luanotifier_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos)
{
    char event_str[MAX_EVENT_SIZE];
    int ret = 0;
    
    if (count > MAX_EVENT_SIZE - 1)
        count = MAX_EVENT_SIZE - 1;
    
    if (copy_from_user(event_str, buf, count)) {
        ret = -EFAULT;
        goto out;
    }
    
    event_str[count] = '\0';
    
    mutex_lock(&notifier_data->lock);
    if (notifier_data->event_count < MAX_EVENTS) {
        strncpy(notifier_data->events[notifier_data->event_count], 
                event_str, MAX_EVENT_SIZE);
        notifier_data->event_count++;
        ret = count;
        printk(KERN_INFO "Lua Notifier: Stored message: %s\n", event_str);
    } else {
        ret = -ENOMEM;
    }
    mutex_unlock(&notifier_data->lock);
    
out:
    return ret;
}

static const struct file_operations luanotifier_fops = {
    .owner = THIS_MODULE,
    .open = luanotifier_open,
    .release = luanotifier_release,
    .read = luanotifier_read,
    .write = luanotifier_write,
};

static int __init luanotifier_init(void)
{
    int ret;
    
    printk(KERN_INFO "Lua Notifier: Initializing module\n");
    
    // Allocate memory for device data
    notifier_data = kmalloc(sizeof(struct lua_notifier_data), GFP_KERNEL);
    if (!notifier_data) {
        printk(KERN_ERR "Lua Notifier: Failed to allocate memory\n");
        return -ENOMEM;
    }
    
    // Initialize mutex
    mutex_init(&notifier_data->lock);
    
    // Initialize device data
    notifier_data->event_count = 0;
    notifier_data->read_index = 0;
    
    // Register character device
    ret = alloc_chrdev_region(&notifier_data->dev, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Lua Notifier: Failed to allocate device number\n");
        kfree(notifier_data);
        return ret;
    }
    
    // Initialize cdev
    cdev_init(&notifier_data->cdev, &luanotifier_fops);
    notifier_data->cdev.owner = THIS_MODULE;
    
    // Add cdev to system
    ret = cdev_add(&notifier_data->cdev, notifier_data->dev, 1);
    if (ret < 0) {
        printk(KERN_ERR "Lua Notifier: Failed to add cdev\n");
        unregister_chrdev_region(notifier_data->dev, 1);
        kfree(notifier_data);
        return ret;
    }
    
    // Create device class
    notifier_data->class = class_create(DEVICE_NAME);
    if (IS_ERR(notifier_data->class)) {
        printk(KERN_ERR "Lua Notifier: Failed to create device class\n");
        cdev_del(&notifier_data->cdev);
        unregister_chrdev_region(notifier_data->dev, 1);
        kfree(notifier_data);
        return PTR_ERR(notifier_data->class);
    }
    
    // Create device node
    device_create(notifier_data->class, NULL, notifier_data->dev, NULL, DEVICE_NAME);
    
    // Register kmsg dumper
    ret = kmsg_dump_register(&lua_dumper);
    if (ret < 0) {
        printk(KERN_ERR "Lua Notifier: Failed to register kmsg dumper\n");
        device_destroy(notifier_data->class, notifier_data->dev);
        class_destroy(notifier_data->class);
        cdev_del(&notifier_data->cdev);
        unregister_chrdev_region(notifier_data->dev, 1);
        kfree(notifier_data);
        return ret;
    }
    
    printk(KERN_INFO "Lua Notifier: Module initialized successfully\n");
    return 0;
}

static void __exit luanotifier_exit(void)
{
    printk(KERN_INFO "Lua Notifier: Cleaning up module\n");
    
    // Unregister kmsg dumper
    kmsg_dump_unregister(&lua_dumper);
    
    // Clean up device
    device_destroy(notifier_data->class, notifier_data->dev);
    class_destroy(notifier_data->class);
    cdev_del(&notifier_data->cdev);
    unregister_chrdev_region(notifier_data->dev, 1);
    kfree(notifier_data);
    
    printk(KERN_INFO "Lua Notifier: Module unloaded\n");
}

module_init(luanotifier_init);
module_exit(luanotifier_exit); 