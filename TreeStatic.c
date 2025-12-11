#define MAX_TREE_SIZE 256

static int heap[MAX_TREE_SIZE];
static int heap_size = 0;

// Helper: Troca valores
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Lógica de Inserção (Bubble Up)
void insert_static(int value) {
    if (heap_size >= MAX_TREE_SIZE) return; // Buffer cheio

    int current = heap_size++;
    heap[current] = value;

    // Reorganiza para manter propriedade de Min-Heap
    while (current != 0 && heap[(current - 1) / 2] > heap[current]) {
        swap(&heap[(current - 1) / 2], &heap[current]);
        current = (current - 1) / 2;
    }
}

// Lógica de Leitura do Menor Valor (Extração da Raiz)
int get_min_static(void) {
    if (heap_size <= 0) return -1; // Erro ou vazio

    int min_val = heap[0];
    
    // Move o último para a raiz e reduz o tamanho
    heap[0] = heap[--heap_size];
    
    // Bubble Down (Reorganiza a árvore para baixo)
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
            swap(&heap[current], &heap[smallest]);
            current = smallest;
        } else {
            break;
        }
    }
    return min_val;
}
