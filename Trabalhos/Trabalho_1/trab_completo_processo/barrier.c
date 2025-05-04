#include "barrier.h"

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
        barr->cont = 0;
        for (int i = 0; i < barr->total - 1; i++) {
            sem_post(&barr->semaforo);
        }
        sem_post(&barr->mutex);
        return;
    }
    sem_post(&barr->mutex); // libera o acesso pro proximo processo
    sem_wait(&barr->semaforo); // processo espera ser liberado (nao foi o ultimo a chegar)
}
