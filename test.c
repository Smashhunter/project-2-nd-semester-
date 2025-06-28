#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Savely");

/* Buffer for data */
#define BUF_LEN 255
static char buffer[BUF_LEN];
static size_t buffer_pointer = 0;

/* Variables for device and device class */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "MyModule"
#define DRIVER_CLASS "MyModuleClass"

/**
 * @brief Read data out of the buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offset) {
    int to_copy, not_copied;

    /* Get amount of data to copy */
    to_copy = min(count, buffer_pointer - *offset);

    /* Copy data to user */
    not_copied = copy_to_user(user_buffer, buffer + *offset, to_copy);
    
    /* Calculate data */
    *offset += to_copy;

    return to_copy - not_copied;
}

/**
 * @brief Write data to buffer - creates greeting message
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offset) {
    int to_copy, not_copied;
    char name[BUF_LEN];

    /* Get amount of data to copy */
    to_copy = min(count, BUF_LEN-1);

    /* Copy data from user */
    not_copied = copy_from_user(name, user_buffer, to_copy);
    name[to_copy] = '\0';

    /* Create greeting message */
    buffer_pointer = snprintf(buffer, BUF_LEN, "Hello, %s!", name);

    return to_copy - not_copied;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_open(struct inode *device_file, struct file *instance) {
    printk("MyModule: open was called!\n");
    return 0;
}

/**
 * @brief This function is called, when the device file is closed
 */
static int driver_close(struct inode *device_file, struct file *instance) {
    printk("MyModule: close was called!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init ModuleInit(void) {
    printk("Hello, Kernel!\n");

    /* Allocate a device nr */
    if( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
        printk("Device Nr. could not be allocated!\n");
        return -1;
    }
    printk("read_write - Device Nr. Major: %d, Minor: %d was registered!\n", my_device_nr >> 20, my_device_nr & 0xfffff);

    /* Create device class */
    my_class = class_create(DRIVER_CLASS);
    if(my_class == NULL) {
        printk("Device class can not be created!\n");
        goto ClassError;
    }

    /* create device file */
    if(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
        printk("Can not create device file!\n");
        goto FileError;
    }

    /* Initialize device file */
    cdev_init(&my_device, &fops);

    /* Regisering device to kernel */
    if(cdev_add(&my_device, my_device_nr, 1) == -1) {
        printk("Registering of device to kernel failed!\n");
        goto AddError;
    }

    return 0;
AddError:
    device_destroy(my_class, my_device_nr);
FileError:
    class_destroy(my_class);
ClassError:
    unregister_chrdev_region(my_device_nr, 1);
    return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);
    printk("Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
