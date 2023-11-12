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
        if (msgrcv(msqid, &msg, sizeof(message), 4, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
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

        else if (msg.Operation_Number == 0)
        {
            msg.mtype = 3;
            printf("Forwarding cleanup request to primary server\n");
            if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            sleep(5);
            break;
        }
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }

    printf("message queue deleted\n");
    return 0;
}