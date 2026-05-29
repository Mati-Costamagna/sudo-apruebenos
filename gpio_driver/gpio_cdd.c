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
#include <linux/io.h>       /* ioremap, iounmap, readl, writel */

#define DEVICE_NAME "SdC_gpio"
#define CLASS_NAME  "SdC_gpio_class"
#define NUM_PINS    2

/*
 * BeagleBone Black — AM335x GPIO
 *
 * El SoC AM335x tiene cuatro bancos GPIO independientes. Cada banco controla
 * hasta 32 pines y tiene su propio bloque de registros en el espacio físico.
 * El acceso es idéntico al de cualquier periférico mapeado en memoria:
 * ioremap() devuelve un puntero virtual; readl()/writel() leen y escriben
 * con las barreras de memoria necesarias en ARM.
 *
 * Banco  │ Base física │ Pines Linux │ Header BBB (ejemplos)
 * ───────┼─────────────┼─────────────┼──────────────────────
 * GPIO0  │ 0x44E07000  │   0 –  31   │ P9_11 (GPIO0[30]), P9_13 (GPIO0[31])
 * GPIO1  │ 0x4804C000  │  32 –  63   │ P9_12 (GPIO1[28]), P9_14 (GPIO1[18])
 * GPIO2  │ 0x481AC000  │  64 –  95   │ P8_7  (GPIO2[2]),  P8_8  (GPIO2[3])
 * GPIO3  │ 0x481AE000  │  96 – 127   │ P9_25 (GPIO3[21]), P9_27 (GPIO3[19])
 *
 * Número Linux de un GPIO: bank * 32 + bit
 *   Ejemplo: GPIO1[28] → 1*32 + 28 = 60
 */
static const unsigned long gpio_bank_bases[] = {
    0x44E07000UL,  /* GPIO0 */
    0x4804C000UL,  /* GPIO1 */
    0x481AC000UL,  /* GPIO2 */
    0x481AE000UL,  /* GPIO3 */
};
#define NUM_BANKS        ARRAY_SIZE(gpio_bank_bases)
#define GPIO_BLOCK_SIZE  0x1000

/*
 * Registros GPIO del AM335x (offsets desde la base del banco):
 *
 * GPIO_OE     — Output Enable.
 *               bit = 1 → pin es ENTRADA (deshabilita el driver de salida)
 *               bit = 0 → pin es salida
 *               Nota: la lógica es inversa al nombre — un '1' en OE significa
 *               que la salida está DESHABILITADA, es decir el pin es entrada.
 *
 * GPIO_DATAIN — Nivel lógico actual de cada pin (solo lectura).
 *               Válido tanto para pines configurados como entrada como salida.
 */
#define GPIO_OE      0x134
#define GPIO_DATAIN  0x138

/*
 * Parámetros configurables en tiempo de carga (insmod gpio_cdd.ko param=val).
 *
 * gpio_bank : banco GPIO (0–3). Default 1 → GPIO1.
 * gpio_bits : bit dentro del banco para cada canal.
 *             Default {28, 18} → P9_12 (GPIO1[28]) y P9_14 (GPIO1[18]).
 * sample_ms : período de muestreo en ms. Default 100 ms (10 Hz).
 *             Para pulsos lentos se puede bajar a 10 ms, pero el timer del
 *             kernel no es tiempo real — usar interrupciones para pulsos rápidos.
 */
static int gpio_bank = 1;
module_param(gpio_bank, int, 0444);
MODULE_PARM_DESC(gpio_bank, "Banco GPIO (0-3, default 1 → GPIO1)");

static int gpio_bits[NUM_PINS] = {28, 18};
module_param_array(gpio_bits, int, NULL, 0444);
MODULE_PARM_DESC(gpio_bits, "Bits dentro del banco para cada canal (default: 28,18 → P9_12,P9_14)");

static int sample_ms = 100;
module_param(sample_ms, int, 0444);
MODULE_PARM_DESC(sample_ms, "Período de muestreo en ms (default: 100)");

static void __iomem *gpio_regs;

static dev_t dev_num;
static struct cdev gpio_cdev;
static struct class *gpio_class;

static int current_pin = 0;
static int pin_states[NUM_PINS];
static spinlock_t data_lock;
static struct timer_list sampling_timer;

/*
 * set_gpio_inputs — Configura los pines monitoreados como entradas.
 *
 * Lee GPIO_OE, pone en 1 los bits correspondientes a los pines del driver
 * y lo escribe de vuelta. El OR garantiza que no se tocan otros bits del
 * banco que puedan estar en uso por otro subsistema del kernel.
 */
static void set_gpio_inputs(void)
{
    u32 oe = readl(gpio_regs + GPIO_OE);
    int i;

    for (i = 0; i < NUM_PINS; i++)
        oe |= (1u << gpio_bits[i]);

    writel(oe, gpio_regs + GPIO_OE);
}

/*
 * read_gpio_bit — Lee el nivel lógico de un pin desde GPIO_DATAIN.
 * Retorna 1 (HIGH) o 0 (LOW).
 */
static int read_gpio_bit(int bit)
{
    return (readl(gpio_regs + GPIO_DATAIN) >> bit) & 1;
}

/*
 * timer_callback — Muestreo periódico de los pines.
 * El spinlock protege pin_states[] frente a lecturas concurrentes de my_read().
 */
static void timer_callback(struct timer_list *t)
{
    int i;

    spin_lock(&data_lock);
    for (i = 0; i < NUM_PINS; i++)
        pin_states[i] = read_gpio_bit(gpio_bits[i]);
    spin_unlock(&data_lock);

    mod_timer(&sampling_timer, jiffies + msecs_to_jiffies(sample_ms));
}

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("SdC_gpio: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("SdC_gpio: release\n");
    return 0;
}

