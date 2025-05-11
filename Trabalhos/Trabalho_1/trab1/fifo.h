#ifndef FIFO_H
#define FIFO_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PROCS 64  

typedef struct {
    int usando; // indica se o recurso esta sendo usado ou nao
    int head; // indica o próximo processo a ser liberado.
    int tail; // indica a próxima posição disponível.
    int num_procs;
    sem_t fila[MAX_PROCS];  // fila circular de semaforos para cada processo
    sem_t lock;
} FifoQT;

void init_fifoQ(FifoQT *F, int tamanho);
void inicia_uso(int recurso, FifoQT *F);
void termina_uso(int recurso, FifoQT *F);

#endif
