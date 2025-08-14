// Autoras:
// Barbara Reis dos Santos - GRR: 20222538 
// Mayara Lessnau de Figueiredo Neves - GRR: 20235083 

#include "barrier.h"
#include "fifo.h"

#define SHM_KEY 0x4567   // chave da area de memoria compartilhada
#define MAX_USOS   3     // ciclos de uso do recurso  

typedef struct {
    FifoQT fila;
    barrier_t barr;
} shared_data_t;

// função do Proc cliente
void* ciclo_cliente(shared_data_t *S, int recurso, int id, int num_procs) {

    // -------------------------- primeira barreira ------------------------------------------
    // semente única por processo, combina tempo atual e PID para gerar números aleatórios diferentes entre processos
    srand(time(NULL) ^ (getpid() << 16));  
    int s = rand() % num_procs;
    sleep(s);

    printf( "--Processo: %d chegando na barreira\n", id );
    process_barrier(&S->barr);  
    printf( "**Processo: %d saindo da barreira\n", id );              
    
    // ----------------------------------- ciclos ------------------------------------------
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

    // -------------------------- segunda barreira ------------------------------------------
    printf( "--Processo: %d chegando novamente na barreira\n", id );
    process_barrier(&S->barr);
    printf( "++Processo: %d saindo da barreira novamente\n", id );

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_processos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_procs = atoi(argv[1]);  // le o numero de processos da linha de comando
    
    // se o número de processos for maior que 100 para o programa: tamanho máximo do buffer atingido
    if (num_procs > 100) {
        fprintf(stderr, "Número de processos maior que o esperado.(MAX = 100)\n");
        exit(EXIT_FAILURE);
    }

    int shmid;
    shared_data_t *shared;

    // retorna um identificador para o seg. de memoria compartilhada
    shmid = shmget(SHM_KEY, sizeof(shared_data_t), IPC_CREAT|0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    // conecta o id ao segmento para obter um ponteiro para ele
    shared = (shared_data_t *) shmat(shmid, NULL, 0);
    if (shared == (void *) -1) {
        perror("Shared memory attach");
        return 1;
    }

    // inicializa as estruturas compartilhadas
    init_fifoQ(&shared->fila, num_procs);
    init_barr(&shared->barr, num_procs);

    // gera um número aleatório entre 0 e 99
    int recurso = rand() % 100;
 
    int nProc = 0;  
    // fork para criar os processos filhos  (o pai será o processo lógico 0, e os filhos de 1 até num_procs-1)
    pid_t pids[num_procs];
    for (int i = 1; i < num_procs; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            nProc = i;
            ciclo_cliente(shared, recurso, i, num_procs);
            exit(0);
        }
        pids[i] = pid; // armazena o PID real do processo filho recem criado
    }

    // verifica se este eh o processo pai (somente o pai executa)
    if (nProc == 0) {
        ciclo_cliente(shared, recurso, 0, num_procs);  

        // espera cada filho terminar
        for (int i = 1; i < num_procs; ++i) {
            int status;
            pid_t pid_terminou = wait(&status);  // espera qualquer filho
            
            // busca o número lógico correspondente ao PID do filho que terminou
            for (int j = 1; j < num_procs; ++j) {
                if (pids[j] == pid_terminou) {
                    printf("+++ Filho de número lógico %d e pid %d terminou!\n", j, pid_terminou);
                    break;
                }
            }
        }

        // limpeza
        sem_destroy(&shared->fila.lock);
        sem_destroy(&shared->barr.mutex);
        sem_destroy(&shared->barr.semaforo);
        shmctl(shmid, IPC_RMID, NULL);
    }
    
    return 0;
}
