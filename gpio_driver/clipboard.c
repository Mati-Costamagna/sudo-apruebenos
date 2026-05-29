#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#define PROC_NAME    "clipboard"
#define BUFFER_SIZE  256

/* Buffer interno que actúa como portapapeles */
static char clipboard_buf[BUFFER_SIZE];
static size_t clipboard_len = 0;

/*
 * clipboard_read - Envía el contenido del buffer al proceso lector.
 * Implementa semántica de EOF con offset: una vez leído todo, retorna 0.
 */
static ssize_t clipboard_read(struct file *file, char __user *buf,
                               size_t len, loff_t *offset)
{
    if (*offset >= clipboard_len)
        return 0; /* EOF */

    len = min(len, clipboard_len - (size_t)*offset);

    if (copy_to_user(buf, clipboard_buf + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

/*
 * clipboard_write - Almacena el texto recibido en el buffer.
 * Trunca silenciosamente si el mensaje supera BUFFER_SIZE - 1 bytes.
 */
static ssize_t clipboard_write(struct file *file, const char __user *buf,
                                size_t len, loff_t *offset)
{
    if (len == 0)
        return 0;

    /* Truncar al tamaño máximo del buffer dejando lugar para '\0' */
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (copy_from_user(clipboard_buf, buf, len))
        return -EFAULT;

    clipboard_buf[len] = '\0';
    clipboard_len = len;

    pr_info(PROC_NAME ": guardado %zu bytes\n", len);
    return len;
}

/*
 * proc_ops es la estructura recomendada desde kernel 5.6 en adelante para
 * entradas /proc. Reemplaza a file_operations para evitar overhead innecesario
 * de campos que no aplican a entradas procfs.
 */
static const struct proc_ops clipboard_fops = {
    .proc_read  = clipboard_read,
    .proc_write = clipboard_write,
};

static struct proc_dir_entry *proc_entry;

static int __init clipboard_init(void)
{
    /* Crea /proc/clipboard con permisos rw para todos (0666) */
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &clipboard_fops);
    if (!proc_entry) {
        pr_err(PROC_NAME ": error al crear entrada en /proc\n");
        return -ENOMEM;
    }

    pr_info(PROC_NAME ": módulo cargado — /proc/clipboard disponible\n");
    return 0;
}

static void __exit clipboard_exit(void)
{
    proc_remove(proc_entry);
    pr_info(PROC_NAME ": módulo removido\n");
}

module_init(clipboard_init);
module_exit(clipboard_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sudo_apruebenos");
MODULE_DESCRIPTION("Módulo /proc clipboard — ejemplo de procfs vs CDD");
