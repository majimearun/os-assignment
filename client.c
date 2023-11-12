#include <errno.h>
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
    char mtext[100];
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
    if ((msqid = msgget(key, PERMS)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    while (1)
    {
        msg.mtype = 4;

        //  send step:

        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");

        int sequence_number = 0;
        int operation_number = 0;
        char mtext[100] = "";

        printf("Enter Sequence Number: ");
        scanf("%d", &sequence_number);
        msg.Sequence_Number = sequence_number;

        printf("Enter Operation Number: ");
        scanf("%d", &operation_number);
        msg.Operation_Number = operation_number;

        printf("Enter Graph File Name: ");
        scanf("%s", mtext);
        strcpy(msg.mtext, mtext);

        printf("--------------------\n");
        printf("client: %d\n", msg.Sequence_Number);
        printf("operation: %d\n", msg.Operation_Number);
        printf("file name: %s\n", msg.mtext);
        printf("mtype: %ld\n", msg.mtype);
        printf("--------------------\n");

        if (msgsnd(msqid, &msg, sizeof(message), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        // receive step:
        if (msgrcv(msqid, &msg, sizeof(message), sequence_number * 10, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        printf("%s\n", msg.mtext);
    }
    return 0;
}