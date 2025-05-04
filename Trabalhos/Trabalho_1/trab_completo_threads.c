// trabalho com as duas partes juntas usando threads

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/syscall.h>  

#define MAX_USOS 3

/* --------- barreira --------- */

/* Definicao da barreira */
typedef struct barrier_s {
    int total;            // numero total de processos que devem esperar
    int cont;             // contador de quantos ja chegaram
    sem_t mutex;          // mutex para proteger a contagem
    sem_t semaforo;       // semaforo para bloquear/liberar processos
} my_barrier_t;

typedef struct {
    int id_logico;          // índice de 0 … n‑1 apenas para prints
    my_barrier_t *bar;      // ponteiro para a barreira compartilhada
    int n;                  // total de threads (só para rand() abaixo)
} th_args_t;


void my_barrier_init(my_barrier_t *b, int n) {
    b->total = n;
    b->cont  = 0;
    sem_init(&b->mutex, 1, 1);    // comeca destravado e em memoria compartilhada 
    sem_init(&b->semaforo, 1, 0); // bloqueado inicialmente
}

void my_barrier_wait(my_barrier_t *b)
{
    sem_wait(&b->mutex); // trava o acesso pra acessar cont com seguranca
    b->cont++;           // um processo chegou entao incrementa              

    if (b->cont == b->total) {     // se foi o ultimo a chegar, libera o resto dos processos
        b->cont = 0;              
        for (int i = 0; i < b->total - 1; ++i)
            sem_post(&b->semaforo);
        sem_post(&b->mutex);      
        return;                  
    }

    sem_post(&b->mutex);       // libera o acesso pro proximo processo           
    sem_wait(&b->semaforo);   // processo espera ser liberado (nao foi o ultimo a chegar) 
}

void my_barrier_destroy(my_barrier_t *b) {
    sem_destroy(&b->mutex);
    sem_destroy(&b->semaforo);
}

// -------- Fila FIFO --------
typedef struct node {
    sem_t sem;
    struct node *next;
    int id;
} Node;

typedef struct fifoQ_s {
    Node *head;
    Node *tail;
    int tamanho;
    pthread_mutex_t lock;
} FifoQT;

void init_fifoQ(FifoQT *F, int tamanho) {
    F->head = NULL;
    F->tail = NULL;
    F->tamanho = tamanho;
    pthread_mutex_init(&(F->lock), NULL);
}

// o processo novo entra na fila de espera FIFO 
// ou passa direto se estiver vazia e o recurso
// estiver disponivel
void espera(FifoQT *F, int id) {
    int should_wait;

    pthread_mutex_lock(&F->lock);
    should_wait = (F->head != NULL);
    pthread_mutex_unlock(&F->lock);

    if (!should_wait) {
        // Fila vazia - entra direto e não cria node
        //printf("[INFO] Thread %d passou direto (fila vazia)\n", id);
        return;
    }

    // Só chega aqui quem vai esperar de verdade
    Node *me = malloc(sizeof(Node));
    me->id = id;
    sem_init(&me->sem, 0, 0);
    me->next = NULL;

    pthread_mutex_lock(&F->lock);
    if (F->tail)
        F->tail->next = me;
    F->tail = me;
    if (F->head == NULL)
        F->head = me;
    pthread_mutex_unlock(&F->lock);

    //printf("[INFO] Thread %d adicionada à fila e vai ESPERAR\n", id);
    sem_wait(&me->sem);
}

void liberaPrimeiro(FifoQT *F) {

    pthread_mutex_lock(&F->lock);

    if (F->head == NULL) {
        pthread_mutex_unlock(&F->lock);
        return;
    }

    Node *first = F->head;
    F->head = first->next;
    if (F->head == NULL)
        F->tail = NULL;

    pthread_mutex_unlock(&F->lock);

    //printf("[INFO] Liberando semáforo da Thread %d\n", first->id);

    sem_post(&first->sem);
    sem_destroy(&first->sem);
    //printf("[INFO] Nodo da Thread %d destruído\n", first->id);
    first->next = NULL;
    free(first);

    return;
}
// ---------------------------

// Variáveis globais
my_barrier_t bar;
FifoQT fila;

