#ifndef BARRIER_H
#define BARRIER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h> // para O_CREAT, O_EXCL
#include <sys/mman.h> // mmap
#include <time.h>

/* Definicao da barreira */

typedef struct shmseg {
    int total;            // numero total de processos que devem esperar
    int cont;             // contador de quantos ja chegaram
    sem_t mutex;          // mutex para proteger a contagem
    sem_t semaforo;       // semaforo para bloquear/liberar processos
 } barrier_t;


/* Inicializa */
void init_barr (barrier_t *barr, int n);
/* Controle da barreira */
void process_barrier(barrier_t *barr);


#endif






