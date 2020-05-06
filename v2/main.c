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
#define CLIENTS 50
#define CASHIERS 3

typedef struct {
    char name[50];
    int prepTime;
}Food;

typedef struct {
    int id;
    int tolerance;
    Food *order;
    sem_t *semQueue;
}Client;

typedef struct {
    int id;
    int ocupado;
    sem_t *semServed;
    sem_t *semCook;
    sem_t *semCheckout;
    sem_t *semQueue;
    Client *client;
}Cashier;

typedef struct {
    Food *menu;
    Cashier *cashier;
    Client *clients;
    Food *currentOrder;
    int open;
    sem_t *semQueue;
    
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
    mercadoChino->cashier = (Cashier *)calloc(CASHIERS,sizeof(Cashier));
    
    
    pthread_t sThread;
    pthread_t *cThread = (pthread_t *)calloc(CASHIERS,sizeof(pthread_t));

    mercadoChino->menu = menuSetup();
    mercadoChino->open = 1;


    int err;

    err = initSemaphore(mercadoChino);

    if(!err){
        pthread_create(&sThread,NULL,streetThread,(void *)mercadoChino);


        for(int i = 0; i < CASHIERS; i++){
         
            mercadoChino->cashier[i].ocupado = 0;
            mercadoChino->cashier[i].id = i;
            mercadoChino->cashier[i].semQueue = mercadoChino->semQueue;

            
            pthread_create(&cThread[i],NULL,cashierThread,&mercadoChino->cashier[i]);
        }
        

        pthread_join(sThread,NULL);
    }
    
    destroySemaphore(mercadoChino);

    free(mercadoChino);
    return 0;
}

int initSemaphore(FoodPlace *mercadoChino){

    int error = 0;
    char str[50];

    // mercadoChino->semQueue = sem_open("/semQueue", O_CREAT, O_RDWR, 1);
    mercadoChino->semQueue = sem_open("/semQueue", O_CREAT, O_RDWR, 3);


    if(mercadoChino->semQueue == SEM_FAILED) {
        perror("/semQueue");
        error = -1;
        return error;
    }

    for(int i = 0; i < CASHIERS; i++){
        if(error != -1) {
            sprintf(str, "/cashier%d.semServed", i);
            mercadoChino->cashier[i].semServed = sem_open(str, O_CREAT, O_RDWR, 0);
        }

        if(mercadoChino->cashier[i].semServed == SEM_FAILED) {
            perror(str);
            error = -1;
            return error;
        }

        if(error != -1) {
            sprintf(str, "/cashier%d.semCook", i);
            mercadoChino->cashier[i].semCook = sem_open(str, O_CREAT, O_RDWR, 0);
        }

        if(mercadoChino->cashier[i].semCook == SEM_FAILED) {
            perror(str);
            error = -1;
            return error;
        }

        if(error != -1) {
            sprintf(str, "/cashier%d.semCheckout", i);
            mercadoChino->cashier[i].semCheckout = sem_open(str, O_CREAT, O_RDWR, 0);
        }

        if(mercadoChino->cashier[i].semCheckout == SEM_FAILED) {
            perror(str);
            error = -1;
            return error;
        }
    }

    
    // sem_wait(mercadoChino->semQueue);


    

    return error;
}

void destroySemaphore(FoodPlace* mercadoChino){
   
    sem_close(mercadoChino->semQueue);

    for(int i = 0; i < CASHIERS; i++){
        sem_close(mercadoChino->cashier[i].semServed);
        sem_close(mercadoChino->cashier[i].semCook);
        sem_close(mercadoChino->cashier[i].semCheckout);

    }
    

    sem_unlink("/semQueue");
    sem_unlink("/cashier0.semServed");
    sem_unlink("/cashier0.semCook");
    sem_unlink("/cashier0.semCheckout");

    sem_unlink("/cashier1.semServed");
    sem_unlink("/cashier1.semCook");
    sem_unlink("/cashier1.semCheckout");
    
    sem_unlink("/cashier2.semServed");
    sem_unlink("/cashier2.semCook");
    sem_unlink("/cashier2.semCheckout");

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

        sleep(rand()%(2));
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
    int num_cajero = -1;

    printf("Nuevo cliente en cola: %d\r\n",client->id);

    struct timespec wait;
    clock_gettime(CLOCK_REALTIME, &wait);
    wait.tv_sec += client->tolerance;

    int errCode = sem_timedwait(client->semQueue, &wait);
    // printf("!!! Cliente %d gano el mutex con %d\n", client->id,errCode);
    if(errCode == 0){
        for(int i = 0; i < CASHIERS; i++){
            if(mercadoChino->cashier[i].ocupado == 0){
                num_cajero = mercadoChino->cashier[i].id;
                i = CASHIERS;
            }
        }
        
        printf("Cliente: %d ordena %s al cajero numero %d.\r\n",client->id,client->order->name,(num_cajero));
        /* Ingreso el pedido del cliente */
        // mercadoChino->currentOrder = client->order;
        mercadoChino->cashier[num_cajero].client = client;
        /* El cajero empieza a cocinar */
        sem_post(mercadoChino->cashier[num_cajero].semCook);
        /* El Cliente Espera que le cobren */
        sem_wait(mercadoChino->cashier[num_cajero].semServed);
        printf("El Cliente %d se retira con su orden\r\n",client->id);
        /* Libero el lugar en la cola para que pase el siguiente */
        sem_post(mercadoChino->semQueue);

    } else{
        printf("El Cliente %d se canso de esperar\r\n",client->id);
    }

    pthread_exit(NULL);
}

void *cashierThread(void *arg){
    Cashier *cashier = (Cashier *)arg;
    // sem_post(cashier->semQueue);
    while (1){
        // if(cashier->ocupado == 0){
            
        //     pthread_t cook,checkout;
        //     pthread_create(&cook,NULL,cookThread,cashier);
        //     pthread_create(&checkout,NULL,checkoutThread,cashier);
        //     pthread_join(checkout,NULL);
        //     sem_post(cashier->semQueue);
        // }
        

        pthread_t cook,checkout;
        pthread_create(&cook,NULL,cookThread,cashier);
        pthread_create(&checkout,NULL,checkoutThread,cashier);
        pthread_join(checkout,NULL);
        // sem_post(cashier->semQueue);



    }

    pthread_exit(NULL);
}

void *cookThread(void *arg){
    Cashier *cashier = (Cashier *)arg;
    sem_wait(cashier->semCook);
    cashier->ocupado = 1;
    printf("Cocinando %s por Cajero %d\r\n",cashier->client->order->name,cashier->id);
    sleep(cashier->client->order->prepTime);
    printf("Pedido del Cliente %d cocinado por Cajero %d\r\n",cashier->client->id,cashier->id);
    sem_post(cashier->semCheckout);

    pthread_exit(NULL);
}

void *checkoutThread(void *arg){
    Cashier *cashier = (Cashier *)arg;

    sem_wait(cashier->semCheckout);
    printf("Cobrando: %s por Cajero %d\r\n",cashier->client->order->name,cashier->id);
    cashier->ocupado = 0;
    
    sem_post(cashier->semServed);
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