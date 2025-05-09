#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include<errno.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>



typedef struct node_s {
    sem_t sem;               /* semáforo que acorda o dono do nó */
    int shmid_next;  // guarda o shmid do próximo nó
    int id;          // PARA DEBUG REMOVER DPS
} Node;

typedef struct fifoQ_s {
    int shmid_head;
    int shmid_tail;
    sem_t lock;
} FifoQT;

void init_fifoQ(FifoQT *F);
void inicia_uso(int recurso,FifoQT *F);
void termina_uso(int recurso, FifoQT *F);