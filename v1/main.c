#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>


#define MS 1000
#define CLIENTS 10
#define CASHIERS 3

typedef struct {
    char name[50];
    int prepTime;
}Food;

typedef struct {
    sem_t *semCook;
    sem_t *semCheckout;
}Cashier;

typedef struct {
    int id;
    int tolerance;
    Food *order;
    sem_t *semQueue;
}Client;

typedef struct {
    Food *menu;
    Cashier cashier;
    Client *clients;
    Food *currentOrder;
    int open;
    sem_t *semQueue;
    sem_t *semServed;
}FoodPlace;

typedef struct {
    Client *client;
    FoodPlace *mercadoChino;
}ClientOrder;

/* Thread functions */
void *streetThread(void *);
void *clientThread(void *);
void *cashierThread(void *);
void *cookThread(void *);
void *checkoutThread(void *);

/* Menu Functions */
Food *menuSetup();
Food *pickFood(Food * menu);
int getMaxWaitTime(Food *menu);


/*Semaphore init*/
int initSemaphore(FoodPlace *);
void destroySemaphore(FoodPlace*);

int main(){
    srand(time(0));

    FoodPlace *mercadoChino = (FoodPlace *)calloc(1,sizeof(FoodPlace));
    pthread_t sThread;
    pthread_t cThread;

    mercadoChino->menu = menuSetup();
    mercadoChino->open = 1;


    int err;

    err = initSemaphore(mercadoChino);

    if(!err){
        pthread_create(&sThread,NULL,streetThread,(void *)mercadoChino);
        pthread_create(&cThread,NULL,cashierThread,(void *)mercadoChino);



        pthread_join(sThread,NULL);
    }
    
    destroySemaphore(mercadoChino);

    free(mercadoChino);
    return 0;
}

int initSemaphore(FoodPlace *mercadoChino){

    int error = 0;

    mercadoChino->semQueue = sem_open("/semQueue", O_CREAT, O_RDWR, 0);

    if(mercadoChino->semQueue == SEM_FAILED) {
        perror("/semQueue");
        error = -1;
        return error;
    }

    if(error != -1) {
        mercadoChino->semServed = sem_open("/semServed", O_CREAT, O_RDWR, 0);
    }

    if(mercadoChino->semServed == SEM_FAILED) {
        perror("/semServed");
        error = -1;
        return error;
    }

    if(error != -1) {
        mercadoChino->cashier.semCook = sem_open("/cashier.semCook", O_CREAT, O_RDWR, 0);
    }

    if(mercadoChino->cashier.semCook == SEM_FAILED) {
        perror("/cashier.semCook");
        error = -1;
        return error;
    }

    if(error != -1) {
        mercadoChino->cashier.semCheckout = sem_open("/cashier.semCheckout", O_CREAT, O_RDWR, 0);
    }

    if(mercadoChino->cashier.semCheckout == SEM_FAILED) {
        perror("/cashier.semCheckout");
        error = -1;
        return error;
    }

    sem_post(mercadoChino->semQueue);


    

    return error;
}

void destroySemaphore(FoodPlace* mercadoChino){
   
    sem_close(mercadoChino->semQueue);
    sem_close(mercadoChino->semServed);
    sem_close(mercadoChino->cashier.semCook);
    sem_close(mercadoChino->cashier.semCheckout);

    sem_unlink("/semQueue");
    sem_unlink("/semServed");
    sem_unlink("/cashier.semCook");
    sem_unlink("/cashier.semCheckout");

}

void *streetThread(void *arg){
    FoodPlace *mercadoChino = (FoodPlace *)arg;
    int tolerance = getMaxWaitTime(mercadoChino->menu);

    pthread_t clientsThreads[CLIENTS];
    Client clients[CLIENTS];
    ClientOrder orders[CLIENTS];

    for (int i = 0; i < CLIENTS; i++){
        clients[i].id = i;
        clients[i].tolerance = tolerance;
        clients[i].order = pickFood(mercadoChino->menu);
        clients[i].semQueue = mercadoChino->semQueue;
        orders[i].client = &clients[i];
        orders[i].mercadoChino = mercadoChino;

        sleep(rand()%(3)+1);
        pthread_create(&clientsThreads[i],NULL,clientThread,&orders[i]);
    }

    for(int j = 0; j < CLIENTS; j++)
        pthread_join(clientsThreads[j],NULL);

    mercadoChino->open = 0;

    pthread_exit(NULL);
}

