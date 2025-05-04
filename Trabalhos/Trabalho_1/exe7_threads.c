// Exercicio 7 com threads (adaptado do seu)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/syscall.h>  


/* Definicao da barreira */
typedef struct {
    int total, count;
    sem_t mutex;      // exclusão mútua ao 'count'
    sem_t turn1;      // primeira cancela
    sem_t turn2;      // segunda cancela
} my_barrier_t;

typedef struct {
    int id_logico;          // índice de 0 … n‑1 apenas para prints
    my_barrier_t *bar;      // ponteiro para a barreira compartilhada
    int n;                  // total de threads (só para rand() abaixo)
} th_args_t;


void my_barrier_init(my_barrier_t *b, int n)
{
    b->total = n;
    b->count = 0;
    sem_init(&b->mutex,  0, 1);
    sem_init(&b->turn1,  0, 0);
    sem_init(&b->turn2,  0, 1);
}

void my_barrier_wait(my_barrier_t *b)
{
    /* Fase 1 – chegar */
    sem_wait(&b->mutex);
    if (++b->count == b->total) {        // última a chegar
        sem_wait(&b->turn2);             // fecha a 2ª cancela
        sem_post(&b->turn1);             // abre a 1ª
    }
    sem_post(&b->mutex);

    sem_wait(&b->turn1);                 // passa a 1ª cancela
    sem_post(&b->turn1);                 // deixa-a aberta p/ o próximo

    /* Fase 2 – sair */
    sem_wait(&b->mutex);
    if (--b->count == 0) {               // última a sair
        sem_wait(&b->turn1);             // fecha a 1ª
        sem_post(&b->turn2);             // abre a 2ª
    }
    sem_post(&b->mutex);

    sem_wait(&b->turn2);                 // passa a 2ª cancela
    sem_post(&b->turn2);                 // deixa-a aberta p/ reutilizar
}

void my_barrier_destroy(my_barrier_t *b)
{
    sem_destroy(&b->mutex);
    sem_destroy(&b->turn1);
    sem_destroy(&b->turn2);
}


//* ------------------------------------ */

my_barrier_t bar;

void *trabalho(void *arg_v) {
    th_args_t *arg = arg_v;

    /* --- fase de preparação / primeira barreira ---------------- */
    srand(time(NULL) ^ (pthread_self() << 16));
    int s = rand() % arg->n;
    printf("[T%2d] dormindo %d s\n", arg->id_logico, s);
    sleep(s);

    printf( "--Processo: %d chegando na barreira\n", arg->id_logico );
    my_barrier_wait(&bar);  
    printf( "**Processo: %d saindo da barreira\n", arg->id_logico );              
   
    // laço

    /* --------- segunda barreira ------- */
    printf( "--Processo: %d chegando novamente na barreira\n", arg->id_logico );
    my_barrier_wait(&bar);
    printf( "++Processo: %d saindo da barreira novamente\n", arg->id_logico );
  
    return NULL;

}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        // imprime tipo "Uso: ./programa <num_processos>"
        fprintf(stderr, "Uso: %s <num_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);  // Le o numero de processos da linha de comando 
    pthread_t th[n];
    th_args_t args[n];

    my_barrier_init(&bar, n);  
  
    for (int i = 0; i < n; ++i) {
        args[i].id_logico = i;
        args[i].n = n;   
        if (pthread_create(&th[i], NULL, trabalho, &args[i])) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; ++i) pthread_join(th[i], NULL);
    
    my_barrier_destroy(&bar);

    return 0;
}