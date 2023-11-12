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
    long mtype;           // Denotes who needs to receive the message
    char contents[100];   // Graph File Name or Server Response
    int Sequence_Number;  // Request number
    int Operation_Number; // Operation to be performed

} message;

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

    while (1)
    {
        message msg;
        msg.mtype = 4;

        //  send step:
        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");
        printf("3. Perform DFS on an existing graph of the database\n");
        printf("4. Perform BFS on an existing graph of the database\n");

        int sequence_number = 0;
        int operation_number = 0;
        char contents[100] = "";

        printf("Enter Sequence Number: ");
        scanf("%d", &sequence_number);
        msg.Sequence_Number = sequence_number;

        printf("Enter Operation Number: ");
        scanf("%d", &operation_number);
        msg.Operation_Number = operation_number;

        printf("Enter Graph File Name: ");
        scanf("%s", contents);
        strcpy(msg.contents, contents);

        if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        // receive step:
        if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), sequence_number * 10, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }
        printf("%s\n", msg.contents);
    }
    return 0;
}