void *clientThread(void *arg){
    ClientOrder *order = (ClientOrder *)arg;
    FoodPlace *mercadoChino = order->mercadoChino;
    Client *client = order->client;

    printf("Nuevo cliente en cola: %d\r\n",client->id);

    struct timespec wait;
    clock_gettime(CLOCK_REALTIME, &wait);
    wait.tv_sec += client->tolerance;

    int errCode = sem_timedwait(client->semQueue, &wait);

    if(errCode == 0){
        printf("Cliente: %d ordena %s\r\n",client->id,client->order->name);
        /* Ingreso el pedido del cliente */
        mercadoChino->currentOrder = client->order;
        /* El cajero empieza a cocinar */
        sem_post(mercadoChino->cashier.semCook);
        /* El Cliente Espera que le cobren */
        sem_wait(mercadoChino->semServed);
        printf("El Cliente %d se retira con su orden\r\n",client->id);
        /* Libero el lugar en la cola para que pase el siguiente */
        sem_post(mercadoChino->semQueue);

    } else{
        printf("El Cliente %d se canso de esperar\r\n",client->id);
    }

    pthread_exit(NULL);
}

void *cashierThread(void *arg){
    FoodPlace *mercadoChino = (FoodPlace *)arg;

    while (1){
        pthread_t cook,checkout;
        pthread_create(&cook,NULL,cookThread,mercadoChino);
        pthread_create(&checkout,NULL,checkoutThread,mercadoChino);
        pthread_join(checkout,NULL);
    }

    pthread_exit(NULL);
}

void *cookThread(void *arg){
    FoodPlace *mercadoChino = (FoodPlace *) arg;

    sem_wait(mercadoChino->cashier.semCook);
    printf("Cocinando %s\r\n",mercadoChino->currentOrder->name);
    sleep(mercadoChino->currentOrder->prepTime);
    printf("Pedido Cocinado\r\n");
    sem_post(mercadoChino->cashier.semCheckout);

    pthread_exit(NULL);
}

void *checkoutThread(void *arg){
    FoodPlace *mercadoChino = (FoodPlace *) arg;

    sem_wait(mercadoChino->cashier.semCheckout);
    printf("Cobrando: %s\r\n",mercadoChino->currentOrder->name);
    sem_post(mercadoChino->semServed);

    pthread_exit(NULL);
}

Food *menuSetup(){
    Food *menu = calloc(10, sizeof(Food));

    sprintf(menu[0].name,"Pizza");
    menu[0].prepTime = 2;

    sprintf(menu[1].name,"Lomito");
    menu[1].prepTime = 2;

    sprintf(menu[2].name,"Empanadas");
    menu[2].prepTime = 5;

    sprintf(menu[3].name,"Ensalada");
    menu[3].prepTime = 4;

    sprintf(menu[4].name,"Milanesa");
    menu[4].prepTime = 3;

    sprintf(menu[5].name,"Sushi");
    menu[5].prepTime = 6;

    sprintf(menu[6].name,"Chop Suey");
    menu[6].prepTime = 3;

    sprintf(menu[7].name,"Pollo");
    menu[7].prepTime = 4;

    sprintf(menu[8].name,"Matambre");
    menu[8].prepTime = 3;

    sprintf(menu[9].name,"Choripan");
    menu[9].prepTime = 2;

    return menu;
}

Food *pickFood(Food *menu){
    return &menu[rand()%10];
}

int getMaxWaitTime(Food *menu){
    int min = menu[0].prepTime;

    for(int i = 0; i < 10; i++)
        if(menu[i].prepTime < min)
            min = menu[i].prepTime;
    return min * 4;
}