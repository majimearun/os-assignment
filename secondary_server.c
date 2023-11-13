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
    long mtype;           // Denotes who needs to receive the message
    char contents[100];   // Graph File Name or Server Response
    int Sequence_Number;  // Request number
    int Operation_Number; // Operation to be performed

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

    if (msg.Operation_Number == 3)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "DFS successfully performed\n";
        strcpy(msg.contents, mess);
    }

    else if (msg.Operation_Number == 4)
    {
        msg.mtype = msg.Sequence_Number * 10;
        char mess[100] = "BFS successfully performed\n";
        strcpy(msg.contents, mess);
    }

    if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    free(td);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Please enter the server number as an argument\n");
        exit(1);
    }

    int server_number = atoi(argv[1]);

    if (server_number != 1 && server_number != 2)
    {
        printf("Please enter a valid server number (1 or 2)\n");
        exit(1);
    }

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
        if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), server_number, 0) == -1)
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
    return 0;
}