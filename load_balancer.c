#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
    if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    while (1)
    {
        if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), 4, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        // forwarding cleanup requests
        if (msg.Operation_Number == 0)
        {
            msg.mtype = 3;
            printf("Forwarding cleanup request to primary server...\n");
            fflush(stdout);
            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            msg.mtype = 2;
            printf("Forwarding cleanup request to secondary server 1...\n");
            fflush(stdout);
            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            msg.mtype = 1;
            printf("Forwarding cleanup request to secondary server 2...\n");
            fflush(stdout);
            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            sleep(5);
            break;
        }

        // forwarding write requests
        else if (msg.Operation_Number == 1 || msg.Operation_Number == 2)
        {
            msg.mtype = 3;
            printf("Forwarding write request to primary server...\n");
            fflush(stdout);
            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
        }

        else
        {
            if (msg.Sequence_Number % 2)
            {
                msg.mtype = 1;
                printf("Forwarding read request to secondary server 1...\n");
                fflush(stdout);
                if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
                {
                    perror("msgsnd");
                    exit(1);
                }
            }
            else
            {
                msg.mtype = 2;
                printf("Forwarding read request to secondary server 2...\n");
                fflush(stdout);
                if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
                {
                    perror("msgsnd");
                    exit(1);
                }
            }
        }
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }

    printf("message queue deleted, cleanup process completed...\n");
    printf("load balancer exiting...\n");
    return 0;
}