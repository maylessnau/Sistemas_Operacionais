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
    struct node_s *next;             /* índice do próximo na fila, -1 se nulo */
    int id;                /* id (1..N) para mensagens */
} Node;

typedef struct fifoQ_s {
    Node *head;                  /* índice do primeiro, -1 se vazio */
    Node *tail;                  /* índice do último, -1 se vazio   */
    sem_t lock;      /* mutex process‑shared            */
} FifoQT;


void init_fifoQ(FifoQT *F);
void inicia_uso(int recurso,FifoQT *F);
void termina_uso(int recurso, FifoQT *F);