/*
 * my_read — Devuelve el estado del canal activo como "0\n" o "1\n".
 * Misma semántica de EOF que drv4: offset > 0 retorna 0 para que cat funcione.
 */
static ssize_t my_read(struct file *file, char __user *buf,
                        size_t len, loff_t *offset)
{
    char kbuf[8];
    int state, n;

    if (*offset > 0)
        return 0;

    spin_lock(&data_lock);
    state = pin_states[current_pin];
    spin_unlock(&data_lock);

    n = snprintf(kbuf, sizeof(kbuf), "%d\n", state);
    if (len < n)
        return -EINVAL;

    if (copy_to_user(buf, kbuf, n))
        return -EFAULT;

    *offset += n;
    pr_info("SdC_gpio: read GPIO%d[%d] → %d\n",
            gpio_bank, gpio_bits[current_pin], state);
    return n;
}

/*
 * my_write — Selecciona el canal activo (0 o 1).
 *   echo 0 > /dev/SdC_gpio  →  monitorea gpio_bits[0]
 *   echo 1 > /dev/SdC_gpio  →  monitorea gpio_bits[1]
 */
static ssize_t my_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    char kbuf[4];
    int pin;

    if (len == 0 || len > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    if (kstrtoint(strim(kbuf), 10, &pin) != 0)
        return -EINVAL;

    if (pin < 0 || pin >= NUM_PINS)
        return -EINVAL;

    spin_lock(&data_lock);
    current_pin = pin;
    spin_unlock(&data_lock);

    pr_info("SdC_gpio: canal seleccionado: %d (GPIO%d[%d])\n",
            pin, gpio_bank, gpio_bits[pin]);
    return len;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

/*
 * gpio_cdd_init — Inicialización.
 *
 * Igual que drv4 pero con hardware real:
 *   1. Valida los parámetros de módulo.
 *   2. ioremap() mapea el banco GPIO seleccionado al espacio virtual del kernel.
 *   3. set_gpio_inputs() pone los pines en modo entrada (GPIO_OE).
 *   4. Registra el CDD (major dinámico, cdev, clase, nodo /dev).
 *   5. Arranca el timer de muestreo.
 *
 * Prerequisito: los pines deben estar en modo GPIO antes de cargar el módulo.
 * Configurar con config-pin (ver README).
 */
static int __init gpio_cdd_init(void)
{
    int ret, i;

    if (gpio_bank < 0 || gpio_bank >= (int)NUM_BANKS) {
        pr_err("SdC_gpio: gpio_bank %d inválido (0-%zu)\n",
               gpio_bank, NUM_BANKS - 1);
        return -EINVAL;
    }
    for (i = 0; i < NUM_PINS; i++) {
        if (gpio_bits[i] < 0 || gpio_bits[i] > 31) {
            pr_err("SdC_gpio: gpio_bits[%d]=%d inválido (0-31)\n",
                   i, gpio_bits[i]);
            return -EINVAL;
        }
    }
    if (sample_ms <= 0) {
        pr_err("SdC_gpio: sample_ms debe ser > 0\n");
        return -EINVAL;
    }

    gpio_regs = ioremap(gpio_bank_bases[gpio_bank], GPIO_BLOCK_SIZE);
    if (!gpio_regs) {
        pr_err("SdC_gpio: ioremap falló para GPIO%d (0x%lx)\n",
               gpio_bank, gpio_bank_bases[gpio_bank]);
        return -ENOMEM;
    }

    set_gpio_inputs();
    spin_lock_init(&data_lock);
    pin_states[0] = pin_states[1] = 0;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("SdC_gpio: error al registrar región: %d\n", ret);
        goto err_region;
    }

    cdev_init(&gpio_cdev, &fops);
    gpio_cdev.owner = THIS_MODULE;

    ret = cdev_add(&gpio_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("SdC_gpio: error al agregar cdev: %d\n", ret);
        goto err_cdev;
    }

    gpio_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(gpio_class)) {
        ret = PTR_ERR(gpio_class);
        goto err_class;
    }

    if (IS_ERR(device_create(gpio_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        goto err_device;
    }

    timer_setup(&sampling_timer, timer_callback, 0);
    mod_timer(&sampling_timer, jiffies + msecs_to_jiffies(sample_ms));

    pr_info("SdC_gpio: inicializado — major=%d, GPIO%d[%d,%d], T=%dms\n",
            MAJOR(dev_num), gpio_bank, gpio_bits[0], gpio_bits[1], sample_ms);
    return 0;

err_device:
    class_destroy(gpio_class);
err_class:
    cdev_del(&gpio_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
err_region:
    iounmap(gpio_regs);
    return ret;
}

/*
 * gpio_cdd_exit — Limpieza en orden inverso.
 * del_timer_sync espera que el callback no esté ejecutándose antes de
 * iounmap para evitar un use-after-free sobre gpio_regs.
 */
static void __exit gpio_cdd_exit(void)
{
    del_timer_sync(&sampling_timer);
    device_destroy(gpio_class, dev_num);
    class_destroy(gpio_class);
    cdev_del(&gpio_cdev);
    unregister_chrdev_region(dev_num, 1);
    iounmap(gpio_regs);
    pr_info("SdC_gpio: removido\n");
}

module_init(gpio_cdd_init);
module_exit(gpio_cdd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("gpio_cdd - CDD con GPIO mapeado en memoria (BeagleBone Black / AM335x)");
