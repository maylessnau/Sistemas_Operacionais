// Solução do exercicio 6 usando processos 

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define N          5           /* Nº de processos “clientes” */
#define MAX_USOS   3           /* ciclos de uso do recurso   */

/* ---------- Estruturas em Memória Partilhada ---------- */
typedef struct node_s {
    sem_t   sem;               /* semáforo que acorda o dono do nó */
    int     next;              /* índice do próximo na fila, -1 se nulo */
    int     id;                /* id (1..N) para mensagens */
} Node;

typedef struct fifoQ_s {
    int head;                  /* índice do primeiro, -1 se vazio */
    int tail;                  /* índice do último, -1 se vazio   */
    pthread_mutex_t lock;      /* mutex process‑shared            */
} FifoQT;

typedef struct shared_s {
    FifoQT fila;               /* fila propriamente dita          */
    Node   nodes[N];           /* um nó fixo por processo         */
} Shared;
/* ------------------------------------------------------- */

/* ---------- Funções FIFO (agora por índice) ------------ */
void init_fifoQ(FifoQT *F)
{
    F->head = F->tail = -1;
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_setpshared(&a, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&F->lock, &a);
}

void espera(Shared *S, int idx)         /* idx = [0..N‑1]      */
{
    FifoQT *F = &S->fila;
    Node   *me = &S->nodes[idx];

    pthread_mutex_lock(&F->lock);
    int fila_nao_vazia = (F->head != -1);
    if (!fila_nao_vazia) {
        /* entra direto, fila vazia                   */
        pthread_mutex_unlock(&F->lock);
        printf("[INFO] Proc %d passou direto (fila vazia)\n", me->id);
        return;
    }

    /* fila não vazia → encadeia seu nó               */
    me->next = -1;
    if (F->tail != -1)
        S->nodes[F->tail].next = idx;
    F->tail = idx;
    pthread_mutex_unlock(&F->lock);

    printf("[INFO] Proc %d entrou na fila e vai ESPERAR\n", me->id);
    sem_wait(&me->sem);
}

void liberaPrimeiro(Shared *S)
{
    FifoQT *F = &S->fila;

    pthread_mutex_lock(&F->lock);
    if (F->head == -1) {               /* fila vazia ‑> nada a liberar */
        pthread_mutex_unlock(&F->lock);
        return;
    }
    int first_idx = F->head;
    Node *first   = &S->nodes[first_idx];

    F->head = first->next;
    if (F->head == -1)
        F->tail = -1;
    pthread_mutex_unlock(&F->lock);

    printf("[INFO] Liberando semáforo do Proc %d\n", first->id);
    sem_post(&first->sem);
}
/* ------------------------------------------------------- */

/* ---------- Função executada por cada processo --------- */
void ciclo_cliente(Shared *S, int idx)
{
    Node *me = &S->nodes[idx];
    for (int i = 0; i < MAX_USOS; ++i) {
        printf("[Proc %d] prólogo (ciclo %d)\n", me->id, i+1);
        sleep(1);

        espera(S, idx);

        printf(">>> [Proc %d] USANDO recurso\n", me->id);
        sleep(2); /* reduzi p/ facilitar teste */

        liberaPrimeiro(S);

        printf("[Proc %d] epílogo\n", me->id);
        sleep(1);
    }
    printf("[Proc %d] terminou.\n", me->id);
}
/* ------------------------------------------------------- */

int main(void)
{
    /* 1. Aloca memória partilhada */
    Shared *S = mmap(NULL, sizeof *S,
                     PROT_READ|PROT_WRITE,
                     MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (S == MAP_FAILED) { perror("mmap"); exit(1); }

    /* 2. Inicializa estruturas antes do fork()       */
    init_fifoQ(&S->fila);

    for (int i = 0; i < N; ++i) {
        Node *n = &S->nodes[i];
        n->id   = i + 1;
        n->next = -1;
        sem_init(&n->sem, /*pshared=*/1, 0);   /* semáforo privado de cada proc */
    }
    /* Fila parte vazia → já destravada              */
    S->fila.head = S->fila.tail = -1;

    /* 3. Cria processos filhos */
    pid_t pids[N];
    for (int i = 0; i < N; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {             /* ——— FILHO ——— */
            /* Cada filho empilha‑se se houver alguém na fila,
               ou passa direto se for o primeiro após liberação.  */
            ciclo_cliente(S, i);
            _exit(0);
        }
        pids[i] = pid;
    }

    /* 4. O processo pai faz o papel do “primeiro liberador”:
          — O 1º filho entra direto; quando ele sai, chama liberaPrimeiro().
          — Portanto, o pai só precisa liberar *uma vez* para iniciar o ciclo.   */
    sleep(1);                    /* pequena margem para filhos criarem‑se */
    pthread_mutex_lock(&S->fila.lock);
    if (S->fila.head == -1) {
        /* fila ainda vazia → próximo filho passará direto; nada a fazer */
    } else {
        int first = S->fila.head;
        S->fila.head = S->nodes[first].next;
        if (S->fila.head == -1) S->fila.tail = -1;
        sem_post(&S->nodes[first].sem);
    }
    pthread_mutex_unlock(&S->fila.lock);

    /* 5. Espera filhos terminarem */
    for (int i = 0; i < N; ++i)
        waitpid(pids[i], NULL, 0);

    /* 6. Destrói objetos e desaloca memória partilhada */
    for (int i = 0; i < N; ++i)
        sem_destroy(&S->nodes[i].sem);

    pthread_mutex_destroy(&S->fila.lock);
    munmap(S, sizeof *S);
    puts("Pai: todos os processos finalizaram.");
    return 0;
}
