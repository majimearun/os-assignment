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
    if ((msqid = msgget(key, PERMS)) == -1)
    {
        perror("msgget");
        exit(1);
    }

    while (1)
    {
        int choice;

        printf("\n\n");
        printf("Want to terminate the application? Press Y (Yes) or N (No) : ");
        char c;
        scanf(" %c", &c);
        if (c == 'Y' || c == 'y')
        {
            msg.Operation_Number = 0;
            msg.mtype = 4;
            if (msgsnd(msqid, &msg, sizeof(msg), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
            break;
        }
    }
    return 0;
}