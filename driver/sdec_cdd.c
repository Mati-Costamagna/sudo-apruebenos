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

#define DEVICE_NAME "SdC_cdd"   /* Nombre del dispositivo en /dev */
#define CLASS_NAME  "SdC_class" /* Nombre de la clase en /sys/class */
#define NUM_SIGNALS 2           /* Cantidad de canales ADC soportados */

/* Rutas sysfs del ADC integrado de la BeagleBone Black.
 * El subsistema IIO (Industrial I/O) expone los valores crudos del ADC
 * como archivos de texto en estas rutas. */
static const char *adc_paths[NUM_SIGNALS] = {
    "/sys/bus/iio/devices/iio:device0/in_voltage0_raw", /* Canal AIN0 */
    "/sys/bus/iio/devices/iio:device0/in_voltage1_raw", /* Canal AIN1 */
};

/* Variables globales del módulo */
static dev_t dev_num;              /* Número de dispositivo (major + minor) asignado dinámicamente */
static struct cdev cdd_cdev;       /* Estructura del character device */
static struct class *cdd_class;    /* Clase del dispositivo para udev/sysfs */

static int current_signal = 0;        /* Canal ADC activo (0 o 1), seleccionable via write() */
static int signal_values[NUM_SIGNALS]; /* Últimas lecturas convertidas a mV de cada canal */
static spinlock_t data_lock;          /* Protege acceso concurrente a signal_values y current_signal */
static struct timer_list sampling_timer; /* Timer periódico de muestreo (1 Hz) */

/*
 * read_adc_raw - Lee el valor crudo del ADC desde sysfs.
 * @channel: índice del canal (0 o 1).
 * Retorna el entero leído (0–4095) o -1 en caso de error.
 *
 * Abre el archivo sysfs del canal, lee el string con el valor crudo
 * y lo convierte a entero mediante kstrtoint.
 */
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
    /* strim elimina espacios y newlines; kstrtoint convierte el string a int */
    if (kstrtoint(strim(buf), 10, &val) != 0)
        return -1;

    return val;
}

/*
 * timer_callback - Función disparada cada 1 segundo por el timer de muestreo.
 * Lee ambos canales ADC y convierte los valores crudos a milivolts.
 *
 * Conversión: ADC de 12 bits (0–4095), referencia 1.8 V → mV = raw * 1800 / 4095
 *
 * Los resultados se escriben bajo spinlock para evitar lecturas inconsistentes
 * desde cdd_read(), que puede ejecutarse concurrentemente en otro contexto.
 * Al final se reprograma el timer para el siguiente segundo (jiffies + HZ).
 */
static void timer_callback(struct timer_list *t)
{
    int raw0, raw1;

    raw0 = read_adc_raw(0);
    raw1 = read_adc_raw(1);

    spin_lock(&data_lock);
    if (raw0 >= 0)
        signal_values[0] = raw0 * 1800 / 4095; /* Convierte raw a milivolts */
    if (raw1 >= 0)
        signal_values[1] = raw1 * 1800 / 4095;
    spin_unlock(&data_lock);

    /* Reprograma el timer para dispararse en exactamente 1 segundo */
    mod_timer(&sampling_timer, jiffies + HZ);
}

/* cdd_open - Llamado al abrir /dev/SdC_cdd. No requiere inicialización adicional. */
static int cdd_open(struct inode *inode, struct file *file)
{
    return 0;
}

/*
 * cdd_read - Devuelve al usuario el valor en mV del canal activo.
 * El valor se formatea como string decimal seguido de '\n'.
 * Solo permite una lectura por apertura (offset > 0 retorna EOF),
 * lo que hace compatible el driver con herramientas como cat.
 */
static ssize_t cdd_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char kbuf[32];
    int val, n;

    /* Semántica de EOF: una vez leído el valor, sucesivas lecturas devuelven 0 */
    if (*offset > 0)
        return 0;

    spin_lock(&data_lock);
    val = signal_values[current_signal];
    spin_unlock(&data_lock);

    n = snprintf(kbuf, sizeof(kbuf), "%d\n", val);
    if (len < n)
        return -EINVAL;

    /* copy_to_user copia de espacio kernel a espacio usuario de forma segura */
    if (copy_to_user(buf, kbuf, n))
        return -EFAULT;

    *offset += n;
    return n;
}

/*
 * cdd_write - Permite al usuario seleccionar el canal ADC activo.
 * El usuario escribe "0" o "1" en /dev/SdC_cdd para elegir el canal.
 * Ejemplo: echo 1 > /dev/SdC_cdd  →  selecciona AIN1
 * Cualquier valor fuera de rango [0, NUM_SIGNALS) devuelve -EINVAL.
 */
static ssize_t cdd_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char kbuf[4];
    int sig;

    if (len == 0 || len > sizeof(kbuf) - 1)
        return -EINVAL;

    /* copy_from_user copia de espacio usuario a espacio kernel de forma segura */
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

/* cdd_release - Llamado al cerrar el descriptor. No requiere limpieza. */
static int cdd_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Tabla de operaciones del character device: enlaza las syscalls del VFS
 * (open, read, write, close) con las funciones de este driver. */
static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = cdd_open,
    .read    = cdd_read,
    .write   = cdd_write,
    .release = cdd_release,
};

/*
 * cdd_init - Punto de entrada del módulo (insmod).
 * Secuencia de inicialización:
 *   1. Inicializa spinlock y valores ADC en cero.
 *   2. Registra un rango de números de dispositivo (major dinámico).
 *   3. Inicializa y agrega el cdev al kernel (vincula fops).
 *   4. Crea la clase y el nodo en /dev para que udev lo exponga.
 *   5. Arranca el timer de muestreo con primer disparo en 1 segundo.
 * En caso de error en cualquier paso, deshace los pasos anteriores (goto).
 */
static int __init cdd_init(void)
{
    int ret;

    spin_lock_init(&data_lock);
    signal_values[0] = 0;
    signal_values[1] = 0;

    /* Solicita un major number dinámico al kernel */
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

    /* class_create registra la clase en /sys/class para que udev genere /dev/SdC_cdd */
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

    /* Configura e inicia el timer de muestreo periódico cada 1 segundo */
    timer_setup(&sampling_timer, timer_callback, 0);
    mod_timer(&sampling_timer, jiffies + HZ);

    pr_info(DEVICE_NAME ": inicializado, major=%d\n", MAJOR(dev_num));
    return 0;

/* Limpieza en orden inverso ante fallos parciales durante la inicialización */
err_device:
    class_destroy(cdd_class);
err_class:
    cdev_del(&cdd_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

/*
 * cdd_exit - Punto de salida del módulo (rmmod).
 * Deshace toda la inicialización en orden inverso.
 * del_timer_sync espera a que el callback en curso termine antes de continuar,
 * evitando un use-after-free al liberar la memoria del módulo.
 */
static void __exit cdd_exit(void)
{
    del_timer_sync(&sampling_timer); /* Espera que el timer no esté en ejecución */
    device_destroy(cdd_class, dev_num);
    class_destroy(cdd_class);
    cdev_del(&cdd_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info(DEVICE_NAME ": removido\n");
}

module_init(cdd_init);
module_exit(cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("Character Device Driver - ADC BeagleBone Black");
