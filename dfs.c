#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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

int main()
{

    int num;
    char name[100];
    printf("Enter the name of the file: ");
    scanf("%s", name);
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

    int start;
    printf("Enter the starting node: ");
    scanf("%d", &start);
    start--;
    DFSGraph.visited[start] = 1;

    DFSThreadData *td = (DFSThreadData *)malloc(sizeof(DFSThreadData));
    td->DFSGraph = &DFSGraph;
    td->node = start;

    pthread_t thread;

    pthread_create(&thread, NULL, depth_search_leaves, (void *)td);

    pthread_join(thread, NULL);

    printf("The leaf nodes are: ");

    for (int i = 0; i < DFSGraph.n_leaves; i++)
    {
        printf("%d ", DFSGraph.leaves[i] + 1);
    }

    printf("\n");

    return 0;
}