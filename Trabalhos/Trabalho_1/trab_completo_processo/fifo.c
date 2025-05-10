#include "fifo.h"
#include <stdio.h>

void init_fifoQ(FifoQT *F, int tamanho) {
    F->usando = 0;
    F->head = 0;
    F->tail = 0;

    for (int i = 0; i < MAX_PROCS; i++) {
        sem_init(&F->fila[i], 1, 0);  // começa bloqueado
    }

    sem_init(&F->lock, 1, 1);
}

void inicia_uso(int recurso, FifoQT *F) {

    sem_wait(&F->lock);

    // se o recurso está livre e a fila está vazia, usa direto
    if (F->usando == 0 && F->head == F->tail) {
        F->usando = 1;
        sem_post(&F->lock);
        return;
    }

    // caso contrário, entra na fila
    int pos = F->tail;
    F->tail = (F->tail + 1) % MAX_PROCS; // fila circular(?)

    sem_post(&F->lock);

    sem_wait(&F->fila[pos]);  // espera ser liberado
}

void termina_uso(int recurso, FifoQT *F) {

    sem_wait(&F->lock);

    // verifica se a fila esta vazia
    if (F->head == F->tail) {
        F->usando = 0;
        sem_post(&F->lock);
        return;
    }

    int pos = F->head; // pega a posição do próximo processo da fila
    F->head = (F->head + 1) % MAX_PROCS; // avança o ponteiro da fila circular

    sem_post(&F->lock);

    sem_post(&F->fila[pos]);
}