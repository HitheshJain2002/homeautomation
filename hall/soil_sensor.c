#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define BCM_GPIO       24
#define GPIO_DESC      "soil_sensor_gpio"
#define TIMER_INTERVAL 2000
#define DEVICE_NAME    "soil_device"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hithesh Jain");
MODULE_DESCRIPTION("Digital Soil Moisture Sensor with /dev interface");
MODULE_VERSION("1.0");

static struct timer_list soil_timer;
static int gpio_pin_number = 595;
static int last_value = -1;
static char status_str[8] = "Unknown";

static dev_t dev_number;
static struct cdev soil_cdev;
static struct class *soil_class;

static void read_sensor(struct timer_list *t) {
    int value = gpio_get_value(gpio_pin_number);

    if (last_value == -1) {
        printk(KERN_INFO "[SoilSensor] First reading: %s\n", value ? "Dry" : "Wet");
    }

    if (value != last_value) {
        last_value = value;
        snprintf(status_str, sizeof(status_str), "%s\n", value ? "Dry" : "Wet");
        printk(KERN_INFO "[SoilSensor] Status: %s\n", status_str);
    }

    mod_timer(&soil_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL));
}

// File operations
static ssize_t soil_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    size_t str_len = strlen(status_str);
    if (*offset >= str_len)
        return 0;

    if (copy_to_user(buf, status_str + *offset, str_len - *offset))
        return -EFAULT;

    *offset += str_len;
    return str_len;
}

static struct file_operations soil_fops = {
    .owner = THIS_MODULE,
    .read = soil_read,
};

static int __init soil_sensor_init(void) {
    int ret;

    printk(KERN_INFO "[SoilSensor] Module loading...\n");

    ret = gpio_request(gpio_pin_number, GPIO_DESC);
    if (ret) {
        printk(KERN_ERR "[SoilSensor] GPIO request failed: %d\n", ret);
        return ret;
    }

    ret = gpio_direction_input(gpio_pin_number);
    if (ret) {
        gpio_free(gpio_pin_number);
        printk(KERN_ERR "[SoilSensor] Failed to set GPIO input\n");
        return ret;
    }

    // Register char device
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "[SoilSensor] Failed to allocate char device region\n");
        return ret;
    }

    cdev_init(&soil_cdev, &soil_fops);
    ret = cdev_add(&soil_cdev, dev_number, 1);
    if (ret) {
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ERR "[SoilSensor] cdev_add failed\n");
        return ret;
    }

    soil_class = class_create("soil_class");

    if (IS_ERR(soil_class)) {
        cdev_del(&soil_cdev);
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ERR "[SoilSensor] class_create failed\n");
        return PTR_ERR(soil_class);
    }

    device_create(soil_class, NULL, dev_number, NULL, DEVICE_NAME);

    timer_setup(&soil_timer, read_sensor, 0);
    mod_timer(&soil_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL));

    printk(KERN_INFO "[SoilSensor] Initialized and ready!\n");
    return 0;
}

static void __exit soil_sensor_exit(void) {
    del_timer_sync(&soil_timer);
    gpio_free(gpio_pin_number);
    device_destroy(soil_class, dev_number);
    class_destroy(soil_class);
    cdev_del(&soil_cdev);
    unregister_chrdev_region(dev_number, 1);
    printk(KERN_INFO "[SoilSensor] Module unloaded\n");
}

module_init(soil_sensor_init);
module_exit(soil_sensor_exit);
