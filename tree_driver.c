#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>  // Necessário para o Passo D (Sincronização)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dev Kernel Grau 0");
MODULE_DESCRIPTION("Driver de Arvore Estatica Min-Heap");

#define MAX_TREE_SIZE 256
#define DEVICE_NAME "min_tree_dev"

// --- ESTRUTURAS DE DADOS ---
static int heap[MAX_TREE_SIZE];
static int heap_size = 0;
static dev_t dev_num;       // Armazena o Major/Minor number
static struct cdev my_cdev; // Estrutura do dispositivo de caractere
static DEFINE_MUTEX(tree_mutex); // Mutex para evitar corrupção de dados

// --- LÓGICA DA ÁRVORE (MIN-HEAP) ---

void heap_swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void insert_static(int value) {
    int current;
    if (heap_size >= MAX_TREE_SIZE) {
        printk(KERN_INFO "TreeDriver: Buffer cheio!\n");
        return;
    }

    current = heap_size++;
    heap[current] = value;

    while (current != 0 && heap[(current - 1) / 2] > heap[current]) {
        heap_swap(&heap[(current - 1) / 2], &heap[current]);
        current = (current - 1) / 2;
    }
}

int get_min_static(void) {
    int min_val;
    if (heap_size <= 0) return -1; // Indica vazio

    min_val = heap[0];
    heap[0] = heap[--heap_size];
    
    int current = 0;
    while (1) {
        int left = 2 * current + 1;
        int right = 2 * current + 2;
        int smallest = current;

        if (left < heap_size && heap[left] < heap[smallest])
            smallest = left;
        if (right < heap_size && heap[right] < heap[smallest])
            smallest = right;

        if (smallest != current) {
            heap_swap(&heap[current], &heap[smallest]);
            current = smallest;
        } else {
            break;
        }
    }
    return min_val;
}

// --- OPERAÇÕES DE ARQUIVO (FILE OPERATIONS) ---

static int dev_open(struct inode *inodep, struct file *filep) {
    // Pode ser usado para inicializar algo por sessão, se necessário
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

// Escrita: Usuário envia número -> Driver insere na árvore
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int value_from_user;
    char kbuf[16]; 

    if (len >= sizeof(kbuf)) return -EINVAL;
    
    if (copy_from_user(kbuf, buffer, len)) return -EFAULT;
    kbuf[len] = '\0'; // Garante terminação da string

    // Passo D: Bloqueia acesso antes de mexer na árvore global
    mutex_lock(&tree_mutex);
    
    if (kstrtoint(kbuf, 10, &value_from_user) == 0) {
        insert_static(value_from_user);
        printk(KERN_INFO "TreeDriver: Inserido %d. Tamanho atual: %d\n", value_from_user, heap_size);
    }
    
    mutex_unlock(&tree_mutex); // Libera acesso
    
    return len;
}

// Leitura: Usuário lê arquivo -> Driver remove raiz e retorna
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    int min_val;
    char kbuf[32];
    int bytes_read;

    // Se o offset for > 0, significa que o cat já leu uma vez. Retornamos 0 para indicar EOF.
    if (*offset > 0) return 0;

    mutex_lock(&tree_mutex);
    
    if (heap_size <= 0) {
        mutex_unlock(&tree_mutex);
        return 0; // Nada para ler
    }

    min_val = get_min_static();
    mutex_unlock(&tree_mutex);

    bytes_read = snprintf(kbuf, sizeof(kbuf), "%d\n", min_val);

    if (copy_to_user(buffer, kbuf, bytes_read)) return -EFAULT;

    *offset += bytes_read; // Atualiza offset para o sistema saber que lemos
    return bytes_read;
}

static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

// --- INICIALIZAÇÃO E SAÍDA ---

static int __init tree_driver_init(void) {
    // 1. Aloca dinamicamente Major/Minor numbers
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
        return -1;
    }
    
    // 2. Inicializa a estrutura CDEV e conecta com as File Operations
    cdev_init(&my_cdev, &fops);
    
    // 3. Adiciona ao Kernel
    if (cdev_add(&my_cdev, dev_num, 1) == -1) {
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    printk(KERN_INFO "TreeDriver: Carregado! Major: %d\n", MAJOR(dev_num));
    printk(KERN_INFO "TreeDriver: Rode 'mknod /dev/%s c %d 0' para criar o node.\n", DEVICE_NAME, MAJOR(dev_num));
    
    return 0;
}

static void __exit tree_driver_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "TreeDriver: Descarregado.\n");
}

module_init(tree_driver_init);
module_exit(tree_driver_exit);