#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#define DEVICE_NAME "SdeC_drv2"

/*
 * drv2 — Registro del par <major, minor>.
 *
 * alloc_chrdev_region solicita al kernel un número mayor dinámico y reserva
 * dos menores (0 y 1). El major queda visible en /proc/devices pero NO se
 * crea ningún archivo en /dev automáticamente: hay que hacerlo a mano con
 * mknod. Intentar cat/echo sobre esos nodos genera error porque todavía
 * no hay ningún cdev (ni file_operations) asociado al major.
 */

static dev_t dev_num;

static int __init drv2_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 2, DEVICE_NAME);
    if (ret < 0) {
        pr_err("SdeC_drv2: error al registrar región: %d\n", ret);
        return ret;
    }

    pr_info("SdeC_drv2: major=%d  (verificar con: cat /proc/devices)\n",
            MAJOR(dev_num));
    pr_info("SdeC_drv2: crear nodos manualmente:\n");
    pr_info("  sudo mknod /dev/drv2_0 c %d 0\n", MAJOR(dev_num));
    pr_info("  sudo mknod /dev/drv2_1 c %d 1\n", MAJOR(dev_num));
    pr_info("SdeC_drv2: luego probar cat/echo — aparece un error (esperado)\n");

    return 0;
}

static void __exit drv2_exit(void)
{
    unregister_chrdev_region(dev_num, 2);
    pr_info("SdeC_drv2: removido\n");
}

module_init(drv2_init);
module_exit(drv2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("drv2 - registro de major/minor; nodos /dev creados manualmente con mknod");
