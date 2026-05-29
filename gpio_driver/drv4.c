#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "SdeC_drv4"
#define CLASS_NAME  "SdeC_drv4_class"

/*
 * drv4 — read() y write() con transferencia real de datos.
 *
 * Agrega un buffer interno en espacio de kernel. write() copia datos
 * del espacio de usuario al buffer (copy_from_user). read() los devuelve
 * al usuario (copy_to_user). El device actúa como un portapapeles en /dev.
 *
 *   echo -n "H" > /dev/SdeC_drv4   → almacena "H" en el buffer
 *   cat /dev/SdeC_drv4             → imprime "H"
 *
 * Por qué copy_to_user / copy_from_user y no memcpy:
 *   Los punteros del espacio de usuario son virtuales y pueden apuntar
 *   a páginas no presentes, swapeadas o maliciosas. Estas funciones
 *   verifican el acceso y manejan los page faults correctamente.
 */

static char   kbuf[PAGE_SIZE];
static size_t kbuf_len = 0;

static dev_t dev_num;
static struct cdev drv4_cdev;
static struct class *drv4_class;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("SdeC_drv4: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("SdeC_drv4: release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf,
                        size_t len, loff_t *offset)
{
    size_t available;

    if (*offset >= kbuf_len)
        return 0; /* EOF */

    available = kbuf_len - (size_t)*offset;
    len = min(len, available);

    if (copy_to_user(buf, kbuf + *offset, len))
        return -EFAULT;

    *offset += len;
    pr_info("SdeC_drv4: read %zu bytes\n", len);
    return len;
}

static ssize_t my_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *offset)
{
    if (len > PAGE_SIZE - 1)
        len = PAGE_SIZE - 1;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';
    kbuf_len  = len;

    pr_info("SdeC_drv4: write %zu bytes: \"%s\"\n", len, kbuf);
    return len;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init drv4_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("SdeC_drv4: error al registrar región: %d\n", ret);
        return ret;
    }

    cdev_init(&drv4_cdev, &fops);
    drv4_cdev.owner = THIS_MODULE;

    ret = cdev_add(&drv4_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("SdeC_drv4: error al agregar cdev: %d\n", ret);
        goto err_cdev;
    }

    drv4_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(drv4_class)) {
        ret = PTR_ERR(drv4_class);
        goto err_class;
    }

    if (IS_ERR(device_create(drv4_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        goto err_device;
    }

    pr_info("SdeC_drv4: inicializado, major=%d\n", MAJOR(dev_num));
    return 0;

err_device:
    class_destroy(drv4_class);
err_class:
    cdev_del(&drv4_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit drv4_exit(void)
{
    device_destroy(drv4_class, dev_num);
    class_destroy(drv4_class);
    cdev_del(&drv4_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("SdeC_drv4: removido\n");
}

module_init(drv4_init);
module_exit(drv4_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("drv4 - read/write con copy_to_user/copy_from_user; buffer interno en kernel");
