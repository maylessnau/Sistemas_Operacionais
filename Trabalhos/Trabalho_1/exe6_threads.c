// Solução do exercicio 6 usando threads

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define MAX_USOS 3

// -------- Fila FIFO --------
typedef struct node {
    sem_t sem;
    struct node *next;
    int id;
} Node;

typedef struct fifoQ_s {
    Node *head;
    Node *tail;
    int tamanho;
    pthread_mutex_t lock;
} FifoQT;

void init_fifoQ(FifoQT *F, int tamanho) {
    F->head = NULL;
    F->tail = NULL;
    F->tamanho = tamanho;
    pthread_mutex_init(&(F->lock), NULL);
}

// o processo novo entra na fila de espera FIFO 
// ou passa direto se estiver vazia e o recurso
// estiver disponivel
void espera(FifoQT *F, int id) {
    int should_wait;

    pthread_mutex_lock(&F->lock);
    should_wait = (F->head != NULL);
    pthread_mutex_unlock(&F->lock);

    if (!should_wait) {
        // Fila vazia → entra direto e não cria node
        printf("[INFO] Thread %d passou direto (fila vazia)\n", id);
        return;
    }

    // Só chega aqui quem vai esperar de verdade
    Node *me = malloc(sizeof(Node));
    me->id = id;
    sem_init(&me->sem, 0, 0);
    me->next = NULL;

    pthread_mutex_lock(&F->lock);
    if (F->tail)
        F->tail->next = me;
    F->tail = me;
    if (F->head == NULL)
        F->head = me;
    pthread_mutex_unlock(&F->lock);

    printf("[INFO] Thread %d adicionada à fila e vai ESPERAR\n", id);
    sem_wait(&me->sem);
}



void liberaPrimeiro(FifoQT *F) {
    pthread_mutex_lock(&F->lock);

    if (F->head == NULL) {
        pthread_mutex_unlock(&F->lock);
        return;
    }

    Node *first = F->head;
    F->head = first->next;
    if (F->head == NULL)
        F->tail = NULL;

    pthread_mutex_unlock(&F->lock);

    printf("[INFO] Liberando semáforo da Thread %d\n", first->id);

    sem_post(&first->sem);
    sem_destroy(&first->sem);
    printf("[INFO] Nodo da Thread %d destruído\n", first->id);
    first->next = NULL;
    free(first);
}

// ---------------------------

// Variáveis globais
FifoQT fila;

// Função da thread cliente
void* cliente(void* arg) {
    int id = *((int*)arg);
    free(arg);

    for (int i = 0; i < MAX_USOS; i++) {
        printf("ciclo: %d\n",i);
        printf("Thread %d: prólogo\n", id);
        sleep(1);

        espera(&fila, id);

        printf(">>> Thread %d: USANDO o recurso\n", id);
        sleep(8);

        liberaPrimeiro(&fila);

        printf("Thread %d: epílogo\n", id);
        sleep(1);
    }

    printf("Thread %d: finalizou todos os ciclos.\n", id);
    return NULL;
}


int main() {
    const int N = 5;
    pthread_t threads[N];

    init_fifoQ(&fila, N);

    for (int i = 0; i < N; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[i], NULL, cliente, id);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // fazer um for para liberar os semaforos

    return 0;
}
