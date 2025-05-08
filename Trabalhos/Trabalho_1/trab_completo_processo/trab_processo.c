// trab com as duas partes juntas usando processos
// o executavel no Make esta como ./trab <num_proc>

#include "barrier.h"
#include "fifo.h"

#define SHM_KEY 0x4567
#define NUM_PROCS  5           /* Nº de processos “clientes” */
#define MAX_USOS   3           /* ciclos de uso do recurso   */


typedef struct {
    FifoQT fila;
    barrier_t barr;
    int contador;
} shared_data_t;

// Função do Proc cliente
void* ciclo_cliente(shared_data_t *S, int id) {

    /* --- fase de preparação / primeira barreira ---------------- */
    srand(time(NULL) ^ (pthread_self() << 16));
    int s = rand() % NUM_PROCS;
    sleep(s);

    printf( "--Processo: %d chegando na barreira\n", id );
    process_barrier(&S->barr);  
    printf( "**Processo: %d saindo da barreira\n", id );              
    
    for (int i = 0; i < MAX_USOS; i++) {
        
        // (A) prólogo  
        int s_prologo = rand() % 4;
        printf( "Processo: %d Prologo: %d de %d segundos\n", id, i, s_prologo );
        sleep(s_prologo);

        // IMPLEMENTAR
        //inicia_uso( recurso, &fila );  // recurso (?) - no enunciado
        
        espera(&S->fila, id);

        // (B) utilização exclusiva
        int s_utilizacao = rand() % 4; 
        printf( "Processo: %d USO: %d por %d segundos\n", id, i, s_utilizacao );
        sleep(s_utilizacao);

        // IMPLEMENTAR
        //termina_uso( recurso, &fila ); // recurso (?) - no enunciado

        liberaPrimeiro(&S->fila);

        // (C) epílogo
        int s_epilogo = rand() % 4; 
        printf( "Processo: %d Epilogo: %d de %d segundos\n", id, i, s_epilogo );
        sleep(s_epilogo);
    }

    printf("Proc %d: finalizou todos os ciclos.\n", id);

    /* --------- segunda barreira (todos concluíram o laço) ------- */
    printf( "--Processo: %d chegando novamente na barreira\n", id );
    process_barrier(&S->barr);
    printf( "++Processo: %d saindo da barreira novamente\n", id );

    S->contador += 1;
    printf("Proc %d: contador atual (compartilhado) = %d\n", id, S->contador);

    
    return NULL;
}


int main() {

 // Le o numero de processos da linha de comando 
 
    // TENTATIVA DE USAR SHARED MEMORY DO LINK DO PROF (N DEU CERTO K)
    int shmid;
    shared_data_t *shared;
   
    printf("Tamanho de shared_data_t: %lu\n", sizeof(shared_data_t));
    printf("shmget args → key=0x%x, size=%lu, flags=0%o\n",
    SHM_KEY, sizeof(shared_data_t), 0644 | IPC_CREAT);
 

    shmid = shmget(SHM_KEY, sizeof(shared_data_t), IPC_CREAT|0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    // Attach to the segment to get a pointer to it.
    shared = (shared_data_t *) shmat(shmid, NULL, 0);
    if (shared == (void *) -1) {
         perror("Shared memory attach");
        return 1;
    }

    //shared_data_t *shared = mmap(NULL, sizeof(shared_data_t),
    //                         PROT_READ | PROT_WRITE,
    //                        MAP_SHARED | MAP_ANONYMOUS,
    //                         -1, 0);
   


    /* 2. Inicializa estruturas antes do fork()       */
    init_fifoQ(&shared->fila, NUM_PROCS);
    // Apenas o processo pai inicializa a barreira (main)
    init_barr(&shared->barr, NUM_PROCS);

    // Inicializa a semente do gerador de números aleatórios
    srand(time(NULL));

    // Gera um número aleatório entre 0 e 99
    int recurso = rand() % 100;
    // IMPLEMENTAR
    // inicializa também a variavel inteira (recurso) com um numero
    // aleatorio indicando o numero do recurso a ser usado. 
    // Todos os processos devem usar o mesmo numero de recurso,
    // ou seja, essa variável PODE ser herdada pelos processo filhos,
    // assim a variável recurso NAO precisa estar em shared memory.
    // int recurso = ... numero aleatorio inteiro qualquer 

    shared->contador = 0;  // inicia com 0
    int nProc = 0;  
    // fork para criar os processos filhos
    pid_t pids[NUM_PROCS];
    for (int i = 1; i < NUM_PROCS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            nProc = i;
            ciclo_cliente(shared, i);
            exit(0);
        }
        pids[i] = pid;
    }

    // processo se identifica
    printf("Processo logico %d, PID=%d, PPID=%d\n", nProc, getpid(), getppid());

    if (nProc == 0) {
        ciclo_cliente(shared, 0);  // pai participa  -- DUVIDA: o pai tambem participa?
    
        // Espera cada filho terminar, na ordem que terminarem
        for (int i = 1; i < NUM_PROCS; ++i) {
            int status;
            pid_t pid_terminou = wait(&status);  // espera qualquer filho
    
            // Descobre o número lógico correspondente ao PID
            for (int j = 1; j < NUM_PROCS; ++j) {
                if (pids[j] == pid_terminou) {
                    printf("+++ Filho de número lógico %d e pid %d terminou!\n", j, pid_terminou);
                    break;
                }
            }
        }
    
        // Limpeza
        sem_destroy(&shared->fila.lock);
        sem_destroy(&shared->barr.mutex);
        sem_destroy(&shared->barr.semaforo);
        munmap(shared, sizeof(shared_data_t));
        puts("Pai: todos os processos finalizaram.");
    }
    
    return 0;
}
