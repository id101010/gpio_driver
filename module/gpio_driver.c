/*
 * gpio loadable kernel module for raspberry pi devices
 */

// includes
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h> 
#include <linux/fs.h>
#include <asm/uaccess.h>  
#include <asm/gpio.h>

// macros and Constants
#define DEV_NODE_NAME "gpio%d"
#define MOD_LICENSE "GPL"
#define MOD_AUTHOR  "Aaron Schmocker <schmocker@puzzle.ch>"
#define MOD_DESCRIPTION "gpio driver loadable kernel module"
#define MODULE_NAME "gpio_driver"
#define LED_DRIVER_VER "1.0"
#define FIRTS_MINOR_NR 0
#define N_MINORS 4
#define MAX_MSG_SIZE 32
#define ON 0
#define OFF 1

// gpio led numnbers
#define LED_1 61
#define LED_2 44
#define LED_3 68
#define LED_4 67

// variables
uint16_t leds[N_MINORS]  = { LED_1, LED_2, LED_3, LED_4 };
static dev_t first_dev;
static struct cdev char_dev;
static struct class *dev_class;

// file open
static int led_open(struct inode *inode, struct file *file_ptr)
{
    int major, minor;

    major = imajor(inode);
    minor = iminor(inode);
    printk(KERN_INFO "Opening device node at major %d minor %d\n", major, minor);
    return 0;
}

// file close operations
static int led_close(struct inode *inode, struct file *file_ptr)
{
    int major, minor;

    /* get major and minor number */
    major = imajor(inode);
    minor = iminor(inode);
    printk(KERN_INFO "Closing device node at major %d minor %d\n", major, minor);
    return 0;
}

// file read operations
static ssize_t gpio_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *f_pos)
{
    size_t len = count, state_len;
    ssize_t retval = 0;
    unsigned long ret = 0;
    int major, minor;
    char led_value;
    char *state;

    /* get major and minor number */
    major = MAJOR(file_ptr->f_path.dentry->d_inode->i_rdev);
    minor = MINOR(file_ptr->f_path.dentry->d_inode->i_rdev);

    /* get led status */
    led_value = gpio_get_value(leds[minor]);
    if (!led_value) {
        state = "\non\n";
    } else {
        state = "\noff\n";
    }
    state_len = strlen(state);

    /* check and adjust limits */
    if (*f_pos >= state_len) {
        return 0;
    }
    if (*f_pos + count > state_len) {
        len = state_len - *f_pos;
    }

    /* copy message to user */
    ret = copy_to_user(user_buffer, state, len);
    *f_pos += len - ret;
    retval = len - ret;
    return retval;
}

// file write operations
static ssize_t gpio_write(struct file *file_ptr, const char __user *user_buffer, size_t size, loff_t *pos)
{
    int major, minor;
    char buf[MAX_MSG_SIZE];
    char led_value;

    /* get major and minor number */
    major = MAJOR(file_ptr->f_path.dentry->d_inode->i_rdev);
    minor = MINOR(file_ptr->f_path.dentry->d_inode->i_rdev);

    /* check size of user message */
    if (size > MAX_MSG_SIZE) {
        return -EINVAL;
    }

    if (copy_from_user(buf, user_buffer, size) != 0) {
        return -EFAULT;
    }

    /* get command for the leds from the user  */
    if (strncmp(buf, "on", strlen("on")) == 0) {
        led_value = ON;
        printk(KERN_INFO "Info: LED1 on!\n");
    } else if (strncmp(buf, "off", strlen("off")) == 0) {
        led_value = OFF;
        printk(KERN_INFO "Info: LED1 off!\n");
    } else {
        return size;
    }

    /* turn led on or off */
    gpio_set_value(leds[minor], led_value);
    return size ;
}

// file operations implemented by this driver
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_close,
    .read = gpio_read,
    .write = gpio_write
};

// module constructor
static int __init led_init(void)
{
    int ret,i;
    struct device *dev_ret;

    /* inform user */
    printk(KERN_INFO "\nInfo: gpio driver registered!\n");
    printk(KERN_INFO "Info: %s V%s\n", MODULE_NAME, LED_DRIVER_VER);

    /* try to request the gpios */
    for (i=0; i<N_MINORS; i++) {
        if ((ret = gpio_request(leds[i], "LED%d")) < 0) {
            return ret;
        }
    }

    /* Set the gpio of L1 on the BBB-BFH-CAPE as output */
    for (i=0; i<N_MINORS; i++) {
        if ((ret = gpio_direction_output(leds[i], OFF)) < 0) {
            return ret;
        }
    }

    /* Allocates a range of char device numbers */
    if ((ret = alloc_chrdev_region(&first_dev, FIRTS_MINOR_NR, N_MINORS, MODULE_NAME)) < 0) {
        return ret;
    }

    /* Create a struct class pointer to be used in device_create() */
    if (IS_ERR(dev_class = class_create(THIS_MODULE, MODULE_NAME))) {
        unregister_chrdev_region(first_dev, N_MINORS);
        return PTR_ERR(dev_class);
    }

    /* Create device in sysfs and register it to the specified class */
    for (i=0; i<N_MINORS; i++) {
        if (IS_ERR(dev_ret = device_create(dev_class, NULL, MKDEV(MAJOR(first_dev), MINOR(first_dev) + i), NULL, DEV_NODE_NAME, i))) {
            class_destroy(dev_class);
            unregister_chrdev_region(first_dev, N_MINORS);
            return PTR_ERR(dev_ret);
        }
    }
    /* Initializes cdev, remembering fops, making it ready to add to the system */
    cdev_init(&char_dev, &led_fops);

    /* Adds the device represented by char_dev to the system */
    if ((ret = cdev_add(&char_dev, first_dev, N_MINORS)) < 0) {
        device_destroy(dev_class, first_dev);
        class_destroy(dev_class);
        unregister_chrdev_region(first_dev, N_MINORS);
        return ret;
    }
    return 0;
}

// Destructor
static void __exit led_exit(void)
{
    int i;

    /* Remove char_dev from the system */
    cdev_del(&char_dev);

    /* Unregisters and cleans up devices created with a call to device_create */
    for (i=0; i<N_MINORS; i++) {
        device_destroy(dev_class, MKDEV(MAJOR(first_dev), MINOR(first_dev) + i));
    }

    /* Destroy an existing class */
    class_destroy(dev_class);

    /* Unregister a range of device numbers */
    unregister_chrdev_region(first_dev, N_MINORS);

    /* Release previously requested gpios */
    for (i=0; i<N_MINORS; i++) {
        gpio_free(leds[i]);
    }

    /* Inform user */
    printk(KERN_INFO "\nInfo: gpio driver unregistered!\n");
}

// Driver init and exit entry points
module_init(led_init);
module_exit(led_exit);

// Module info
MODULE_LICENSE(MOD_LICENSE);
MODULE_DESCRIPTION(MOD_DESCRIPTION);
MODULE_AUTHOR(MOD_AUTHOR);
