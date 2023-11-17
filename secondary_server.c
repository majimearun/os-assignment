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
#define MAX_THREADS 100
#define BUF_SIZE 1024

int extractNumber(const char *input)
{
    int number;
    // Using sscanf to extract the number from the string
    sscanf(input, "G%d.txt", &number);
    return number - 1;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct message
{
    long mtype;           // Denotes who needs to receive the message
    char contents[100];   // Graph File Name or Server Response
    int Sequence_Number;  // Request number
    int Operation_Number; // Operation to be performed

} message;

typedef struct ThreadData
{
    int msqid;
    message msg;
    int n_threads;
} ThreadData;

sem_t *sem;
sem_t *sem_read;
int n_readers[30];
typedef struct DFSGraph
{
    int num_nodes;         // number of nodes in the DFSGraph
    int *visited;          // visited array
    int **matrix;          // adjacency matrix
    int *leaves;           // array to store the leaf nodes
    int n_leaves;          // number of leaf nodes
    pthread_mutex_t mutex; // mutex for DFSGraph

} DFSGraph;

void init_DFSGraph(DFSGraph *DFSGraph, int num_nodes)
{
    DFSGraph->num_nodes = num_nodes;

    DFSGraph->visited = (int *)malloc(sizeof(int) * num_nodes);
    DFSGraph->matrix = (int **)malloc(sizeof(int *) * num_nodes);

    for (int i = 0; i < num_nodes; i++)
    {
        DFSGraph->visited[i] = 0;

        DFSGraph->matrix[i] = (int *)malloc(sizeof(int) * num_nodes);
        for (int j = 0; j < num_nodes; j++)
        {
            DFSGraph->matrix[i][j] = 0;
        }
    }

    DFSGraph->leaves = (int *)malloc(sizeof(int) * num_nodes);
    DFSGraph->n_leaves = 0;

    pthread_mutex_init(&DFSGraph->mutex, NULL);
}

typedef struct DFSThreadData
{
    DFSGraph *DFSGraph;
    int node;
} DFSThreadData;

void *depth_search_leaves(void *arg)
{
    DFSThreadData *td = (DFSThreadData *)arg;
    DFSGraph *DFSGraph = td->DFSGraph;
    int node = td->node;

    pthread_t threads[DFSGraph->num_nodes];
    int n_threads = 0;

    int leaf = 1;

    for (int i = 0; i < DFSGraph->num_nodes; i++)
    {
        if (DFSGraph->matrix[node][i] == 1 && DFSGraph->visited[i] == 0)
        {
            leaf = 0;
            pthread_mutex_lock(&DFSGraph->mutex);
            DFSGraph->visited[i] = 1;
            pthread_mutex_unlock(&DFSGraph->mutex);
            DFSThreadData *new_td = (DFSThreadData *)malloc(sizeof(DFSThreadData));
            new_td->DFSGraph = DFSGraph;
            new_td->node = i;

            pthread_create(&threads[n_threads++], NULL, depth_search_leaves, (void *)new_td);
        }
    }

    for (int i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    if (leaf == 1)
    {
        pthread_mutex_lock(&DFSGraph->mutex);
        DFSGraph->leaves[DFSGraph->n_leaves++] = node;
        pthread_mutex_unlock(&DFSGraph->mutex);
    }

    pthread_exit(NULL);
}

void dfs(message *msg) // msg as parameter
{
    int num;

    char name[100];

    strcpy(name, msg->contents);

    FILE *fp = fopen(name, "r");
    fscanf(fp, "%d", &num);
    int matrix[num][num];

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            fscanf(fp, "%d", &matrix[i][j]);
    }

    DFSGraph DFSGraph;
    init_DFSGraph(&DFSGraph, num);
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            DFSGraph.matrix[i][j] = matrix[i][j];
    }

    key_t key_shm;
    int shmid;

    if ((key_shm = ftok("testing.txt", msg->Sequence_Number)) == -1)
    {
        perror("error\n");
        exit(1);
    }

    if ((shmid = shmget(key_shm, sizeof(char[BUF_SIZE]), 0666)) == -1)
    {
        perror("shared memory");
        exit(1);
    }

    char *shm = (char *)shmat(shmid, NULL, 0);
    if (shm == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int start = atoi(strtok(shm, "\n"));

    start--;
    DFSGraph.visited[start] = 1;

    DFSThreadData *td = (DFSThreadData *)malloc(sizeof(DFSThreadData));
    td->DFSGraph = &DFSGraph;
    td->node = start;

    pthread_t thread;

    pthread_create(&thread, NULL, depth_search_leaves, (void *)td);

    pthread_join(thread, NULL);

    for (int i = 0; i < DFSGraph.n_leaves; i++)
    {
        sprintf(shm + strlen(shm), "%d ", DFSGraph.leaves[i] + 1);
    }
    sprintf(shm + strlen(shm), "\n");

    if (shmdt(shm) == -1)
    {
        perror("shmdt");
        exit(1);
    }
}

typedef struct BFSGraph
{
    int num_nodes;         // number of nodes in the BFSGraph
    int *visited;          // visited array
    int *current_level;    // nodes visited in current layer
    int *next_level;       // nodes that need to be visited in next layer
    int **matrix;          // adjacency matrix
    int nextc;             // index of the current_level array
    int nextn;             // index of the next_level array
    int left;              // number of nodes left to be visited
    pthread_mutex_t mutex; // mutex for BFSGraph
    int *traversal;        // array to store the traversal
    int t_index;           // index of traversal array

} BFSGraph;

void init_BFSGraph(BFSGraph *BFSGraph, int num_nodes)
{
    BFSGraph->num_nodes = num_nodes;

    BFSGraph->visited = (int *)malloc(sizeof(int) * num_nodes);
    BFSGraph->current_level = (int *)malloc(sizeof(int) * num_nodes);
    BFSGraph->next_level = (int *)malloc(sizeof(int) * num_nodes);
    BFSGraph->matrix = (int **)malloc(sizeof(int *) * num_nodes);
    BFSGraph->traversal = (int *)malloc(sizeof(int) * num_nodes);
    BFSGraph->t_index = 0;

    for (int i = 0; i < num_nodes; i++)
    {
        BFSGraph->visited[i] = 0;
        BFSGraph->current_level[i] = 0;
        BFSGraph->next_level[i] = 0;
        BFSGraph->matrix[i] = (int *)malloc(sizeof(int) * num_nodes);
        for (int j = 0; j < num_nodes; j++)
        {
            BFSGraph->matrix[i][j] = 0;
        }
    }

    BFSGraph->nextc = 0;
    BFSGraph->nextn = 0;
    BFSGraph->left = num_nodes;

    pthread_mutex_init(&BFSGraph->mutex, NULL);
}

typedef struct BFSThreadData
{
    BFSGraph *BFSGraph;
    int node;
} BFSThreadData;

void *add_to_next_level(void *arg)
{
    // printf("In thread for %d\n", ((BFSThreadData *)arg)->node + 1);
    BFSThreadData *td = (BFSThreadData *)arg;
    BFSGraph *BFSGraph = td->BFSGraph;
    int node = td->node;

    for (int i = 0; i < BFSGraph->num_nodes; i++)
    {
        if (BFSGraph->matrix[node][i] == 1 && BFSGraph->visited[i] == 0)
        {
            pthread_mutex_lock(&BFSGraph->mutex);
            BFSGraph->traversal[BFSGraph->t_index++] = i;
            BFSGraph->next_level[BFSGraph->nextn++] = i;
            BFSGraph->visited[i] = 1;
            BFSGraph->left--;
            pthread_mutex_unlock(&BFSGraph->mutex);
        }
    }

    pthread_exit(NULL);
}

void bfs(message *msg)
{
    int num;

    char name[100];

    strcpy(name, msg->contents);

    FILE *fp = fopen(name, "r");
    fscanf(fp, "%d", &num);
    int matrix[num][num];

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            fscanf(fp, "%d", &matrix[i][j]);
    }

    BFSGraph BFSGraph;
    init_BFSGraph(&BFSGraph, num);
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            BFSGraph.matrix[i][j] = matrix[i][j];
    }

    key_t key_shm;
    int shmid;

    if ((key_shm = ftok("testing.txt", msg->Sequence_Number)) == -1)
    {
        perror("error\n");
        exit(1);
    }

    if ((shmid = shmget(key_shm, sizeof(char[BUF_SIZE]), 0666)) == -1)
    {
        perror("shared memory");
        exit(1);
    }

    char *shm = (char *)shmat(shmid, NULL, 0);
    if (shm == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int start = atoi(strtok(shm, "\n"));

    start--;
    BFSGraph.visited[start] = 1;
    BFSGraph.current_level[BFSGraph.nextc++] = start;

    pthread_t threads[num];
    int n_threads = 0;

    BFSGraph.left--;
    while (BFSGraph.left > 0)
    {

        for (int i = 0; i < BFSGraph.nextc; i++)
        {
            BFSThreadData *td = (BFSThreadData *)malloc(sizeof(BFSThreadData));
            td->BFSGraph = &BFSGraph;
            td->node = BFSGraph.current_level[i];
            pthread_create(&threads[n_threads++], NULL, add_to_next_level, (void *)td);
        }

        for (int i = 0; i < n_threads; i++)
        {
            pthread_join(threads[i], NULL);
        }

        free(BFSGraph.current_level);
        BFSGraph.current_level = BFSGraph.next_level;
        BFSGraph.next_level = (int *)malloc(sizeof(int) * num);
        BFSGraph.nextc = BFSGraph.nextn;
        BFSGraph.nextn = 0;

        n_threads = 0;
    }

    for (int i = 0; i < BFSGraph.t_index; i++)
    {
        sprintf(shm + strlen(shm), "%d ", BFSGraph.traversal[i] + 1);
    }

    sprintf(shm + strlen(shm), "\n");

    if (shmdt(shm) == -1)
    {
        perror("shmdt");
        exit(1);
    }
}

