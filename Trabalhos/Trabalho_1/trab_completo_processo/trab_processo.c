#include "barrier.h"
#include "fifo.h"
#include <pthread.h>

#define SHM_KEY 0x4567   // chave da area de memoria compartilhada
#define MAX_USOS   3     // ciclos de uso do recurso  
#define MAX_TERMINOS 100 // tamanho do vetor usado pra saber a ordem de quem terminou

typedef struct {
    FifoQT fila;
    barrier_t barr;
    int ordem_termino[MAX_TERMINOS];
    int prox_indice_termino;    // guarda em qual posição o vetor de termino parou
    sem_t lock_termino;         // pra evitar race condition
} shared_data_t;

// typedef struct {
//     FifoQT fila;
//     barrier_t barr;
// } shared_data_t;

// Função do Proc cliente
void* ciclo_cliente(shared_data_t *S, int recurso, int id, int num_procs) {

    /* ------------------ fase de preparação / primeira barreira -------------- */
    srand(time(NULL) ^ (pthread_self() << 16));
    int s = rand() % num_procs;
    sleep(s);

    printf( "--Processo: %d chegando na barreira\n", id );
    process_barrier(&S->barr);  
    printf( "**Processo: %d saindo da barreira\n", id );              
    
    for (int i = 0; i < MAX_USOS; i++) {
        
        // (A) prólogo  
        int s_prologo = rand() % 4;
        printf( "Processo: %d Prologo: %d de %d segundos\n", id, i, s_prologo );
        sleep(s_prologo);

        inicia_uso( recurso, &S->fila );  
            // REGIÃO CRÍTICA
            // (B) utilização exclusiva
            int s_utilizacao = rand() % 4; 
            printf( "Processo: %d USO: %d por %d segundos\n", id, i, s_utilizacao );
            sleep(s_utilizacao);
        termina_uso( recurso, &S->fila ); 

        // (C) epílogo
        int s_epilogo = rand() % 4; 
        printf( "Processo: %d Epilogo: %d de %d segundos\n", id, i, s_epilogo );
        sleep(s_epilogo);
    }

    /* ----------- segunda barreira (todos concluíram o laço) -------------- */
    printf( "--Processo: %d chegando novamente na barreira\n", id );
    process_barrier(&S->barr);
    printf( "++Processo: %d saindo da barreira novamente\n", id );

    // quem chamou a funcao exit primeiro
    // semaforo pra acessar um de cada vez
    sem_wait(&S->lock_termino);

    // Armazena o id na posição correta do vetor
    S->ordem_termino[S->prox_indice_termino] = id;
    // Incrementa o índice para a próxima posição
    S->prox_indice_termino++;
    
    sem_post(&S->lock_termino);

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        // imprime tipo "Uso: ./programa <num_processos>"
        fprintf(stderr, "Uso: %s <num_processos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_procs = atoi(argv[1]);  // Le o numero de processos da linha de comando 

    int shmid;
    shared_data_t *shared;
  
    printf("Tamanho de shared_data_t: %lu bytes\n", sizeof(shared_data_t));

    // Retorna um identificador para o seg. de memoria compartilhada
    shmid = shmget(SHM_KEY, sizeof(shared_data_t), IPC_CREAT|0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    // Conecta o id ao segmento para obter um ponteiro para ele.
    shared = (shared_data_t *) shmat(shmid, NULL, 0);
    if (shared == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    // inicializa as estruturas compartilhadas
    init_fifoQ(&shared->fila, num_procs);
    init_barr(&shared->barr, num_procs);
    shared->prox_indice_termino = 0;
    sem_init(&shared->lock_termino, 1, 1); // 1 = compartilhado entre processos

    // Gera um número aleatório entre 0 e 99
    int recurso = rand() % 100;
 
    int nProc = 0;  
    // fork para criar os processos filhos
    pid_t pids[num_procs];
    for (int i = 1; i < num_procs; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            nProc = i;
            ciclo_cliente(shared, recurso, i, num_procs);
            exit(0);
        }
        pids[i] = pid;
    }

    if (nProc == 0) {
        ciclo_cliente(shared, recurso, 0, num_procs);  

        // // Espera cada filho terminar
        // for (int i = 1; i < num_procs; ++i) {
        //     int status;
        //     pid_t pid_terminou = wait(&status);  // espera qualquer filho
            
        //     // Descobre o número lógico correspondente ao PID
        //     for (int j = 1; j < num_procs; ++j) {
        //         if (pids[j] == pid_terminou) {
        //             printf("+++ Filho de número lógico %d e pid %d terminou!\n", j, pid_terminou);
        //             break;
        //         }
        //     }
        // }

        // imprime os filhos na ordem em que terminaram
        for (int i = 0; i < num_procs; ++i) {
            int num_logico = shared->ordem_termino[i];
            // exclui a main
            if (num_logico != 0) {
                pid_t pid = pids[num_logico];
                printf("++++ Filho de número lógico %d e pid %d terminou!\n", num_logico, pid);
            }
        }
        
        // Limpeza
        sem_destroy(&shared->fila.lock);
        sem_destroy(&shared->barr.mutex);
        sem_destroy(&shared->barr.semaforo);
        sem_destroy(&shared->lock_termino);
        shmctl(shmid, IPC_RMID, NULL);
    }
    
    return 0;
}
