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
    char contents[100]; // Graph File Name or Server Response
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

        //  send step:
        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");
        printf("3. Perform DFS on an existing graph of the database\n");
        printf("4. Perform BFS on an existing graph of the database\n");

        int sequence_number = 0;
        int operation_number = 0;
        char contents[100] = "";

        // TODO: ensure input are in correct range

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

            if ((key_shm = ftok("testing.txt", msg.Sequence_Number)) == -1)
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

            char n[100] = ""; // no of nodes which we have to write into shm[0], add a new line too

            printf("Enter number of nodes of the graph : ");
            scanf("%s", n);

            // Write the number of nodes to shared memory
            sprintf(shm, "%s\n", n);

            char adjrow[100] = "";

            // Clear the stdin buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;

            printf("Enter adjacency matrix, each row on a separate line and elements of a single row separated by "
                   "whitespace characters: \n");

            for (int i = 0; i < atoi(n); i++)
            {
                scanf(" %[^\n]", adjrow); // Notice the space before % to skip leading whitespaces

                // Clear the stdin buffer
                while ((c = getchar()) != '\n' && c != EOF)
                    ;

                // Append each row into shared memory along with the new line
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

            // receive step:
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
    }

    return 0;
}