void *func(void *data)
{
    ThreadData *td = (ThreadData *)data;

    int msqid = td->msqid;
    message msg = td->msg;

    int shmid;
    key_t key_shm;

    // Create and initialize the semaphore (wrt)
    sem = sem_open(msg.contents, O_CREAT, PERMS, 1);
    if (sem == SEM_FAILED)
    {
        if (errno != EEXIST)
        {
            perror("sem_open");
            exit(1);
        }
        else
        {
            sem = sem_open(msg.contents, 0); // Semaphore already exists, open it without O_CREAT
            if (sem == SEM_FAILED)
            {
                perror("sem_open");
                exit(1);
            }
        }
    }

    // // Create and initialize the semaphore (read)
    // char name[100];
    // strcpy(name, msg.contents);
    // strcat(name, "_read");

    // sem_read = sem_open(name, O_CREAT, PERMS, 1);
    // if (sem_read == SEM_FAILED)
    // {
    //     if (errno != EEXIST)
    //     {
    //         perror("sem_open");
    //         exit(1);
    //     }
    //     else
    //     {
    //         sem_read = sem_open(msg.contents, 0); // Semaphore already exists, open it without O_CREAT
    //         if (sem_read == SEM_FAILED)
    //         {
    //             perror("sem_open");
    //             exit(1);
    //         }
    //     }
    // }

    printf("getting read lock: inc\n");
    pthread_mutex_lock(&mutex);

    int f = extractNumber(msg.contents);
    n_readers[f]++;
    if (n_readers[f] == 1)
    {
        printf("locking write for file %d\n", f);
        sem_wait(sem);
    }

    pthread_mutex_unlock(&mutex);
    printf("releasing read lock: inc\n");

    if (msg.Operation_Number == 3)
    {
        msg.mtype = msg.Sequence_Number * 10;
        dfs(&msg);
        char mess[100] = "DFS successfully performed\n";
        strcpy(msg.contents, mess);
    }

    else if (msg.Operation_Number == 4)
    {
        msg.mtype = msg.Sequence_Number * 10;
        bfs(&msg);
        char mess[100] = "BFS successfully performed\n";
        strcpy(msg.contents, mess);
    }

    if (msgsnd(msqid, &msg, sizeof(message) - sizeof(long), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    printf("sleeping for 10 seconds\n");
    sleep(10);

    printf("getting read lock: reduce\n");
    pthread_mutex_lock(&mutex);
    n_readers[f]--;
    if (n_readers[f] == 0)
    {
        sem_post(sem);
        printf("unlocking write for file %d\n", f);
    }
    pthread_mutex_unlock(&mutex);
    printf("releasing read lock: reduce\n");

    free(td);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

    for (int i = 0; i < 30; i++)
    {
        n_readers[i] = 0;
    }

    pthread_mutex_init(&mutex, NULL);

    if (argc != 2)
    {
        printf("Please enter the server number as an argument\n");
        exit(1);
    }

    int server_number = atoi(argv[1]);

    if (server_number != 1 && server_number != 2)
    {
        printf("Please enter a valid server number (1 or 2)\n");
        exit(1);
    }

    message msg;
    int n_threads = 0;

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

    pthread_t threads[MAX_THREADS];

    while (1)
    {
        message msg;
        if (msgrcv(msqid, &msg, sizeof(message) - sizeof(long), server_number, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        if (msg.Operation_Number == 0)
        {
            for (int i = 0; i < n_threads; i++)
            {
                printf("Waiting for thread %d to finish...\n", i);
                pthread_join(threads[i], NULL);
            }
            break;
        }

        else
        {
            ThreadData *td = (ThreadData *)malloc(sizeof(ThreadData));
            td->msqid = msqid;
            td->msg = msg;
            pthread_create(&threads[n_threads++], NULL, func, (void *)td);
        }
    }
    return 0;
}