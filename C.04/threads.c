#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct{
    sem_t managerfinishDaySem;
} arguments;

void* sellTickets (void* v);

int pthread();

int main(){
    printf("BLA BLU \"");
}