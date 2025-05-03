#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h> // para O_CREAT, O_EXCL
#include <sys/mman.h> // mmap
#include <time.h>

/* Definicao da barreira */
typedef struct barrier_s {
    int total;            // numero total de processos que devem esperar
    int cont;             // contador de quantos ja chegaram
    sem_t mutex;          // mutex para proteger a contagem
    sem_t semaforo;       // semaforo para bloquear/liberar processos
} barrier_t;

/* Inicializa */
void init_barr (barrier_t *barr, int n) {
    barr->total = n;
    barr->cont = 0;
    sem_init(&barr->mutex, 1, 1);          // comeca destravado e em memoria compartilhada 
    sem_init(&barr->semaforo, 1, 0);       // bloqueado inicialmente
}

/* Controle da barreira */
void process_barrier(barrier_t *barr) {
    sem_wait (&barr->mutex); // trava o acesso pra acessar cont com seguranca
    barr->cont++;            // um processo chegou entao incrementa
    if (barr->cont == barr->total) {  // se foi o ultimo a chegar, libera o resto dos processos
        for (int i = 0; i < barr->total - 1; i++) {
            sem_post(&barr->semaforo);
        }
        sem_post(&barr->mutex);
        return;
    }
    sem_post(&barr->mutex); // libera o acesso pro proximo processo
    sem_wait(&barr->semaforo); // processo espera ser liberado (nao foi o ultimo a chegar)
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        // imprime tipo "Uso: ./programa <num_processos>"
        fprintf(stderr, "Uso: %s <num_processos>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]); // Le o numero de processos da linha de comando 

    /* aloca uma regiao de memoria compartilhada onde o sistema decidir alocar (NULL)
     * do tamanho da struc barrier_t, que pode ser lida e escrita (PROT_READ | PROT_WRITE)
     * e eh compartilhada entre processos e sem arquivo pro tras (MAP_SHARED | 
     * MAP_ANONYMOUS)(-1 e 0 vao ser ignorados pq nao estamos usando arquivos, mas sao 
     * necessarios como parametros) 
     */
    barrier_t *barr = mmap(NULL, sizeof(barrier_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // verifica se o mmap deu certo
    if (barr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Apenas o processo pai inicializa a barreira (main)
    init_barr(barr, n);
    int nProc = 0;
    
    // fork para criar os processos filhos
    for (int i = 1; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            nProc = i;
            break; // filho sai do loop
        }
    }

    // processo se identifica
    printf("Processo logico %d, PID=%d, PPID=%d\n", nProc, getpid(), getppid());

    // cada processo escolhe um tempo aleatorio pra dormir 
    // simula que os processos nao cheguem ao mesmo tempo na barreira
    srand(time(NULL) ^ (getpid() << 16));
    int ns = rand() % n;
    printf("[Proc %d] Dormindo por %d segundos...\n", nProc, ns);
    sleep(ns);

    printf("[Proc %d] Chegando na barreira\n", nProc);
    process_barrier(barr);
    printf("[Proc %d] Saiu da barreira!\n", nProc);

    // processo pai espera os filhos terminarem e depois limpa a memoria
    if (nProc == 0) {
        for (int i = 1; i < n; i++) wait(NULL);
        munmap(barr, sizeof(barrier_t));
    }

    return 0;
}

