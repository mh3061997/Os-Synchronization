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
};

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
int shmid;

struct Buffer
{
    int Buffersize;
    int *Buffer;
    int count;
};

int main()
{
    //setting fn handler for sigint to free ipc
    /*HANDLER FN SEE DONT SEE CHANGES IN GLOBAL VAR
     AFTER HANDLER IS ASSIGNED
    */
    signal(SIGINT, handler);
    sleep(2); // will be deleted just to invoke sigint to delete shm

    //Buffer Initialization
    int Buffersize;
    printf("Enter Buffer Size:", &Buffersize);
    scanf("%d", &Buffersize);
    struct Buffer *myBuffer = (struct Buffer *)malloc(sizeof(struct Buffer));
    myBuffer->Buffer = (int *)malloc(sizeof(int)*Buffersize);
    myBuffer->count = 0;
    // struct Buffer *mybuffer;

    printf("%d \n ", sizeof(struct Buffer) + sizeof(int)*Buffersize);

    //pid
    pid_t pid;

    //unique ipc key
    key_t keymsg = ftok(getenv("PWD"), 6); //PWD path to current directory , 99 any unique number
    key_t keyshm = ftok(getenv("PWD"), 5); //PWD path to current directory , 98 any unique number
    key_t keysem = ftok(getenv("PWD"), 4); //PWD path to current directory , 97 any unique number

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
    size_t Sizetotal =  sizeof(struct Buffer) + sizeof(int)*Buffersize;
    //IF ERROR THEN DELETE SHARED MEMORY FIRST
    //You cannot attach a segment that already exists with a larger size.
    //So you need to remove it first or use a different key.
    shmid = shmget(keyshm, Sizetotal, IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error in creating shared memory !");
        exit(-1);
    }
    else
    {
        printf("Created Shared memory ID = %d\n", shmid);
    }

    //shared memory address attach to process
    void *shmaddr = shmat(shmid, (void *)0, 0);
    struct Buffer *mybuf = shmaddr;
    printf("total size %d \n",mybuf->Buffer);
    mybuf->Buffer[0] = 65;
    //myBuffer->Buffer[1]=6;
    if (shmaddr == -1)
    {
        perror("Error in attaching memory");
        exit(-1);
    }
    else
    {
        printf("Shared memory attached at address %x\n", shmaddr);
    }

    //initialize semaphore

    //1 is number of semaphores
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

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
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
    sleep(5);

    /*
    while(1){

        //if buffer empty 
        //produce 
        //then send msg to consumer
        //telling buffer is now not empty
        if(){

        }
        //if buffer is full
        //wait for msg from consumer
        //telling buffer is now not full
        //then produce
        else if(){

        }
        //if buffer not empty nor full
        //produce 
        else{

        }
    }
    */

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
        printf("Success Message Queue Freed \n");
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
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)0) == -1)
    {
        perror("Error Deleting Shared Memory !\n");
    }
    else
    {
        printf("Success Shared Memory Freed\n");
    }
    exit(1);
}