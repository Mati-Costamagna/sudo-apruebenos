#include <linux/module.h>
#include <linux/kernel.h>

/*
 * drv1 — Módulo mínimo.
 *
 * Todo driver Linux tiene un constructor (module_init) y un destructor
 * (module_exit). El kernel los invoca al hacer insmod y rmmod respectivamente.
 * Este es el punto de partida antes de interactuar con ningún dispositivo.
 */

static int __init drv1_init(void)
{
    pr_info("SdeC_drv1: constructor — módulo cargado\n");
    return 0;
}

static void __exit drv1_exit(void)
{
    pr_info("SdeC_drv1: destructor — módulo removido\n");
}

module_init(drv1_init);
module_exit(drv1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("drv1 - módulo básico: constructor y destructor");
