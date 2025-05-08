#include "fifo.h"

void init_fifoQ(FifoQT *F)
{
    F->head = NULL;
    F->tail = NULL;
    // mutex com compartilhamento entre processos (pshared = 1)
    sem_init(&(F->lock), 1, 1);
  
}

void inicia_uso(int recurso, FifoQT *F)
{
    int should_wait;

    sem_wait(&F->lock);
    //should_wait = (F->head != NULL);
    //printf ("should_wait: %d", should_wait);
 
    Node *me = malloc(sizeof(Node));
    if (!me) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    me->next = NULL;

    // inicializa o semaforo do processo bloqueado
    sem_init(&me->sem, 0, 0);

    if (F->tail)
        F->tail->next = me;
    F->tail = me;

    if (F->head == NULL)
        F->head = me;

    should_wait = (F->head != me); 
    sem_post(&F->lock); 
    if (should_wait) {
        sem_wait(&me->sem);

    }

    return;
}   


void termina_uso(int recurso, FifoQT *F)
{
    sem_wait(&F->lock);

    Node *first = F->head;

    if (first == NULL) {
        // Fila já está vazia, nada a liberar
        sem_post(&F->lock);
        return;
    }

    F->head = first->next;

    // Se a fila ficou vazia, atualiza o tail também
    if (F->head == NULL)
        F->tail = NULL;

    sem_post(&F->lock);

    sem_post(&first->sem);       // Libera o próximo processo
    sem_destroy(&first->sem);    // Destroi o semáforo do nó atual

    free(first);
}
