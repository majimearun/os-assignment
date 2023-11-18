#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PERMS 0644
#define BUF_SIZE 1024
#define MAX_THREADS 100

typedef struct message
{
    long mtype;
    char contents[100];
    int Sequence_Number;
    int Operation_Number;

} message;

typedef struct ThreadData
{
    int msqid;
    message msg;
} ThreadData;

sem_t *sem;
sem_t *sem1;
sem_t *sem2;

void *func(void *data)
{
    ThreadData *td = (ThreadData *)data;
    int msqid = td->msqid;
    message msg = td->msg;

    key_t key_shm;
    int shmid;

    sem = sem_open(msg.contents, O_CREAT, PERMS, 1);
    if (sem == SEM_FAILED)
    {
        if (errno != EEXIST)
        {
            perror("sem_open");
            exit(1);
        }
        else
        {
            sem = sem_open(msg.contents, 0);
            if (sem == SEM_FAILED)
            {
                perror("sem_open");
                exit(1);
            }
        }
    }

    char name1[100] = "";
    strcat(name1, msg.contents);
    strcat(name1, " 1\0");
    sem1 = sem_open(name1, O_CREAT, PERMS, 1);
    if (sem1 == SEM_FAILED)
    {
        if (errno != EEXIST)
        {
            perror("sem_open");
            exit(1);
        }
        else
        {
            sem1 = sem_open(name1, 0);
            if (sem1 == SEM_FAILED)
            {
                perror("sem_open");
                exit(1);
            }
        }
    }

    char name2[100] = "";
    strcat(name2, msg.contents);
    strcat(name2, " 2\0");
    sem2 = sem_open(name2, O_CREAT, PERMS, 1);
    if (sem2 == SEM_FAILED)
    {
        if (errno != EEXIST)
        {
            perror("sem_open");
            exit(1);
        }
        else
        {
            sem2 = sem_open(name2, 0);
            if (sem2 == SEM_FAILED)
            {
                perror("sem_open");
                exit(1);
            }
        }
    }

    sem_wait(sem);
    sem_wait(sem1);
    sem_wait(sem2);

    if ((key_shm = ftok("load_balancer.c", msg.Sequence_Number)) == -1)
    {
        perror("error\n");
        exit(1);
    }

    if ((shmid = shmget(key_shm, sizeof(char[BUF_SIZE]), 0666)) == -1)
    {
        perror("shared memory");
        exit(1);
    }

    char *shm = (char *)shmat(shmid, NULL, 0);
    if (shm == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int numNodes = atoi(strtok(shm, "\n"));

    FILE *graphFile = fopen(msg.contents, "w");
    if (graphFile == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    fprintf(graphFile, "%d\n", numNodes);

    for (int i = 0; i < numNodes; i++)
    {
        char *adjrow = strtok(NULL, "\n");
        if (adjrow == NULL)
        {
            perror("Error reading from shared memory");
            exit(1);
        }
        fprintf(graphFile, "%s\n", adjrow);
    }

    fclose(graphFile);

    if (shmdt(shm) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    if (msg.Operation_Number == 1)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "File successfully added\n";
        strcpy(msg.contents, mess);
    }

    else if (msg.Operation_Number == 2)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "File successfully modified\n";
        strcpy(msg.contents, mess);
    }

    if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    sem_post(sem);
    sem_post(sem1);
    sem_post(sem2);

    free(td);
    pthread_exit(NULL);
}

int main()
{
    key_t key;
    if ((key = ftok("load_balancer.c", 'A')) == -1)
    {
        perror("ftok");
        exit(1);
    }

    int msqid;
    if ((msqid = msgget(key, PERMS)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    pthread_t threads[MAX_THREADS];
    int n_threads = 0;

    while (1)
    {
        message msg;
        if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), 3, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        if (msg.Operation_Number == 0)
        {
            for (int i = 0; i < n_threads; i++)
            {
                printf("Waiting for thread %d to finish...\n", i);
                pthread_join(threads[i], NULL);
            }
            break;
        }

        else
        {
            ThreadData *td = (ThreadData *)malloc(sizeof(ThreadData));
            td->msqid = msqid;
            td->msg = msg;

            pthread_create(&threads[n_threads++], NULL, func, (void *)td);
        }
    }
    sem_close(sem);
    return 0;
}