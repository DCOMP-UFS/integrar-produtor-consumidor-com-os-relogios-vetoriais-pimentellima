/**
 * Código de implementação da parte 3 do projeto de PPC
 * (integrando relógios vetoriais com o modelo produtor-consumidor)
 * 
 * Compilação: mpicc -o rvet rvet.c
 * Execução:   mpiexec -n 3 ./rvet
 */


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

#define MAX 6

typedef struct {
    int p[3];
    int senderId, receiverId;
} Clock;

typedef struct {
    Clock queue[MAX];
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t condFull, condEmpty;
} Queue;

Queue entranceQueue, exitQueue;

void push(Queue *queue, Clock clock) {
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == MAX) {
        printf("Fila cheia!\n");
        pthread_cond_wait(&queue->condFull, &queue->mutex);
    }
    int size = queue->size;
    queue->queue[size] = clock;
    queue->size++;
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_signal(&queue->condEmpty);
}

Clock pop(Queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == 0) {
        printf("Fila vazia!\n");
        pthread_cond_wait(&queue->condEmpty, &queue->mutex);
    }
    
    Clock Clock = queue->queue[0];
    int i;
    for (i = 0; i < queue->size - 1; i++){
      queue->queue[i] = queue->queue[i+1];
    }
    queue->size--;
   
    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_signal(&queue->condFull);
    
    return Clock;
    
} 

int Max(int a, int b) {
   if (a > b) return a;
   else return b;
}

void Event(int pid, Clock *clock){
   clock->p[pid]++;
   printf("Process: %d, Clock: (%d, %d, %d)\n", pid, clock->p[0], clock->p[1], clock->p[2]);
}


void Send(int myid, int pid, Clock *clock){
   clock->p[myid]++;
   Clock clockToSend = *clock;
   clockToSend.senderId = myid;
   clockToSend.receiverId = pid;
   push(&exitQueue, clockToSend);
   //printf("Process: %d, Clock: (%d, %d, %d)\n", myid, clock->p[0], clock->p[1], clock->p[2]);
}

void Receive(int myid, Clock *clock){
   clock->p[myid]++;
   Clock recv_clock = pop(&entranceQueue);
   clock->p[0] = Max(recv_clock.p[0], clock->p[0]);
   clock->p[1] = Max(recv_clock.p[1], clock->p[1]);
   clock->p[2] = Max(recv_clock.p[2], clock->p[2]);
   printf("Process: %d, Clock: (%d, %d, %d)\n", myid, clock->p[0], clock->p[1], clock->p[2]);

}

// Representa o processo de rank 0
void process0(){
   Clock clock = {{0,0,0}};
   Event(0, &clock);
   
   Send(0, 1, &clock);
   
   Receive(0, &clock);
   
   Send(0, 2, &clock);
   
   Receive(0, &clock);
   
   Send(0, 1, &clock);
   
   Event(0, &clock);
   
   printf("Final result - Process: %d, Clock: (%d, %d, %d)\n", 0, clock.p[0], clock.p[1], clock.p[2]);

}

// Representa o processo de rank 1
void process1(){
   Clock clock = {{0,0,0}};

   Send(1, 0, &clock);
   
   Receive(1, &clock);
   
   Receive(1, &clock);
   
   printf("Final result - Process: %d, Clock: (%d, %d, %d)\n", 1, clock.p[0], clock.p[1], clock.p[2]);
}

// Representa o processo de rank 2
void process2(){
   Clock clock = {{0,0,0}};
   
   Event(2, &clock);
   
   Send(2, 0, &clock);
   
   Receive(2, &clock);
   
   printf("Final result - Process: %d, Clock: (%d, %d, %d)\n", 2, clock.p[0], clock.p[1], clock.p[2]);
   
}

void startQueue(Queue *queue) {
    
    queue->size = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->condEmpty, NULL);
    pthread_cond_init(&queue->condFull, NULL);
    
}

void *startEntranceThread(void *args) {
    
    long id = (long) args;
    while(1) {
        Clock receivedClock;
        MPI_Recv(&receivedClock, sizeof(Clock), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        push(&entranceQueue, receivedClock);
    }
    
    return NULL;
}

void *startExitThread(void *args) {
    
    long id = (long) args;
    while(1) {
        Clock clockToSend = pop(&exitQueue);
        MPI_Send(&clockToSend, sizeof(Clock), MPI_BYTE, clockToSend.receiverId, 0, MPI_COMM_WORLD);
        printf("Process: %d, Clock: (%d, %d, %d)\n", clockToSend.receiverId, clockToSend.p[0], clockToSend.p[1], clockToSend.p[2]);
    }
 
    return NULL;   
}

int main(void) {
   int my_rank;               

   MPI_Init(NULL, NULL); 
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   
   startQueue(&entranceQueue);
   startQueue(&exitQueue);
   
   pthread_t entranceThread, exitThread;
   
   pthread_create(&entranceThread, NULL, &startEntranceThread, NULL);
   pthread_create(&exitThread, NULL, &startExitThread, NULL);

   if (my_rank == 0) { 
      process0();
   } else if (my_rank == 1) {  
      process1();
   } else if (my_rank == 2) {  
      process2();
   }
   
   pthread_join(entranceThread, NULL);
   pthread_join(exitThread, NULL);

   /* Finaliza MPI */
   MPI_Finalize(); 

   return 0;
}  /* main */
