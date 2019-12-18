#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <math.h>
//sigint handler declaration
void handler(int signum);

//message buffer struct
struct msgbuff
{
    long mtype;
    char mtext[70];
} message;

/* arg for semctl system calls. */
union Semun {
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

//////////////////////////////////////////
void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}

void do_child(int sem1, int sem2)
{
    printf("A\n");
    up(sem1);
    printf("B\n");
    down(sem2);
    printf("C\n");
}

void do_parent(int sem1, int sem2)
{
    printf("G\n");
    down(sem1);
    printf("F\n");
    up(sem2);
    printf("E\n");
}

key_t msgqid;
int sem;
//argument for sem ops
union Semun semun;
int shmidstack;
int shmidcount;
int shmidbuffersize;
int Buffersize;
int *Buffer;
int count;

int main()
{
    //setting fn handler for sigint to free ipc
    /*HANDLER FN SEE DONT SEE CHANGES IN GLOBAL VAR
     AFTER HANDLER IS ASSIGNED
    */
    signal(SIGINT, handler);
    sleep(2); // will be deleted just to invoke sigint to delete shm

    //pid
    pid_t pid;

    //unique ipc key
    key_t keymsg = ftok(getenv("PWD"), 18);      //PWD path to current directory , 99 any unique number
    key_t keyshmstack = ftok(getenv("PWD"), 19); //PWD path to current directory , 98 any unique number
    key_t keyshmcount = ftok(getenv("PWD"), 20); //PWD path to current directory , 98 any unique number
    key_t keysem = ftok(getenv("PWD"), 21);      //PWD path to current directory , 97 any unique number
    key_t keybufsize = ftok(getenv("PWD"), 22);  //PWD path to current directory , 97 any unique number

    //initialize message queue
    //IPC_CREAT creates msg if not exist if exist return it's id
    //0666 is read and write permisions for everyone
    //key is  unique key
    msgqid = msgget(keymsg, IPC_CREAT | 0666);
    if (msgqid == -1)
    {
        perror("Error creating message queue");
        exit(-1);
    }
    else
    {
        printf("Created msgqid = %d\n", msgqid);
    }

    //initialize shared memory
    //4096 is segment size

    //IF ERROR THEN DELETE SHARED MEMORY FIRST
    //You cannot attach a segment that already exists with a larger size.
    //So you need to remove it first or use a different key.

    //Buffer Initialization
    printf("Enter Buffer Size:");
    scanf("%d", &Buffersize);
    count = 0;

    shmidcount = shmget(keyshmcount, sizeof(int), IPC_CREAT | 0666);
    if (shmidcount == -1)
    {
        perror("Error in creating shared memory !");
        exit(-1);
    }
    else
    {
        printf("Created Shared memory ID = %d\n", shmidcount);
    }
    printf("size is %d \n", sizeof(int) * Buffersize);

    //shared memory address attach to process
    int *shmaddrcount = shmat(shmidcount, (void *)0, 0);
    if (shmaddrcount == -1)
    {
        perror("Error in attaching memory");
        exit(-1);
    }
    else
    {
        printf("Shared memory attached at address %x\n", shmaddrcount);
    }

    /////
    shmidbuffersize = shmget(keybufsize, sizeof(int), IPC_CREAT | 0666);
    if (shmidbuffersize == -1)
    {
        perror("Error in creating shared memory !");
        exit(-1);
    }
    else
    {
        printf("Created Shared memory ID = %d\n", shmidbuffersize);
    }

    //shared memory address attach to process
    int *shmaddrbuffersize = shmat(shmidbuffersize, (void *)0, 0);
    if (shmaddrbuffersize == -1)
    {
        perror("Error in attaching memory");
        exit(-1);
    }
    else
    {
        printf("Shared memory attached at address %x\n", shmaddrbuffersize);
    }
    /////

    int totalsize = sizeof(int) * Buffersize;
    shmidstack = shmget(keyshmstack, totalsize, IPC_CREAT | 0666);
    if (shmidstack == -1)
    {
        perror("Error in creating shared memory !");
        exit(-1);
    }
    else
    {
        printf("Created Shared memory ID = %d\n", shmidstack);
    }

    //shared memory address attach to process
    int *shmaddrstack = shmat(shmidstack, (void *)0, 0);
    if (shmaddrstack == -1)
    {
        perror("Error in attaching memory");
        exit(-1);
    }
    else
    {
        printf("Shared memory attached at address %x\n", shmaddrstack);
    }

    /////////////////////
    (*shmaddrbuffersize) = Buffersize;
    //count++;
    (*shmaddrcount) = count;

    //shmaddrstack[1]=876;

    /////////////////////
    //initialize semaphore

    // //1 is number of semaphores
    sem = semget(keysem, 1, IPC_CREAT | 0666);

    if (sem == -1)
    {
        perror("Error  creating semaphore");
        exit(-1);
    }
    else
    {
        printf("Created Semaphore  %d\n", sem);
    }

    semun.val = 1; /* initial value of the semaphore, binary semaphore */
    //setting value of semaphore
    if (semctl(sem, 0, SETVAL, semun) == -1)
    {
        printf("Error in semctl");
        exit(-1);
    }
    else
    {
        printf("semaphore value set %d\n ", semun.val);
    }
    printf("buffersize is %d\n", Buffersize);

    printf("\n\n================WORKING NOW===================\n\n");
    int firstrun =1;
    while (1)
    {
        sleep(2);
        //if buffer empty
        //produce
        //then send msg to consumer
        //telling buffer is now not empty

        //reload count
        count = *shmaddrcount;
        if (count == 0)
        {
            printf("Buffer Empty producing\n");
            //create item
            int item = rand() % 100;
            // //reload count
            // down(sem);
            // count = *shmaddrcount;
            // up(sem);
            //insert item in shared memory buffer
            down(sem);
            shmaddrstack[count] = item;
            up(sem);
            count++;
            down(sem);
            (*shmaddrcount) = count;
            up(sem);
            //Semaphore up count

            // semun.val = count + 1; /* initial value of the semaphore, Counting semaphore */
            //                        //setting value of semaphore
            // if (semctl(sem, 0, SETVAL, semun) == -1)
            // {
            //     printf("Error in semctl");
            //     exit(-1);
            // }
            // else
            // {
            //     // printf("semaphore value set %d\n ", semun.val);
            // }

            // (*shmaddrcount)++; // increment count of elements in buffer

            printf("Produced %d %d,count is %d \n", item, shmaddrstack[count - 1], *shmaddrcount);
            if(firstrun!=1){
                //send msg to consumer
            strcpy(message.mtext, "notemptyanymore");
            message.mtype = 88; //anynumber for prod
            int send_val = msgsnd(msgqid, &message, sizeof(message), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
            }
            firstrun=0;
        }
        //if buffer is full
        //wait for msg from consumer
        //telling buffer is now not full
        //then produce
        else if (count >= Buffersize)
        {

            printf("Buffer full waiting !\n");
            //wait for msg from consumer
            //77 is any number sent by consumer
            //can be replaced with pid
            int rec_val = msgrcv(msgqid, &message, sizeof(message), 77, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
            else
                printf("\nMessage received: %s\n", message.mtext);

            //reload count
            count = *shmaddrcount;

            int item = rand() % 100;
            //insert item in shared memory buffer
            down(sem);
            shmaddrstack[count] = item;
            up(sem);
            count++;
            down(sem);
            *shmaddrcount = count;
            up(sem);
            //Semaphore up count
            // semun.val = count; /* initial value of the semaphore, Counting semaphore */
            //                    //setting value of semaphore
            // if (semctl(sem, 0, SETVAL, semun) == -1)
            // {
            //     printf("Error in semctl");
            //     exit(-1);
            // }
            // else
            // {
            //     // printf("semaphore value set %d\n ", semun.val);
            // }

            // (*shmaddrcount)++; // increment count of elements in buffer

            printf("Produced %d %d,count is %d \n", item, shmaddrstack[count - 1], *shmaddrcount);
        }
        //if buffer not empty nor full
        //produce
        else
        {
            //reload count
            count = *shmaddrcount;
            printf("Not empty nor full producing XD\n");
            int item = rand() % 100;
            //insert item in shared memory buffer
            down(sem);
            shmaddrstack[count] = item;
            up(sem);
            count++;
            down(sem);
            *shmaddrcount = count;
            up(sem);
            //Semaphore up count
            // semun.val = count; /* initial value of the semaphore, Counting semaphore */
            //                    //setting value of semaphore
            // if (semctl(sem, 0, SETVAL, semun) == -1)
            // {
            //     printf("Error in semctl");
            //     exit(-1);
            // }
            // else
            // {
            //     // printf("semaphore value set %d\n ", semun.val);
            // }

            // (*shmaddrcount)++; // increment count of elements in buffer
            printf("Produced %d %d,count is %d \n", item, shmaddrstack[count - 1], *shmaddrcount);
        }
    }

    return 0;
}

//handler for sigint
void handler(int signum)
{

    //free message queue
    if (msgctl(msgqid, IPC_RMID, (struct msqid_ds *)0) == -1)
    {
        printf("\nError Deleting msg queue  !\n");
    }
    else
    {
        printf("\nSuccess Message Queue Freed \n");
    }

    //free Semaphore
    if (semctl(sem, 0, SETVAL, semun) == -1)
    {
        printf("Error Deleting semaphore !\n");
    }
    else
    {
        printf("Success Semaphore Freed\n");
    }

    //free shared memory
    if (shmctl(shmidcount, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("Error Deleting Shared Memory !\n");
    }
    else
    {
        printf("Success Shared Memory Freed\n");
    }
    if (shmctl(shmidstack, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("Error Deleting Shared Memory !\n");
    }
    else
    {
        printf("Success Shared Memory Freed\n");
    }
    if (shmctl(shmidbuffersize, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("Error Deleting Shared Memory !\n");
    }
    else
    {
        printf("Success Shared Memory Freed\n");
    }
    exit(1);
}