#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grupo 5");
MODULE_DESCRIPTION("Driver de Min Heap");
MODULE_VERSION("2.1");

#define DEVICE_NAME "minheap_device"
#define CLASS_NAME "minheap_class"
#define MAX_SIZE 100

static int major;
static struct class* cls;
static struct device* dev;
static int heap[MAX_SIZE];
static int size = 0;
static DEFINE_MUTEX(lock);

// --- Funções Auxiliares da Heap ---
static void h_swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static void heap_up(int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;

        if (heap[parent] <= heap[i])
            break;

        h_swap(&heap[i], &heap[parent]);
        i = parent;
    }
}

static void heap_down(int i) {
    while (true) {
        int s = i;
        int l = 2 * i + 1;
        int r = 2 * i + 2;

        if (l < size && heap[l] < heap[s]) s = l;
        if (r < size && heap[r] < heap[s]) s = r;
        if (s == i) break;

        h_swap(&heap[i], &heap[s]);
        i = s;
    }
}

static int internal_insert(int val) {
    if (size >= MAX_SIZE) return -ENOMEM;
    heap[size++] = val;
    heap_up(size-1);
    return 0;
}

static int internal_extract(int *val) {
    if (size <= 0) return -EINVAL;
    *val = heap[0];
    heap[0] = heap[--size];
    heap_down(0);
    return 0;
}

// --- Operações de Arquivo (User Space <-> Kernel) ---
static ssize_t dev_write(struct file *f, const char *buf, size_t len, loff_t *off) {
    char kbuf[32];
    int val;

    // 1. Copia a string do usuário
    if (len > 31 || copy_from_user(kbuf, buf, len)) return -EFAULT;
    kbuf[len] = 0;

    // 2. Converte para inteiro
    if (kstrtoint(kbuf, 10, &val) < 0) return -EINVAL;

    // 3. Insere na Heap ou limpa caso -1
    mutex_lock(&lock);
    if (val == -1) {
    	size = 0;
    	printk(KERN_INFO "MinHeap: Heap foi Limpa!");
    } else if (internal_insert(val) == 0) {
        printk(KERN_INFO "MinHeap: Inserido %d. Total: %d\n", val, size);
    } else {
        printk(KERN_ALERT "MinHeap: Heap Cheia!\n");
        mutex_unlock(&lock);
        return -ENOMEM;
    }
    mutex_unlock(&lock);

    return len;
}

static ssize_t dev_read(struct file *f, char *buf, size_t len, loff_t *off) {
    char kbuf[32];
    int val, l;

    if (*off > 0) return 0; // EOF (Fim de arquivo)

    mutex_lock(&lock);
    if (internal_extract(&val) != 0) {
        printk(KERN_INFO "MinHeap: Heap Vazia!\n");
        mutex_unlock(&lock);
        return 0;
    }
    mutex_unlock(&lock);

    // Formata para string e envia ao usuário
    l = snprintf(kbuf, 32, "%d\n", val);
    if (copy_to_user(buf, kbuf, l)) return -EFAULT;

    *off += l;
    printk(KERN_INFO "MinHeap: Removido %d\n", val);
    return l;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = dev_write,
    .read = dev_read
};

// --- Inicialização e Saída ---
static int __init my_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) return major;

    cls = class_create(CLASS_NAME);
    if (IS_ERR(cls)) { unregister_chrdev(major, DEVICE_NAME); return PTR_ERR(cls); }

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev)) { class_destroy(cls); unregister_chrdev(major, DEVICE_NAME); return PTR_ERR(dev); }

    printk(KERN_INFO "MinHeap: Driver carregado!\n");
    return 0;
}

static void __exit my_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "MinHeap: Driver descarregado.\n");
}

module_init(my_init);
module_exit(my_exit);
