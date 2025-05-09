#include "fifo.h"

void init_fifoQ(FifoQT *F)
{
    // define que a fila está inicialmente vazia
    // -1 indica "sem elemento", pois nenhum shmid válido será -1
    F->shmid_head = -1;
    F->shmid_tail = -1;

    // inicializa o semáforo (mutex) com valor 1
    // o segundo argumento '1' indica que o semáforo será compartilhado entre processos
    sem_init(&(F->lock), 1, 1);
}

// PARA DEBUG REMOVER DEPOS
void print_fila(FifoQT *F) {
    sem_wait(&F->lock);

    printf("=== Estado atual da fila ===\n");

    int curr_shmid = F->shmid_head;
    int pos = 0;

    while (curr_shmid != -1) {
        Node *node = (Node *) shmat(curr_shmid, NULL, 0);
        if (node == (void *) -1) {
            perror("shmat print_fila");
            break;
        }

        printf("Posição %d → PID: %d | SHMID: %d | NEXT: %d\n", 
               pos, node->id, curr_shmid, node->shmid_next);

        curr_shmid = node->shmid_next;
        shmdt(node);
        pos++;
    }

    if (pos == 0) {
        printf("(Fila vazia)\n");
    }

    printf("===========================\n");

    sem_post(&F->lock);
}


void inicia_uso(int recurso, FifoQT *F)
{
    // cria nó em memória compartilhada
    int shmid = shmget(IPC_PRIVATE, sizeof(Node), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget node");
        exit(1);
    }

    // anexa o segmento criado e obtém um ponteiro que aponta para a area compartilhada
    Node *me = (Node *) shmat(shmid, NULL, 0);
    if (me == (void *) -1) {
        perror("shmat node");
        exit(1);
    }

    // inicializa o novo no
    me->shmid_next = -1; // inicialmente, o novo nó não aponta para nenhum próximo (-1 indica que nao possui sucessor)
    me->id = getpid();
    sem_init(&me->sem, 1, 0); // inicializa o semáforo bloqueado para que o processo espere até ser liberado pela fila

    int is_first = 0; // flag para saber se esse proc. sera o primeiro

    sem_wait(&F->lock);

    // fila nao esta vazia
    if (F->shmid_tail != -1) {
        // conecta o seg. identificado por shmid_tail e obtém um ponteiro para acessá-lo 
        Node *tail = (Node *) shmat(F->shmid_tail, NULL, 0);

        // atualiza shmid_next do último nó, fazendo-o apontar para o novo nó
        tail->shmid_next = shmid;

        // desconecta o seg. de memoria compartilhada do ultimo no
        shmdt(tail);
    } else {
        // fila vazia, esse será o primeiro
        F->shmid_head = shmid;
        // marca que esse processo sera o primeiro e nao deve esperar
        is_first = 1;
    }

    // atualiza o tail da fila para apontar para o novo nó
    F->shmid_tail = shmid;

    // sai da RC
    sem_post(&F->lock); 

    // REMOVER
    printf("[INFO] Processo %d entrou na fila.\n", me->id);
    print_fila(F);

    if (is_first) {
        // se for o primeiro, se libera imediatamente
        sem_post(&me->sem);
    }

    // espera ser liberado pelo proc. anterior
    sem_wait(&me->sem);  

    // desanexa nó criado
    shmdt(me);
}

 
void termina_uso(int recurso, FifoQT *F)
{
    sem_wait(&F->lock);

    // verifica se a fila está vazia 
    if (F->shmid_head == -1) {
        sem_post(&F->lock);
        return;
    }

    // obtem o shmid do primeiro nó da fila (nó atual que está terminando o uso)
    int shmid = F->shmid_head;

    // anexa o primeiro nó para poder acessá-lo
    Node *first = (Node *) shmat(shmid, NULL, 0);
    if (first == (void *) -1) {
        perror("shmat termina_uso");
        exit(1);
    }

    // guarda o próximo da fila (caso exista)
    int next_shmid = first->shmid_next;

    // atualiza a head da fila para o próximo nó
    F->shmid_head = next_shmid;

    // se não há próximo, a fila ficou vazia
    if (F->shmid_head == -1)
        F->shmid_tail = -1;

    sem_post(&F->lock);

    printf("[INFO] Liberando próximo processo da fila...\n");

    // se existe próximo processo na fila, libera seu semáforo
    if (next_shmid != -1) {
        Node *next = (Node *) shmat(next_shmid, NULL, 0);
        if (next == (void *) -1) {
            perror("shmat next node");
            exit(1);
        }

        // libera o processo que está no próximo nó da fila
        sem_post(&next->sem);

        // desanexa o nó do próximo processo
        shmdt(next);
    }

    // REMOVER mensagem de que o processo atual foi removido da fila
    printf("[INFO] Processo %d foi removido da fila.\n", first->id);
    printf("TERMINA USO\n");
    print_fila(F); 

    // destroi o semáforo do processo atual
    sem_destroy(&first->sem);
    printf("[INFO] Nodo do Processo %d destruído\n", first->id);

    // desanexa e remove o segmento de memória do nó atual
    shmdt(first);
    shmctl(shmid, IPC_RMID, NULL);  // marca o segmento para remoção
}


