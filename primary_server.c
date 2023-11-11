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
// 2 - load balancer to secondary server 1 (odd requests)
// 1 - load balancer to secondary server 2 (even requests)
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
    if ((msqid = msgget(key, PERMS)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    while (1)
    {
        message msg;
        if (msgrcv(msqid, &msg, sizeof(message), 3, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        pid_t child_server;

        if ((child_server = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }

        if (child_server == 0)
        {

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
        }
        else
        {
            wait(NULL);
        }
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }

    return 0;
}