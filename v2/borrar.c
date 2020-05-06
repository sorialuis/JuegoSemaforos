#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
    int error=0;
    char str[50];

    error = sem_unlink("/semQueue");
    if (error) {
      perror("/semQueue");
    }
    else {
      printf("/semQueue borrado!\n");
    }

    for(int i = 0; i < 3; i++){
      sprintf(str, "/cashier%d.semServed", i);
      error = sem_unlink(str);
      if (error) {
        perror(str);
      }
      else {
        sprintf(str, "/cashier%d.semServed borrado!", i);
        puts(str);
      }
      sprintf(str, "/cashier%d.semCook", i);
      error = sem_unlink(str);
      if (error) {
        perror(str);
      }
      else {
        sprintf(str, "/cashier%d.semCook borrado!", i);
        puts(str);
      }
      sprintf(str, "/cashier%d.semCheckout", i);
      error = sem_unlink(str);
      if (error) {
        perror(str);
      }
      else {
        sprintf(str, "/cashier%d.semCheckout borrado!", i);
        puts(str);
      }

      }
      


  return error;
}