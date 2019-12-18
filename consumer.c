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
    sleep(2);
    //setting fn handler for sigint to free ipc
    /*HANDLER FN SEE DONT SEE CHANGES IN GLOBAL VAR
     AFTER HANDLER IS ASSIGNED
    */
    signal(SIGINT, handler);

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
    Buffersize = *shmaddrbuffersize;
    printf("Buffersize is %d \n", Buffersize);
    printf("size is %d \n", sizeof(int) * Buffersize);

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
    printf("buffersize is %d\n", *shmaddrbuffersize);
    printf("count is %d\n", *shmaddrcount);
    // printf("stack[1] is %d\n", shmaddrstack[1]);

    /////////////////////
    //initialize semaphore

    // //1 is number of semaphores
    // sem = semget(keyshmcount, 1, IPC_CREAT | 0666);

    // if (sem == -1)
    // {
    //     perror("Error  creating semaphore");
    //     exit(-1);
    // }
    // else
    // {
    //     printf("Created Semaphore  %d\n", sem);
    // }

    // semun.val = 0; /* initial value of the semaphore, counting semaphore */
    //                //setting value of semaphore
    // if (semctl(sem, 0, SETVAL, semun) == -1)
    // {
    //     printf("Error in semctl");
    //     exit(-1);
    // }
    // else
    // {
    //     printf("semaphore value set %d\n ", semun.val);
    // }

    printf("\n\n================WORKING NOW===================\n\n");

    while (1)
    {
        sleep(1);
        //if buffer empty
        //wait for msg from producer
        //telling that it has produced
        //then consume
        count = *shmaddrcount;
        if (count == 0)
        {
            printf("Buffer Empty consumer waiting\n");

            //waiting for message from producer
            //88 is unique number for producer
            //can be replaced with producer pid
            int rec_val = msgrcv(msgqid, &message, sizeof(message), 88, !IPC_NOWAIT);
            if (rec_val == -1)
                perror("Error in receive");
            else
                printf("\nMessage received: %s\n", message.mtext);

            //reload count
            count = *shmaddrcount;
            //consuming
            int itemconsumed = -1;
            itemconsumed = shmaddrstack[0];
            count--;
            (*shmaddrcount)--;

             //shift all elements in buffer
            for(int i=0;i<count;i++){
                shmaddrstack[i]=shmaddrstack[i+1];
            }
            //semaphore down count
            //  down(sem);
            //decrementing count
            //(*shmaddrcount)--;
            printf("Consumed item %d count now %d\n", itemconsumed, *shmaddrcount);
        }
        //if buffer full
        //consume
        //send a msg to producer telling buffer is no longer full
        else if (count == Buffersize)
        {
            //reload count
            count = *shmaddrcount;
            printf("Buffer Full consuming and sending msg\n");
            //consuming
            int itemconsumed = -1;
            itemconsumed = shmaddrstack[0];
            count--;
            (*shmaddrcount)--;

             //shift all elements in buffer
            for(int i=0;i<count;i++){
                shmaddrstack[i]=shmaddrstack[i+1];
            }
            //semaphore down count
            // down(sem);
            //decrementing count
            //(*shmaddrcount)--;
            printf("Consumed item %d count now %d\n", itemconsumed, *shmaddrcount);

            //send msg to producer telling buffer now not full
            //send msg to consumer
            strcpy(message.mtext, "notfullanymore");
            message.mtype = 77; //anynumber for consumer
            int send_val = msgsnd(msgqid, &message, sizeof(message), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
        }
        //if not empty nor full
        //consume
        else
        {
            //reload count
            count = *shmaddrcount;
            printf("Not empty nor full CONSUMING XD\n");
            //consuming
            int itemconsumed = -1;
            itemconsumed = shmaddrstack[0];
            count--;
            (*shmaddrcount)--;

            //shift all elements in buffer
            for(int i=0;i<count;i++){
                shmaddrstack[i]=shmaddrstack[i+1];
            }
            //semaphore down count
            //  down(sem);
            //decrementing count
            //(*shmaddrcount)--;
            printf("Consumed item %d count now %d\n", itemconsumed, *shmaddrcount);
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