void *trabalho(void *arg_v) {
    th_args_t *arg = arg_v;

    /* --- fase de preparação / primeira barreira ---------------- */
    srand(time(NULL) ^ (pthread_self() << 16));
    int s = rand() % arg->n;
    //printf("[T%2d] dormindo %d s\n", arg->id_logico, s);
    sleep(s);

    printf( "--Processo: %d chegando na barreira\n", arg->id_logico );
    my_barrier_wait(&bar);  
    printf( "**Processo: %d saindo da barreira\n", arg->id_logico );              
   
    /* --------- laço de uso exclusivo do recurso ---------------- */
    for (int i = 0; i < MAX_USOS; i++) {
    
        // (A) prólogo  
        // sorteia um inteiro aleatorio s (0,1,2 ou 3)
        int s_prologo = rand() % 4; 
        printf( "Processo: %d Prologo: %d de %d segundos\n", arg->id_logico, i, s_prologo );
        sleep(s_prologo);

        // IMPLEMENTAR
        //inicia_uso( recurso, &fila );  // recurso (?) - no enunciado
        
        espera(&fila, arg->id_logico);

        // (B) utilização exclusiva 
        // sorteia um inteiro aleatorio s (0,1,2 ou 3)
        int s_utilizacao = rand() % 4; 
        printf( "Processo: %d USO: %d por %d segundos\n", arg->id_logico, i, s_utilizacao );
        sleep(s_utilizacao);

        // IMPLEMENTAR
        //termina_uso( recurso, &fila ); // recurso (?) - no enunciado

        liberaPrimeiro(&fila); // libera recurso

        // (C) epílogo
        int s_epilogo = rand() % 4; 
        printf( "Processo: %d Epilogo: %d de %d segundos\n", arg->id_logico, i, s_epilogo );
        sleep(s_epilogo);
    }

    /* --------- segunda barreira (todos concluíram o laço) ------- */
    printf( "--Processo: %d chegando novamente na barreira\n", arg->id_logico );
    my_barrier_wait(&bar);
    printf( "++Processo: %d saindo da barreira novamente\n", arg->id_logico );
    
    /* devolve o TID e encerra a thread */
    pid_t *tid = malloc(sizeof *tid);
    if (!tid) { perror("malloc"); pthread_exit(NULL); }

    *tid = syscall(SYS_gettid);

    printf("[thread %2d] terminou — pid=%d  tid=%d\n",
        arg->id_logico, getpid(), *tid);

    pthread_exit(tid);      /* único ponto de saída da thread */
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

    // IMPLEMENTAR
    // inicializa também a variavel inteira (recurso) com um numero
    // aleatorio indicando o numero do recurso a ser usado. 
    // Todos os processos devem usar o mesmo numero de recurso,
    // ou seja, essa variável PODE ser herdada pelos processo filhos,
    // assim a variável recurso NAO precisa estar em shared memory.
    //   int recurso = ... numero aleatorio inteiro qualquer 

    my_barrier_init(&bar, n);
    init_fifoQ(&fila, n);  
  
    for (int i = 0; i < n; ++i) {
        args[i].id_logico = i;
        args[i].n = n;   
        if (pthread_create(&th[i], NULL, trabalho, &args[i])) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    /* a main também espera na barreira ?? */
    //printf("main() esperando na barreira\n");
    // my_barrier_wait(&bar);
    //printf("main() passou da barreira!\n");
    

    // IMPLEMENTAR
    // programa main (APENAS O MAIN) deve esperar todos os filhos terminarem,
    // e APENAS O MAIN deve imprimir na tela os numeros (logico e PID) 
    // do filho que terminou, NA MESMA ordem de termino!
    // printf( "+++ Filho de número lógico %d e pid %d terminou!\n", Pi, pid_do_filho );

    // o trecho abaixo pega o valor de TID
    int total_threads = n;   /* valor original — não muda       */
    int restantes     = n;   /* quantas ainda faltam terminar   */

    while (restantes) {
        for (int i = 0; i < total_threads; ++i) {
            if (th[i] == 0) continue;            /* já unido          */

            void *ret;
            int r = pthread_tryjoin_np(th[i], &ret);
            if (r == 0) {                        /* acabou de terminar */
                pid_t tid = *(pid_t *)ret;
                free(ret);

                int Pi = args[i].id_logico;      /* índice lógico correto */
                printf("+++ Filho lógico %d (tid=%d) terminou\n", Pi, tid);

                th[i] = 0;                       /* marca como unido  */
                --restantes;                     /* só este contador diminui */
            }
            /* r == EBUSY → thread ainda rodando; ignora e segue      */
        }
    }

    my_barrier_destroy(&bar);

    return 0;
}
