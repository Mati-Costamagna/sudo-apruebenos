#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/slab.h>

#define DEVICE_NAME "SdC_cdd"
#define CLASS_NAME  "SdC_class"
#define NUM_SIGNALS 2

/* Rutas sysfs del ADC integrado de la BeagleBone Black */
static const char *adc_paths[NUM_SIGNALS] = {
    "/sys/bus/iio/devices/iio:device0/in_voltage0_raw",
    "/sys/bus/iio/devices/iio:device0/in_voltage1_raw",
};

static dev_t dev_num;
static struct cdev cdd_cdev;
static struct class *cdd_class;

static int current_signal = 0;
static int signal_values[NUM_SIGNALS];
static spinlock_t data_lock;
static struct timer_list sampling_timer;

static int read_adc_raw(int channel)
{
    struct file *f;
    char buf[16];
    loff_t pos = 0;
    ssize_t n;
    int val = 0;

    f = filp_open(adc_paths[channel], O_RDONLY, 0);
    if (IS_ERR(f))
        return -1;

    n = kernel_read(f, buf, sizeof(buf) - 1, &pos);
    filp_close(f, NULL);

    if (n <= 0)
        return -1;

    buf[n] = '\0';
    if (kstrtoint(strim(buf), 10, &val) != 0)
        return -1;

    return val;
}

static void timer_callback(struct timer_list *t)
{
    int raw0, raw1;

    raw0 = read_adc_raw(0);
    raw1 = read_adc_raw(1);

    spin_lock(&data_lock);
    if (raw0 >= 0)
        signal_values[0] = raw0 * 1800 / 4095;
    if (raw1 >= 0)
        signal_values[1] = raw1 * 1800 / 4095;
    spin_unlock(&data_lock);

    mod_timer(&sampling_timer, jiffies + HZ);
}

static int cdd_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t cdd_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char kbuf[32];
    int val, n;

    if (*offset > 0)
        return 0;

    spin_lock(&data_lock);
    val = signal_values[current_signal];
    spin_unlock(&data_lock);

    n = snprintf(kbuf, sizeof(kbuf), "%d\n", val);
    if (len < n)
        return -EINVAL;

    if (copy_to_user(buf, kbuf, n))
        return -EFAULT;

    *offset += n;
    return n;
}

static ssize_t cdd_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char kbuf[4];
    int sig;

    if (len == 0 || len > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    if (kstrtoint(strim(kbuf), 10, &sig) != 0)
        return -EINVAL;

    if (sig < 0 || sig >= NUM_SIGNALS)
        return -EINVAL;

    spin_lock(&data_lock);
    current_signal = sig;
    spin_unlock(&data_lock);

    pr_info(DEVICE_NAME ": señal seleccionada: %d\n", sig);
    return len;
}

static int cdd_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = cdd_open,
    .read    = cdd_read,
    .write   = cdd_write,
    .release = cdd_release,
};

static int __init cdd_init(void)
{
    int ret;

    spin_lock_init(&data_lock);
    signal_values[0] = 0;
    signal_values[1] = 0;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err(DEVICE_NAME ": error al registrar región de dispositivo: %d\n", ret);
        return ret;
    }

    cdev_init(&cdd_cdev, &fops);
    cdd_cdev.owner = THIS_MODULE;

    ret = cdev_add(&cdd_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err(DEVICE_NAME ": error al agregar cdev: %d\n", ret);
        goto err_cdev;
    }

    cdd_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(cdd_class)) {
        ret = PTR_ERR(cdd_class);
        pr_err(DEVICE_NAME ": error al crear clase: %d\n", ret);
        goto err_class;
    }

    if (IS_ERR(device_create(cdd_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        pr_err(DEVICE_NAME ": error al crear device\n");
        goto err_device;
    }

    timer_setup(&sampling_timer, timer_callback, 0);
    mod_timer(&sampling_timer, jiffies + HZ);

    pr_info(DEVICE_NAME ": inicializado, major=%d\n", MAJOR(dev_num));
    return 0;

err_device:
    class_destroy(cdd_class);
err_class:
    cdev_del(&cdd_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit cdd_exit(void)
{
    del_timer_sync(&sampling_timer);
    device_destroy(cdd_class, dev_num);
    class_destroy(cdd_class);
    cdev_del(&cdd_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info(DEVICE_NAME ": removido\n");
}

module_init(cdd_init);
module_exit(cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SdC");
MODULE_DESCRIPTION("Character Device Driver - ADC BeagleBone Black");
