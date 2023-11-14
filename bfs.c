#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
    int *traversal;        // array to store the travesal
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

    BFSGraph BFSGraph;
    init_BFSGraph(&BFSGraph, num);
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            BFSGraph.matrix[i][j] = matrix[i][j];
    }

    int start;
    printf("Enter the starting node: ");
    scanf("%d", &start);
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

    printf("%d ", start + 1);
    for (int i = 0; i < BFSGraph.t_index; i++)
    {
        printf("%d ", BFSGraph.traversal[i] + 1);
    }
    printf("\n");
    return 0;
}