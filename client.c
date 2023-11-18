#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PERMS 0644
#define BUF_SIZE 1024

typedef struct message
{
    long mtype;
    char contents[100];
    int Sequence_Number;
    int Operation_Number;

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

        if (msg.Operation_Number == 1 || msg.Operation_Number == 2)
        {

            key_t key_shm;
            int shmid;
            char *shm;

            if ((key_shm = ftok("load_balancer.c", msg.Sequence_Number)) == -1)
            {
                perror("error\n");
                exit(1);
            }

            if ((shmid = shmget(key_shm, sizeof(char[BUF_SIZE]), 0666 | IPC_CREAT)) == -1)
            {
                perror("shared memory");
                return 1;
            }

            shm = (char *)shmat(shmid, NULL, 0);

            char n[100] = "";

            printf("Enter number of nodes of the graph : ");
            scanf("%s", n);

            sprintf(shm, "%s\n", n);

            char adjrow[100] = "";

            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;

            printf("Enter adjacency matrix, each row on a separate line and elements of a single row separated by "
                   "whitespace characters: \n");

            for (int i = 0; i < atoi(n); i++)
            {
                scanf(" %[^\n]", adjrow);

                while ((c = getchar()) != '\n' && c != EOF)
                    ;

                sprintf(shm + strlen(shm), "%s\n", adjrow);
            }

            if (shmdt(shm) == -1)
            {
                perror("shmdt");
                return 1;
            }

            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), sequence_number * 10, 0) == -1)
            {
                perror("msgrcv");
                exit(1);
            }
            printf("%s\n", msg.contents);

            if (shmctl(shmid, IPC_RMID, 0) == -1)
            {
                perror("shmctl");
                return 1;
            }
        }
        else
        {

            key_t key_shm;
            int shmid;
            char *shm;

            if ((key_shm = ftok("load_balancer.c", msg.Sequence_Number)) == -1)
            {
                perror("error\n");
                exit(1);
            }

            if ((shmid = shmget(key_shm, sizeof(char[BUF_SIZE]), 0666 | IPC_CREAT)) == -1)
            {
                perror("shared memory");
                return 1;
            }

            shm = (char *)shmat(shmid, NULL, 0);

            char start[100] = "";
            printf("Enter the starting node: ");
            scanf("%s", start);

            sprintf(shm, "%s\n", start);

            if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), sequence_number * 10, 0) == -1)
            {
                perror("msgrcv");
                exit(1);
            }
            printf("%s\n", msg.contents);

            char *op = strtok(shm, "\n");

            char *startPtr = strstr(op, start);
            if (startPtr != NULL)
            {
                memmove(startPtr, startPtr + strlen(start), strlen(startPtr + strlen(start)) + 1);
            }

            if (msg.Operation_Number == 3)
            {
                printf("The leaf nodes are: \n%s\n", op);
            }
            else
            {
                printf("Output of the BFS traversal: \n");
                printf("%s\n", op);
            }

            if (shmdt(shm) == -1)
            {
                perror("shmdt");
                return 1;
            }

            if (shmctl(shmid, IPC_RMID, 0) == -1)
            {
                perror("shmctl");
                return 1;
            }
        }
    }

    return 0;
}