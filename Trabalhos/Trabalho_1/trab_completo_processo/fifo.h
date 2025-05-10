#ifndef FIFO_H
#define FIFO_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PROCS 64  


// typedef struct {
//     int id;        // ID logico do processo (? nao sei se precisa)
//     sem_t sem;     // semáforo do processo
// } Processo;

typedef struct {
    int usando;
    int head; // indica o próximo processo a ser liberado.
    int tail; // indica a próxima posição disponível.
    //Processo fila[MAX_PROCS];  // Fila circular de structs
    sem_t fila[MAX_PROCS];  // Fila circular de semaforos para cada processo
    sem_t lock;
} FifoQT;


void init_fifoQ(FifoQT *F, int tamanho);
void inicia_uso(int recurso, FifoQT *F);
void termina_uso(int recurso, FifoQT *F);


#endif
