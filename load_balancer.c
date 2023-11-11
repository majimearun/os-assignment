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
    long mtype;
    char mtext[100]; // Graph File Name or Server Response
    int Sequence_Number;
    int Operation_Number;

} message;

// MTYPE INDEX BEING USED
// 4 - client to load balancer
// 3 - load balancer to primary server
// 1 - load balancer to secondary server 1 (odd requests)
// 2 - load balancer to secondary server 2 (even requests)
// sequence number * 10 - load balancer to client

int main()
{
    message msg;

    key_t key;
    if ((key = ftok("testing.txt", 'A')) == -1)
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
        message msg;
        if (msgrcv(msqid, &msg, sizeof(message), 4, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        // cleanup step
        if (msg.Operation_Number == 0)
        {
            break;
        }

        // forwarding write requests
        if (msg.Operation_Number == 1 || msg.Operation_Number == 2)
        {
            msg.mtype = 3;
            printf("Forwarding write request to primary server\n");
            if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
        }

        // forwarding read requests with odd sequence numbers
        else if (msg.Sequence_Number % 2)
        {
            msg.mtype = 1;
            printf("Forwarding read request to secondary server 1\n");
            if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
        }

        // forwarding read requests with even sequence numbers
        else
        {
            msg.mtype = 2;
            printf("Forwarding read request to secondary server 2\n");
            if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
        }
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }

    return 0;
}