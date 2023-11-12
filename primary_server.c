#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PERMS 0644

typedef struct message
{
    long mtype;
    char mtext[100]; // Graph File Name or Server Response
    int Sequence_Number;
    int Operation_Number;

} message;

typedef struct ThreadData
{
    int msqid;
    message msg;
} ThreadData;

#define MAX_THREADS 100

void *func(void *data)
{
    ThreadData *td = (ThreadData *)data;
    int msqid = td->msqid;
    message msg = td->msg;

    printf("client: %d\n", msg.Sequence_Number);
    printf("operation: %d\n", msg.Operation_Number);
    printf("file name: %s\n", msg.mtext);
    printf("--------------------\n");

    if (msg.Operation_Number == 1)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "File successfully added\n";
        strcpy(msg.mtext, mess);
    }

    else if (msg.Operation_Number == 2)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "File successfully modified\n";
        strcpy(msg.mtext, mess);
    }

    if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    free(td);
    pthread_exit(NULL);
}

int main()
{
    message msg;

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
        if (msgrcv(msqid, &msg, sizeof(message), 3, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        if (msg.Operation_Number == 0)
        {
            for (int i = 0; i < n_threads; i++)
            {
                printf("Waiting for thread %d to finish\n", i);
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
    return 0;
}