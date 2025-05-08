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
    should_wait = (F->head != NULL);
    sem_post(&F->lock);



    if (!should_wait) {
        //printf("[INFO] Processo %d passou direto (fila vazia)\n", id);
        return;
    }

    Node *me = malloc(sizeof(Node));
    if (!me) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    me->next = NULL;
    // inicializa o semaforo do proccesso bloqueado
    sem_init(&me->sem, 0, 0);

    sem_wait(&F->lock);
    if (F->tail)
        F->tail->next = me;
    F->tail = me;
    if (F->head == NULL)
        F->head = me;
    sem_post(&F->lock);

    //printf("[INFO] Processo %d adicionada à fila e vai ESPERAR\n", id);
    sem_wait(&me->sem);
}   


void termina_uso(int recurso, FifoQT *F)
{
    sem_wait(&F->lock);

    if (F->head == NULL) {
        sem_post(&F->lock);
        return;
    }

    Node *first = F->head;
    F->head = first->next;
    if (F->head == NULL)
        F->tail = NULL;

    sem_post(&F->lock);

    printf("[INFO] Liberando semáforo da Processo %d\n", first->id);

    sem_post(&first->sem);
    sem_destroy(&first->sem);
    printf("[INFO] Nodo da Processo %d destruído\n", first->id);
    free(first);


}