#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "SdeC_drv3"
#define CLASS_NAME  "SdeC_drv3_class"

/*
 * drv3 — Vinculación de las operaciones del CDF con el CDD.
 *
 * Agrega dos cosas respecto a drv2:
 *   1. Un cdev con file_operations que vincula open/read/write/release
 *      a funciones reales del driver.
 *   2. class_create + device_create: udev crea /dev/SdeC_drv3
 *      automáticamente sin necesitar mknod.
 *
 * Las funciones de este paso solo loguean en dmesg; no transfieren datos.
 * cat /dev/SdeC_drv3 no produce salida porque read() retorna 0 (EOF).
 * echo "..." > /dev/SdeC_drv3 no falla, pero los datos se descartan.
 * Observar el comportamiento con: dmesg | tail
 */

static dev_t dev_num;
static struct cdev drv3_cdev;
static struct class *drv3_class;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("SdeC_drv3: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("SdeC_drv3: release\n");
    return 0;
}

/*
 * my_read — retorna 0 (EOF) inmediatamente.
 * cat lo interpreta como "no hay datos" y termina sin imprimir nada.
 * El valor de retorno de read() es ssize_t: positivo = bytes leídos,
 * 0 = EOF, negativo = error.
 */
static ssize_t my_read(struct file *file, char __user *buf,
                        size_t len, loff_t *offset)
{
    pr_info("SdeC_drv3: read — len=%zu offset=%lld → retorna 0 (EOF)\n",
            len, *offset);
    return 0;
}

/*
 * my_write — acepta los datos pero los descarta.
 * Retorna len para indicar que todos los bytes fueron "consumidos".
 * Si retornara 0 o negativo, el shell mostraría un error al hacer echo.
 */
static ssize_t my_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *offset)
{
    pr_info("SdeC_drv3: write — len=%zu → datos descartados\n", len);
    return len;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init drv3_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("SdeC_drv3: error al registrar región: %d\n", ret);
        return ret;
    }

    cdev_init(&drv3_cdev, &fops);
    drv3_cdev.owner = THIS_MODULE;

    ret = cdev_add(&drv3_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("SdeC_drv3: error al agregar cdev: %d\n", ret);
        goto err_cdev;
    }

    drv3_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(drv3_class)) {
        ret = PTR_ERR(drv3_class);
        goto err_class;
    }

    if (IS_ERR(device_create(drv3_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        goto err_device;
    }

    pr_info("SdeC_drv3: inicializado, major=%d — /dev/SdeC_drv3 creado por udev\n",
            MAJOR(dev_num));
    return 0;

err_device:
    class_destroy(drv3_class);
err_class:
    cdev_del(&drv3_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit drv3_exit(void)
{
    device_destroy(drv3_class, dev_num);
    class_destroy(drv3_class);
    cdev_del(&drv3_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("SdeC_drv3: removido\n");
}

module_init(drv3_init);
module_exit(drv3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("drv3 - file_operations básicas; operaciones loguean en dmesg sin transferir datos");
