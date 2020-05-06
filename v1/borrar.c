#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
    int error=0;

    error = sem_unlink("/semQueue");
    if (error) {
      perror("/semQueue");
      return error;
    }
    else {
      printf("/semQueue borrado!\n");
    }
    
    error = sem_unlink("/semServed");
    if (error) {
      perror("/semServed");
      return error;
    }
    else {
      printf("/semServed borrado!\n");
    }

    error = sem_unlink("/cashier.semCook");
    if (error) {
      perror("/cashier.semCook");
      return error;
    }
    else {
      printf("/cashier.semCook borrado!\n");
    }

    error = sem_unlink("/cashier.semCheckout");
    if (error) {
      perror("/cashier.semCheckout");
      return error;
    }
    else {
      printf("/cashier.semCheckout borrado!\n");
    }


  return error;
}