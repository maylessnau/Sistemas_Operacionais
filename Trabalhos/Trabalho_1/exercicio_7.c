#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h> // para O_CREAT, O_EXCL
#include <sys/mman.h> // mmap
#include <time.h>

typedef struct barrier_s {
    int total;             // número total de processos que devem esperar
    int count;             // contador de quantos já chegaram
    sem_t mutex;           // semáforo para proteger a contagem
    sem_t turnstile;       // semáforo para bloquear/liberar processos
} barrier_t;

