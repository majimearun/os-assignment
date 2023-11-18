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
    char contents[100];
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
        int choice;

        printf("\n\n");
        printf("Want to terminate the application? Press Y (Yes) or N (No) : ");
        char c;
        scanf(" %c", &c);
        if (c == 'Y' || c == 'y')
        {
            msg.Operation_Number = 0;
            msg.mtype = 4;
            if (msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }
            break;
        }
    }
    return 0;
}