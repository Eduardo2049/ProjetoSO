#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "minheap_device"
#define CLASS_NAME "minheap_class"
#define HEAP_MAX_SIZE 1024
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grupo 5");
MODULE_DESCRIPTION("Um driver de Min Heap para Linux");
MODULE_VERSION("2.0");

static int majorNumber;
static struct class* minheapClass  = NULL;
static struct device* minheapDevice = NULL;

static int heap[HEAP_MAX_SIZE];
static int heap_size = 0;

static DEFINE_MUTEX(minheap_mutex);

static void heap_swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

static void heapify_up(int index) {
    if (index && heap[(index - 1) / 2] > heap[index]) {
        heap_swap(&heap[index], &heap[(index - 1) / 2]);
        heapify_up((index - 1) / 2);
    }
}

static void heapify_down(int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap_size && heap[left] < heap[smallest])
        smallest = left;

    if (right < heap_size && heap[right] < heap[smallest])
        smallest = right;

    if (smallest != index) {
        heap_swap(&heap[index], &heap[smallest]);
        heapify_down(smallest);
    }
}

static int insert_value(int value) {
    if (heap_size >= HEAP_MAX_SIZE) {
        return -1;
    }
    heap[heap_size++] = value;
    heapify_up(heap_size - 1);
    return 0;
}

static int extract_min(int *value) {
    if (heap_size <= 0) {
        return -1;
    }
    *value = heap[0];
    heap[0] = heap[--heap_size];
    heapify_down(0);
    return 0;
}

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static int dev_open(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "MinHeap: Dispositivo aberto\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "MinHeap: Dispositivo fechado\n");
    return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    char kbuffer[BUFFER_SIZE];
    int valor_convertido;
    int ret;

    if (len > BUFFER_SIZE - 1)
        return -EINVAL;

    if (copy_from_user(kbuffer, buffer, len)) {
        return -EFAULT;
    }
    kbuffer[len] = '\0';

    ret = kstrtoint(kbuffer, 10, &valor_convertido);
    if (ret < 0) {
        printk(KERN_ALERT "MinHeap: Falha ao converter string para int\n");
        return ret;
    }

    mutex_lock(&minheap_mutex);
    if (insert_value(valor_convertido) == 0) {
        printk(KERN_INFO "MinHeap: Inserido valor %d. Tamanho atual: %d\n", valor_convertido, heap_size);
    } else {
        printk(KERN_ALERT "MinHeap: Erro - Heap cheia!\n");
        mutex_unlock(&minheap_mutex);
        return -ENOMEM;
    }
    mutex_unlock(&minheap_mutex);

    return len;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int valor_minimo;
    char kbuffer[BUFFER_SIZE];
    int len_msg;
    int erro_heap;

    if (*offset > 0) return 0;

    mutex_lock(&minheap_mutex);
    erro_heap = extract_min(&valor_minimo);
    mutex_unlock(&minheap_mutex);

    if (erro_heap != 0) {
        printk(KERN_INFO "MinHeap: Heap vazia\n");
        return 0;
    }

    len_msg = snprintf(kbuffer, BUFFER_SIZE, "%d\n", valor_minimo);

    if (copy_to_user(buffer, kbuffer, len_msg)) {
        return -EFAULT;
    }

    *offset += len_msg;

    printk(KERN_INFO "MinHeap: Lido valor %d\n", valor_minimo);
    return len_msg;
}

static int __init minheap_init(void) {
    printk(KERN_INFO "MinHeap: Inicializando...\n");

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "MinHeap: Falha ao registrar um major number\n");
        return majorNumber;
    }

    minheapClass = class_create(CLASS_NAME);
    if (IS_ERR(minheapClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "MinHeap: Falha ao registrar a classe do dispositivo\n");
        return PTR_ERR(minheapClass);
    }

    minheapDevice = device_create(minheapClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(minheapDevice)) {
        class_destroy(minheapClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "MinHeap: Falha ao criar o dispositivo\n");
        return PTR_ERR(minheapDevice);
    }

    mutex_init(&minheap_mutex);

    printk(KERN_INFO "MinHeap: Modulo carregado corretamente. Major Number: %d\n", majorNumber);
    return 0;
}

static void __exit minheap_exit(void) {
    mutex_destroy(&minheap_mutex);
    device_destroy(minheapClass, MKDEV(majorNumber, 0));
    class_unregister(minheapClass);
    class_destroy(minheapClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "MinHeap: Modulo descarregado!\n");
}

module_init(minheap_init);
module_exit(minheap_exit);
